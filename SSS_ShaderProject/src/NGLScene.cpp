#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/NGLInit.h>
#include <ngl/Mat4.h>
#include <ngl/ShaderLib.h>
#include <ngl/VAOPrimitives.h>
#include <iostream>

#include "imgui.h"
#include "QtImGui.h"
#include "ImGuizmo.h"


constexpr auto sssShaderProgram = "SSS";

NGLScene::NGLScene()
{
  // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
  setTitle("SSS Demo");
}

NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::resizeGL(int _w , int _h)
{
    m_projection = ngl::perspective(45.0f, static_cast<float>(_w) / _h, 0.1f, 200.0f);

    m_win.width  = static_cast<int>( _w * devicePixelRatio() );
    m_win.height = static_cast<int>( _h * devicePixelRatio() );
}

void NGLScene::initializeGL()
{
    // we must call that first before any other GL commands to load and link the
    // gl commands from the lib, if that is not done program will crash
    ngl::NGLInit::initialize();
    
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f); // Grey Background

    // enable depth testing for drawing
    glEnable(GL_DEPTH_TEST);
    // enable multisampling for smoother drawing
    glEnable(GL_MULTISAMPLE);

    // Load the SSS shaders
    {
        constexpr auto vertexShader = "SSSVertex";
        constexpr auto fragShader = "SSSFragment";

        // create the shader program
        ngl::ShaderLib::createShaderProgram(sssShaderProgram);
        // now we are going to create empty shaders for Frag and Vert
        ngl::ShaderLib::attachShader(vertexShader, ngl::ShaderType::VERTEX);
        ngl::ShaderLib::attachShader(fragShader, ngl::ShaderType::FRAGMENT);
        // attach the source
        ngl::ShaderLib::loadShaderSource(vertexShader, "shaders/SSSVertex.glsl");
        ngl::ShaderLib::loadShaderSource(fragShader, "shaders/SSSFragment.glsl");
        // compile the shaders
        ngl::ShaderLib::compileShader(vertexShader);
        ngl::ShaderLib::compileShader(fragShader);
        // add them to the program
        ngl::ShaderLib::attachShaderToProgram(sssShaderProgram, vertexShader);
        ngl::ShaderLib::attachShaderToProgram(sssShaderProgram, fragShader);
        // now we have associated that data we can link the shader
        ngl::ShaderLib::linkProgramObject(sssShaderProgram);
        // and make it active ready to load values
        ngl::ShaderLib::use(sssShaderProgram);
    }
    
    // We now create our view matrix for a static camera
    ngl::Vec3 from{ 0.0f, 2.0f, 2.0f };
    ngl::Vec3 to{ 0.0f, 0.0f, 0.0f };
    ngl::Vec3 up{ 0.0f, 1.0f, 0.0f };

    // now load to our new camera
    m_view = ngl::lookAt(from, to, up);
    ngl::ShaderLib::setUniform("camPos", from);

    // hardcoded cos the floor looks weird when we move our light
    ngl::Vec3 envlightPos = ngl::Vec3(0.0, 2.0f, 1.0f);

    // setup the default shader material and light uniforms
    {
        ngl::ShaderLib::setUniform("lightPosition", envlightPos);
        ngl::ShaderLib::setUniform("lightColor", 400.0f, 400.0f, 400.0f);
    }
    
    // create vaoprimitives so we can use them later
    {
        ngl::VAOPrimitives::createTrianglePlane("floor", 20, 20, 1, 1, ngl::Vec3::up());
        ngl::VAOPrimitives::createSphere("sphere", 0.1f, 50); // sphere used to preview the light position
    }

    // set default uniforms for the checker shader
    {
        ngl::ShaderLib::use(ngl::nglCheckerShader);
        ngl::ShaderLib::setUniform("lightDiffuse", 1.0f, 1.0f, 1.0f, 1.0f);
        ngl::ShaderLib::setUniform("checkOn", true);

        ngl::ShaderLib::setUniform("colour1", 0.9f, 0.9f, 0.9f, 1.0f);
        ngl::ShaderLib::setUniform("colour2", 0.6f, 0.6f, 0.6f, 1.0f);
        ngl::ShaderLib::setUniform("checkSize", 60.0f);
        ngl::ShaderLib::setUniform("lightPos", envlightPos);
    }

    // set default uniforms for the diffuse shader
    {
        ngl::ShaderLib::use(ngl::nglDiffuseShader);
        ngl::ShaderLib::setUniform("Colour", 1.0f, 1.0f, 1.0f, 1.0f);
        ngl::ShaderLib::setUniform("lightDiffuse", 1.0f, 1.0f, 1.0f, 1.0f);
        ngl::ShaderLib::setUniform("lightPos", envlightPos);
    }

    // trigger the update timer every 40ms so the light can move
    m_lightTimer = startTimer(40);

    // for the SSS shader, these get updated by imgui in drawUI() and get passed to the shader in paintGL()
    m_lightColour = ngl::Vec3(.4, .6, .4);
    m_distortion = 0.1;
    m_thickness = 1;
    m_scale = 1;
    m_power = 3;
    m_attenuation = 1;
    m_time = 0.0;

    // initialize imgui
    QtImGui::initialize(this);
}

