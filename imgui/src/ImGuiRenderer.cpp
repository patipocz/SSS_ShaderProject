#include "ImGuiRenderer.h"
#include <QDateTime>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QClipboard>
#include <QCursor>
#include <QDebug>
#include <iostream>
#include <ngl/ShaderLib.h>
#include <ngl/SimpleIndexVAO.h>
#include <ngl/VAOFactory.h>
namespace QtImGui {
static const char* ImGUIShader="ImGUIShader";
namespace {

QHash<int, ImGuiKey> keyMap = {
    { Qt::Key_Tab, ImGuiKey_Tab },
    { Qt::Key_Left, ImGuiKey_LeftArrow },
    { Qt::Key_Right, ImGuiKey_RightArrow },
    { Qt::Key_Up, ImGuiKey_UpArrow },
    { Qt::Key_Down, ImGuiKey_DownArrow },
    { Qt::Key_PageUp, ImGuiKey_PageUp },
    { Qt::Key_PageDown, ImGuiKey_PageDown },
    { Qt::Key_Home, ImGuiKey_Home },
    { Qt::Key_End, ImGuiKey_End },
    { Qt::Key_Delete, ImGuiKey_Delete },
    { Qt::Key_Backspace, ImGuiKey_Backspace },
    { Qt::Key_Enter, ImGuiKey_Enter },
    { Qt::Key_Escape, ImGuiKey_Escape },
    { Qt::Key_A, ImGuiKey_A },
    { Qt::Key_C, ImGuiKey_C },
    { Qt::Key_V, ImGuiKey_V },
    { Qt::Key_X, ImGuiKey_X },
    { Qt::Key_Y, ImGuiKey_Y },
    { Qt::Key_Z, ImGuiKey_Z },
};

QByteArray g_currentClipboardText;

}

void ImGuiRenderer::initialize(WindowWrapper *window)
{
    m_window.reset(window);

    ImGuiIO &io = ImGui::GetIO();
    for (ImGuiKey key : keyMap.values()) {
        io.KeyMap[key] = key;
    }

    io.RenderDrawListsFn = [](ImDrawData *drawData) {
        instance()->renderDrawList(drawData);
    };
    io.SetClipboardTextFn = [](void *user_data, const char *text) {
        Q_UNUSED(user_data);
        QGuiApplication::clipboard()->setText(text);
    };
    io.GetClipboardTextFn = [](void *user_data) {
        Q_UNUSED(user_data);
        g_currentClipboardText = QGuiApplication::clipboard()->text().toUtf8();
        return (const char *)g_currentClipboardText.data();
    };

    window->installEventFilter(this);
}

void ImGuiRenderer::renderDrawList(ImDrawData *draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup GL state
    GLint last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
    glActiveTexture(GL_TEXTURE0);
    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, &last_blend_src_rgb);
    GLint last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, &last_blend_dst_rgb);
    GLint last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src_alpha);
    GLint last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dst_alpha);
    GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    //const float ortho_projection[4][4] =
    ngl::Mat4 ortho_projection
    (
         2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f ,
         0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f ,
         0.0f,                  0.0f,                  -1.0f, 0.0f ,
        -1.0f,                  1.0f,                   0.0f, 1.0f
    );
    ngl::ShaderLib::use(ImGUIShader);
    ngl::ShaderLib::setUniform("Texture",0);
    ngl::ShaderLib::setUniform("ProjMtx",ortho_projection);
    std::unique_ptr<ngl::AbstractVAO> vao;
    vao=ngl::VAOFactory::createVAO(ngl::simpleIndexVAO,GL_TRIANGLES);

    vao->bind();
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = nullptr;
        vao->setData(ngl::SimpleIndexVAO::VertexData(
                         cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
                         cmd_list->VtxBuffer.Data->pos.x,
                         (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),(const GLvoid*)cmd_list->IdxBuffer.Data,
                         sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT));
        vao->setVertexAttributePointer(0,2,GL_FLOAT,sizeof(ImDrawVert), 0);
        vao->setVertexAttributePointer(1,2,GL_FLOAT,sizeof(ImDrawVert), 2);
        vao->setVertexAttributePointer(2,4,GL_UNSIGNED_BYTE,sizeof(ImDrawVert), 4,GL_TRUE);

        vao->setNumIndices(cmd_list->IdxBuffer.Size);
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
               // glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
               vao->draw();
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }
    vao->unbind();
    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glActiveTexture(last_active_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

bool ImGuiRenderer::createFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

bool ImGuiRenderer::createDeviceObjects()
{

  // Backup GL state
  GLint last_texture, last_array_buffer, last_vertex_array;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
  ngl::ShaderLib::loadShader(ImGUIShader,"shaders/imguiVertex.glsl","shaders/imguiFragment.glsl");

  createFontsTexture();
  // Restore modified GL state
  glBindTexture(GL_TEXTURE_2D, last_texture);
  glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
  glBindVertexArray(last_vertex_array);

  return true;
}

void ImGuiRenderer::newFrame()
{
    if (!g_FontTexture)
        createDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2(m_window->size().width(), m_window->size().height());
    io.DisplayFramebufferScale = ImVec2(m_window->devicePixelRatio(), m_window->devicePixelRatio());

    // Setup time step
    double current_time =  QDateTime::currentMSecsSinceEpoch() / double(1000);
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if (m_window->isActive())
    {
        auto pos = m_window->mapFromGlobal(QCursor::pos());
        io.MousePos = ImVec2(pos.x(), pos.y());   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
    }
    else
    {
        io.MousePos = ImVec2(-1,-1);
    }

    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = g_MousePressed[i];
    }

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    // glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    // Start the frame
    ImGui::NewFrame();
}

