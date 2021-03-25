#include "NGLScene.h"
#include <ngl/NGLInit.h>
#include <ngl/NGLStream.h>
#include <ngl/ShaderLib.h>
#include <ngl/VAOPrimitives.h>
#include <QGuiApplication>
#include <QMouseEvent>

#include "imgui.h"
#include "QtImGui.h"
#include "ImGuizmo.h"


NGLScene::NGLScene()
{
  setTitle( "Frostbite Style SSS Qt5 Simple NGL Demo" );
}


NGLScene::~NGLScene()
{
  std::cout << "Shutting down NGL, removing VAO's and Shaders\n";
}


void NGLScene::resizeGL( int _w, int _h )
{
  m_projection=ngl::perspective( 45.0f, static_cast<float>( _w ) / _h, 0.1f, 200.0f );

  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}
constexpr auto shaderProgram = "PBR";

void NGLScene::reloadShaders() {
    // now to load the shader and set the values
  // grab an instance of shader manager
  // in the code create some constexpr
    constexpr auto vertexShader = "PBRVertex";
    constexpr auto fragShader = "PBRFragment";

    // create the shader program
    ngl::ShaderLib::createShaderProgram(shaderProgram);
    // now we are going to create empty shaders for Frag and Vert
    ngl::ShaderLib::attachShader( vertexShader, ngl::ShaderType::VERTEX );
    ngl::ShaderLib::attachShader( fragShader, ngl::ShaderType::FRAGMENT );
    // attach the source
    ngl::ShaderLib::loadShaderSource( vertexShader, "shaders/PBRVertex.glsl" );
    ngl::ShaderLib::loadShaderSource( fragShader, "shaders/PBRFragment.glsl" );
    // compile the shaders
    ngl::ShaderLib::compileShader( vertexShader );
    ngl::ShaderLib::compileShader( fragShader );
    // add them to the program
    ngl::ShaderLib::attachShaderToProgram( shaderProgram, vertexShader );
    ngl::ShaderLib::attachShaderToProgram( shaderProgram, fragShader );
    // now we have associated that data we can link the shader
    ngl::ShaderLib::linkProgramObject( shaderProgram );
    // and make it active ready to load values
    ngl::ShaderLib::use(shaderProgram );
}

void NGLScene::initializeGL()
{
    // uniform defaults
    m_lightColour = ngl::Vec3(.4, .6, .4);
    m_distortion = 0.1;
    m_thickness = 1;
    m_scale = 1;
    m_power = 3;
    m_attenuation = 1;

    m_time = 0.0;
    m_transformLight = true; // Toggle this by pressing L

    // we must call that first before any other GL commands to load and link the
    // gl commands from the lib, if that is not done program will crash
    ngl::NGLInit::initialize();
    // uncomment this line to make ngl less noisy with debug info
    // ngl::NGLInit::instance()->setCommunicationMode( ngl::CommunicationMode::NULLCONSUMER);
    glClearColor( 0.4f, 0.4f, 0.4f, 1.0f ); // Grey Background
    // enable depth testing for drawing
    glEnable( GL_DEPTH_TEST );
    // enable multisampling for smoother drawing
    glEnable( GL_MULTISAMPLE );

    reloadShaders();
  
    // We now create our view matrix for a static camera
    ngl::Vec3 from{ 0.0f, 2.0f, 2.0f };
    ngl::Vec3 to{ 0.0f, 0.0f, 0.0f };
    ngl::Vec3 up{ 0.0f, 1.0f, 0.0f };

    // now load to our new camera
    m_view = ngl::lookAt(from,to,up);
    ngl::ShaderLib::setUniform( "camPos", from );
    
    // hardcoded cos the floor looks weird when we move our light
    ngl::Vec3 envlightPos = ngl::Vec3(0.0, 2.0f, 1.0f);
    
    // setup the default shader material and light porerties
    // these are "uniform" so will retain their values
    ngl::ShaderLib::setUniform("lightPosition", envlightPos);
    ngl::ShaderLib::setUniform("lightColor",400.0f,400.0f,400.0f);
    ngl::ShaderLib::setUniform("exposure",2.2f);
    ngl::ShaderLib::setUniform("albedo",0.950f, 0.71f, 0.29f);

    ngl::ShaderLib::setUniform("diffuseKd", 0.4f, 0.6f, 0.4f);

    ngl::ShaderLib::setUniform("metallic",1.02f);
    ngl::ShaderLib::setUniform("roughness",0.38f);
    ngl::ShaderLib::setUniform("ao",0.3f);
    ngl::VAOPrimitives::createTrianglePlane("floor",20,20,1,1,ngl::Vec3::up());
    ngl::ShaderLib::printRegisteredUniforms(shaderProgram);

    // Used to preview the light position
    ngl::VAOPrimitives::createSphere("sphere", 0.1f, 50);

    ngl::ShaderLib::use(ngl::nglCheckerShader);
    ngl::ShaderLib::setUniform("lightDiffuse",1.0f,1.0f,1.0f,1.0f);
    ngl::ShaderLib::setUniform("checkOn",true);
    
    ngl::ShaderLib::setUniform("colour1",0.9f,0.9f,0.9f,1.0f);
    ngl::ShaderLib::setUniform("colour2",0.6f,0.6f,0.6f,1.0f);
    ngl::ShaderLib::setUniform("checkSize",60.0f);
    ngl::ShaderLib::setUniform("lightPos", envlightPos);
    ngl::ShaderLib::printRegisteredUniforms(ngl::nglCheckerShader);

    ngl::ShaderLib::use(ngl::nglDiffuseShader);
    ngl::ShaderLib::setUniform("Colour", 1.0f, 1.0f, 1.0f, 1.0f);
    ngl::ShaderLib::setUniform("lightDiffuse", 1.0f, 1.0f, 1.0f, 1.0f);
    ngl::ShaderLib::setUniform("lightPos", envlightPos);
    ngl::ShaderLib::printRegisteredUniforms(ngl::nglDiffuseShader);
    
    // Trigger the update timer every 40ms
    m_lightTimer = startTimer(40);

    QtImGui::initialize(this);
}