void NGLScene::loadMatricesToShader()
{
    // this matches the TransformUBO struct in SSSVertex
    struct transform
    {
        ngl::Mat4 MVP;
        ngl::Mat4 normalMatrix;
        ngl::Mat4 M;
    };

    // The dragon is big, so scale him down
    ngl::Mat4 tx;
    tx.translate(0, -0.5, 0);
    tx.scale(0.1, 0.1, 0.1);

    // update the shader transform uniforms
    transform t;
    t.M = m_view * m_mouseGlobalTX * tx;
    t.MVP = m_projection * t.M;
    t.normalMatrix = t.M;
    t.normalMatrix.inverse().transpose();
    ngl::ShaderLib::setUniformBuffer("TransformUBO", sizeof(transform), &t.MVP.m_00);
}

void NGLScene::paintGL()
{
    // clear the screen and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,m_win.width,m_win.height);

    // Rotation based on the mouse position for our global transform
    ngl::Mat4 rotX;
    ngl::Mat4 rotY;
    // create the rotation matrices
    rotX.rotateX(m_win.spinXFace);
    rotY.rotateY(m_win.spinYFace);
    // multiply the rotations
    m_mouseGlobalTX = rotX * rotY;
    // add the translations
    m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
    m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
    m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;

    // Draw SSS buddha
    {
        ngl::ShaderLib::use(sssShaderProgram);

        // get the VBO instance and draw the built in dragon draw
        loadMatricesToShader();

        ngl::ShaderLib::setUniform("lightAmbient", m_lightColour);
        ngl::ShaderLib::setUniform("distortion", m_distortion);
        ngl::ShaderLib::setUniform("thickness", m_thickness);
        ngl::ShaderLib::setUniform("scale", m_scale);
        ngl::ShaderLib::setUniform("power", m_power);
        ngl::ShaderLib::setUniform("attenuation", m_attenuation);

        ngl::ShaderLib::setUniform("lightPosition", m_rotatedLightPos);
        ngl::VAOPrimitives::draw("buddah");
    }

    // Draw floor
    {
        ngl::ShaderLib::use(ngl::nglCheckerShader);

        // move the floor down
        ngl::Mat4 tx;
        tx.translate(0.0f, -0.45f, 0.0f);
        ngl::Mat4 MVP = m_projection * m_view * m_mouseGlobalTX * tx;
        ngl::Mat3 normalMatrix = m_view * m_mouseGlobalTX;
        normalMatrix.inverse().transpose();

        ngl::ShaderLib::setUniform("MVP", MVP);
        ngl::ShaderLib::setUniform("normalMatrix", normalMatrix);

        ngl::VAOPrimitives::draw("floor");
    }

    // Draw light sphere preview for debugging
    {
        ngl::ShaderLib::use(ngl::nglDiffuseShader);

        ngl::Mat4 tx;
        tx.translate(m_rotatedLightPos.m_x, m_rotatedLightPos.m_y, m_rotatedLightPos.m_z);
        ngl::Mat4 MVP = m_projection * m_view * m_mouseGlobalTX * tx;
        ngl::Mat3 normalMatrix = m_view * m_mouseGlobalTX;
        normalMatrix.inverse().transpose();
        ngl::ShaderLib::setUniform("MVP", MVP);
        ngl::ShaderLib::setUniform("normalMatrix", normalMatrix);

        loadMatricesToShader();
        ngl::VAOPrimitives::draw("sphere");
    }

    // draw the imgui stuff
    drawUI();
}

//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
    // this method is called every time the main window recives a key event.
    // we then switch on the key value and set the camera in the GLWindow
    switch (_event->key())
    {
    // escape key to quite
    case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
    case Qt::Key_Space :
        m_win.spinXFace=0;
        m_win.spinYFace=0;
        m_modelPos.set(ngl::Vec3::zero());

    break;
        default : break;
    }
    // finally update the GLWindow and re-draw

    update();
}

void NGLScene::updateLight() {
    float t = m_time * 0.04;
    float x = std::sin(t);
    float y = .25;
    float z = std::cos(t);

    ngl::Mat4 rotateX;
    rotateX.rotateX(x);
    ngl::Mat4 rotateZ;
    rotateZ.rotateZ(z);

    ngl::Mat4 rotation = rotateZ * rotateX;

    ngl::Vec3 rotatedLight = ngl::Vec3(x, y, z) * 2;
    m_rotatedLightPos = rotatedLight;

    m_time += 1;
}


void NGLScene::timerEvent(QTimerEvent* _event) {
    if (_event->timerId() == m_lightTimer) {
        updateLight();
    }

    // re-draw GL
    update();
}

void NGLScene::drawUI()
{
    QtImGui::newFrame();

    ImGui::Begin("SSS");

    ImGui::SliderFloat("LightR", &m_lightColour.m_r, 0.0f, 1.0f);
    ImGui::SliderFloat("LightG", &m_lightColour.m_g, 0.0f, 1.0f);
    ImGui::SliderFloat("LightB", &m_lightColour.m_b, 0.0f, 1.0f);
    ImGui::Separator();
    ImGui::SliderFloat("Distortion", &m_distortion, 0.0f, 1.0f);
    ImGui::SliderFloat("Thickness", &m_thickness, 0.0f, 32.0f);
    ImGui::SliderFloat("Scale", &m_scale, 0.0f, 2.0f);
    ImGui::SliderFloat("Power", &m_power, 0.0f, 64.0f);
    ImGui::SliderFloat("Attenuation", &m_attenuation, 0.0f, 2.0f);

    ImGui::End();

    ImGui::Render();
}