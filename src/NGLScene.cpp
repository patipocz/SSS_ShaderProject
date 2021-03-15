#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/NGLInit.h>
#include <iostream>

#include <ngl/NGLStream.h>
#include <ngl/ShaderLib.h>
#include <ngl/VAOPrimitives.h>


NGLScene::NGLScene()
{
  // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
  setTitle("Blank NGL");
}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}



void NGLScene::resizeGL(int _w , int _h)
{
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}


void NGLScene::initializeGL()
{
  // we must call that first before any other GL commands to load and link the
  // gl commands from the lib, if that is not done program will crash
  ngl::NGLInit::initialize();
  glClearColor(1.0f, 0.5f, 0.5f, 1.0f);			   // reddish grey is cute
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);
  
  //shader called Constant shader program
  ngl::ShaderLib::createShaderProgram("Constant");
  //empty shaders vertex and fragment
  ngl::ShaderLib::attachShader("ConstantVertex", ngl::ShaderType::VERTEX);
  ngl::ShaderLib::attachShader("ConstantFragment", ngl::ShaderType::FRAGMENT);
  //attach source

  ngl::ShaderLib::loadShaderSource("ConstantVertex", "shaders/ConstantVertex.glsl");
  ngl::ShaderLib::loadShaderSource("ConstantFragment", "shaders/ConstantFragment.glsl");

  //compile shaders
  ngl::ShaderLib::compileShader("ConstantVertex");
  ngl::ShaderLib::compileShader("ConstantFragment");
  //add them to shader program
  ngl::ShaderLib::attachShaderToProgram("Constant", "ConstantVertex");
  ngl::ShaderLib::attachShaderToProgram("Constant", "ConstantFragment");


  //link the shader
  ngl::ShaderLib::linkProgramObject("Constant");
  //make active ready to load values
  ngl::ShaderLib::use("Constant");
  ngl::ShaderLib::setUniform("Colour", 1.0f, 0.0f, 0.0f, 1.0f);

  //basic camera from graphics lib, static so only set once
  //values for camera position
  
  ngl::Vec3 from(0, 2, 2);
  ngl::Vec3 to(0, 0, 0);
  ngl::Vec3 up(0, 1, 0);

  //load to camera
  m_view = ngl::lookAt(from, to, up);

  //set shape using FOV 4 Aspect Ratio based on width and height
  m_project = ngl::perspective(45, (float)720.0 / 576.0, 0.5, 350);

  //resize
 //glViewport(0, 0, width), height());


}


//idk what matrices and why it is loading
void NGLScene::loadMatricesToShader()
{

    ngl::Mat4 MVP;
    MVP = m_project * m_view * m_mouseGlobalTX;
    ngl::ShaderLib::setUniform("MVP", MVP);
}


void NGLScene::paintGL()
{
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,m_win.width,m_win.height);
  ngl::ShaderLib::use("Constant");


 // Rotation based on the mouse position for our global transform
 // Rotation based on the mouse position for our global transform
 // ngl::Transformation trans;
  // Rotation based on the mouse position for our global
  // transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX(m_spinXFace);
  rotY.rotateY(m_spinYFace);
  // multiply the rotations
  m_mouseGlobalTX = rotX * rotY;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;
  // draw dragon teapot or buddahor bunny here buddah
  loadMatricesToShader();
  ngl::VAOPrimitives::draw("bunny");


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
  case Qt::Key_W: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
      // turn off wire frame
  case Qt::Key_S: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
      // show full screen
  case Qt::Key_F: showFullScreen(); break;
      // show windowed
  case Qt::Key_N: showNormal(); break;
  default: break;
  }
  // finally update the GLWindow and re-draw

    update();
}

void NGLScene::mouseMoveEvent(QMouseEvent* _event)
{
    // note the method buttons() is the button state when event was called
    // this is different from button() which is used to check which button was
    // pressed when the mousePress/Release event is generated
    if (m_rotate && _event->buttons() == Qt::LeftButton)
    {
        int diffx = _event->x() - m_origX;
        int diffy = _event->y() - m_origY;
        m_spinXFace += (float)0.5f * diffy;
        m_spinYFace += (float)0.5f * diffx;
        m_origX = _event->x();
        m_origY = _event->y();
        update();

    }
    // right mouse translate code
    else if (m_translate && _event->buttons() == Qt::RightButton)
    {
        int diffX = (int)(_event->x() - m_origXPos);
        int diffY = (int)(_event->y() - m_origYPos);
        m_origXPos = _event->x();
        m_origYPos = _event->y();
        m_modelPos.m_x += INCREMENT * diffX;
        m_modelPos.m_y -= INCREMENT * diffY;
        update();

    }
}
void NGLScene::mousePressEvent(QMouseEvent* _event)
{
    // this method is called when the mouse button is pressed in this case we
    // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
    if (_event->button() == Qt::LeftButton)
    {
        m_origX = _event->x();
        m_origY = _event->y();
        m_rotate = true;
    }
    // right mouse translate mode
    else if (_event->button() == Qt::RightButton)
    {
        m_origXPos = _event->x();
        m_origYPos = _event->y();
        m_translate = true;
    }

}
void NGLScene::mouseReleaseEvent(QMouseEvent* _event)
{
    // this event is called when the mouse button is released
    // we then set Rotate to false
    if (_event->button() == Qt::LeftButton)
    {
        m_rotate = false;
    }
    // right mouse translate mode
    if (_event->button() == Qt::RightButton)
    {
        m_translate = false;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent* _event)
{

    // check the diff of the wheel position (0 means no change)
    if (_event->delta() > 0)
    {
        m_modelPos.m_z += ZOOM;
    }
    else if (_event->delta() < 0)
    {
        m_modelPos.m_z -= ZOOM;
    }
    update();
}