void NGLScene::loadMatricesToShader()
{   
    struct transform
    {
        ngl::Mat4 MVP;
        ngl::Mat4 normalMatrix;
        ngl::Mat4 M;
    };

    transform t;
    t.M = m_view * m_mouseGlobalTX;
    t.MVP = m_projection * t.M;
    t.normalMatrix = t.M;
    t.normalMatrix.inverse().transpose();
    ngl::ShaderLib::setUniformBuffer("TransformUBO",sizeof(transform),&t.MVP.m_00);
}

void NGLScene::paintGL()
{
    glViewport( 0, 0, m_win.width, m_win.height );
    // clear the screen and depth buffer
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Rotation based on the mouse position for our global transform
    ngl::Mat4 rotX;
    ngl::Mat4 rotY;
    // create the rotation matrices
    rotX.rotateX( m_win.spinXFace );
    rotY.rotateY( m_win.spinYFace );
    // multiply the rotations
    m_mouseGlobalTX = rotX * rotY;
    // add the translations
    m_mouseGlobalTX.m_m[ 3 ][ 0 ] = m_modelPos.m_x;
    m_mouseGlobalTX.m_m[ 3 ][ 1 ] = m_modelPos.m_y;
    m_mouseGlobalTX.m_m[ 3 ][ 2 ] = m_modelPos.m_z;
    
    // Draw SSS teapot
    {
        ngl::ShaderLib::use("PBR");

        // get the VBO instance and draw the built in teapot draw
        loadMatricesToShader();
        
        ngl::ShaderLib::setUniform("lightAmbient", m_lightColour);
        ngl::ShaderLib::setUniform("distortion", m_distortion);
        ngl::ShaderLib::setUniform("thickness", m_thickness);
        ngl::ShaderLib::setUniform("scale", m_scale);
        ngl::ShaderLib::setUniform("power", m_power);
        ngl::ShaderLib::setUniform("attenuation", m_attenuation);

        ngl::ShaderLib::setUniform("lightPosition", m_rotatedLightPos);
        ngl::VAOPrimitives::draw("dragon");
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
    
    drawUI();
}

//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent( QKeyEvent* _event )
{
  // that method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch ( _event->key() )
  {
    // escape key to quit
    case Qt::Key_Escape:
      QGuiApplication::exit( EXIT_SUCCESS );
      break;
// turn on wirframe rendering
#ifndef USINGIOS_
    case Qt::Key_W:
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      break;
    // turn off wire frame
    case Qt::Key_S:
      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
      break;
#endif
    // show full screen
    case Qt::Key_F:
      showFullScreen();
      break;
    // show windowed
    case Qt::Key_N:
      showNormal();
      break;
    case Qt::Key_Space :
      m_win.spinXFace=0;
      m_win.spinYFace=0;
      m_modelPos.set(ngl::Vec3::zero());
    break;

    case Qt::Key_L :
    m_transformLight^=true;
    break;
    case Qt::Key_U:
      reloadShaders();
      break;
    default:
      break;
  }
  update();
}

void NGLScene::updateLight() {    
    float t = m_time * 0.04;
    float x = std::sin(t);
    float y = 1.0;
    float z = std::cos(t);

    ngl::Mat4 rotateX;
    rotateX.rotateX(x);
    ngl::Mat4 rotateZ;
    rotateZ.rotateZ(z);

    ngl::Mat4 rotation = rotateZ * rotateX;

    //const ngl::Vec4 lightOrigin = m_lightPos;
    //ngl::Vec3 rotatedLight = (m_lightPos * rotation).toVec3();
    ngl::Vec3 rotatedLight = ngl::Vec3(x, y, z) * 10;
    m_rotatedLightPos = rotatedLight;

    m_time += 1;
}


void NGLScene::timerEvent(QTimerEvent* _event) {
    if (_event->timerId() == m_lightTimer && m_transformLight == true) {
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