void ImGuiRenderer::onMousePressedChange(QMouseEvent *event)
{
    g_MousePressed[0] = event->buttons() & Qt::LeftButton;
    g_MousePressed[1] = event->buttons() & Qt::RightButton;
    g_MousePressed[2] = event->buttons() & Qt::MiddleButton;
}

void ImGuiRenderer::onWheel(QWheelEvent *event)
{
    // 5 lines per unit
    g_MouseWheel += event->pixelDelta().y() / (5.0 * ImGui::GetTextLineHeight());
}

void ImGuiRenderer::onKeyPressRelease(QKeyEvent *event)
{
    ImGuiIO& io = ImGui::GetIO();
    if (keyMap.contains(event->key())) {
        io.KeysDown[keyMap[event->key()]] = event->type() == QEvent::KeyPress;
    }

    if (event->type() == QEvent::KeyPress) {
        QString text = event->text();
        if (text.size() == 1) {
            io.AddInputCharacter(text.at(0).unicode());
        }
    }

#ifdef Q_OS_MAC
    io.KeyCtrl = event->modifiers() & Qt::MetaModifier;
    io.KeyShift = event->modifiers() & Qt::ShiftModifier;
    io.KeyAlt = event->modifiers() & Qt::AltModifier;
    io.KeySuper = event->modifiers() & Qt::ControlModifier; // Comamnd key
#else
    io.KeyCtrl = event->modifiers() & Qt::ControlModifier;
    io.KeyShift = event->modifiers() & Qt::ShiftModifier;
    io.KeyAlt = event->modifiers() & Qt::AltModifier;
    io.KeySuper = event->modifiers() & Qt::MetaModifier;
#endif
}

bool ImGuiRenderer::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        this->onMousePressedChange(static_cast<QMouseEvent *>(event));
        break;
    case QEvent::Wheel:
        this->onWheel(static_cast<QWheelEvent *>(event));
        break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        this->onKeyPressRelease(static_cast<QKeyEvent *>(event));
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

ImGuiRenderer* ImGuiRenderer::instance() {
    static ImGuiRenderer* instance = nullptr;
    if (!instance) {
        instance = new ImGuiRenderer();
    }
    return instance;
}

}
