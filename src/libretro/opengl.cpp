#include <glsm/glsm.h>

#include "input.h"
#include "libretro_state.h"
#include "screenlayout.h"
#include "utils.h"

#include "Config.h"
#include "NDS.h"
#include "GPU.h"
#include "OpenGLSupport.h"
#include "shaders.h"

extern bool enable_opengl;
extern bool using_opengl;
extern bool refresh_opengl;

static bool initialized_glsm;
static GLuint screen_shader[3];
static GLuint shader[3];
static GLuint screen_framebuffer_texture;
static float screen_vertices[2 * 3 * 2 * 4];
static GLuint vao, vbo;

struct
{
   GLfloat uScreenSize[2];
   u32 u3DScale;
   u32 uFilterMode;
   GLint cursorPos[4];

} GL_ShaderConfig;
static GLuint ubo;

static bool setup_opengl(void)
{
   GPU::InitRenderer(true);

   if (!OpenGL::BuildShaderProgram(screen_vertex_shader, screen_fragment_shader, screen_shader, "ScreenShader"))
      return false;

   if (!OpenGL::BuildShaderProgram(vertex_shader, fragment_shader, shader, "AccelShader"))
      return false;

   glBindAttribLocation(shader[2], 0, "vPosition");
   glBindAttribLocation(shader[2], 1, "vTexcoord");
   glBindFragDataLocation(shader[2], 0, "oColor");

   if (!OpenGL::LinkShaderProgram(shader))
      return false;

   GLuint uni_id;

   uni_id = glGetUniformBlockIndex(shader[2], "uConfig");
   glUniformBlockBinding(shader[2], uni_id, 16);

   glUseProgram(shader[2]);
   uni_id = glGetUniformLocation(shader[2], "ScreenTex");
   glUniform1i(uni_id, 0);
   uni_id = glGetUniformLocation(shader[2], "_3DTex");
   glUniform1i(uni_id, 1);

   memset(&GL_ShaderConfig, 0, sizeof(GL_ShaderConfig));

   glGenBuffers(1, &ubo);
   glBindBuffer(GL_UNIFORM_BUFFER, ubo);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(GL_ShaderConfig), &GL_ShaderConfig, GL_STATIC_DRAW);
   glBindBufferBase(GL_UNIFORM_BUFFER, 16, ubo);

   glGenBuffers(1, &vbo);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), NULL, GL_STATIC_DRAW);

   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);
   glEnableVertexAttribArray(0); // position
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*4, (void*)(0));
   glEnableVertexAttribArray(1); // texcoord
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*4, (void*)(2*4));

   glGenTextures(1, &screen_framebuffer_texture);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, screen_framebuffer_texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, 256*3 + 1, 192*2, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, NULL);

   refresh_opengl = true;

   return true;
}

static void context_reset(void)
{
   if(using_opengl)
      GPU::DeInitRenderer();

   glsm_ctl(GLSM_CTL_STATE_CONTEXT_RESET, NULL);

   if (!glsm_ctl(GLSM_CTL_STATE_SETUP, NULL))
      return;

   glsm_ctl(GLSM_CTL_STATE_BIND, NULL);
   setup_opengl();

   if(using_opengl)
      GPU::InitRenderer(true);
   glsm_ctl(GLSM_CTL_STATE_UNBIND, NULL);

   initialized_glsm = true;
   using_opengl = true;
}

static void context_destroy(void)
{
   glsm_ctl(GLSM_CTL_STATE_BIND, NULL);
   glDeleteTextures(1, &screen_framebuffer_texture);

   glDeleteVertexArrays(1, &vao);
   glDeleteBuffers(1, &vbo);

   OpenGL::DeleteShaderProgram(shader);
   glsm_ctl(GLSM_CTL_STATE_UNBIND, NULL);

   initialized_glsm = false;
}

static bool context_framebuffer_lock(void *data)
{
    return false;
}

bool initialize_opengl()
{
   glsm_ctx_params_t params = {0};

   // melonds wants an opengl 3.1 context, so glcore is required for mesa compatibility
   params.context_type     = RETRO_HW_CONTEXT_OPENGL_CORE;
   params.major            = 3;
   params.minor            = 1;
   params.context_reset    = context_reset;
   params.context_destroy  = context_destroy;
   params.environ_cb       = environ_cb;
   params.stencil          = false;
   params.framebuffer_lock = context_framebuffer_lock;

   if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params))
   {
      log_cb(RETRO_LOG_ERROR, "Could not setup opengl context, falling back to software rasterization.\n");
      return false;
   }

   return true;
}

void deinitialize_opengl_renderer(void)
{
   GPU::DeInitRenderer();
   GPU::InitRenderer(false);
}

void setup_opengl_frame_state(void)
{
   refresh_opengl = false;

   GPU::SetRenderSettings(true, video_settings);

   GL_ShaderConfig.uScreenSize[0] = screen_layout_data.buffer_width;
   GL_ShaderConfig.uScreenSize[1] = screen_layout_data.buffer_height;
   GL_ShaderConfig.u3DScale = video_settings.GL_ScaleFactor;
   GL_ShaderConfig.cursorPos[0] = -1;
   GL_ShaderConfig.cursorPos[1] = -1;
   GL_ShaderConfig.cursorPos[2] = -1;
   GL_ShaderConfig.cursorPos[3] = -1;

   glBindBuffer(GL_UNIFORM_BUFFER, ubo);
   void* unibuf = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
   if (unibuf) memcpy(unibuf, &GL_ShaderConfig, sizeof(GL_ShaderConfig));
   glUnmapBuffer(GL_UNIFORM_BUFFER);

   float screen_width = (float)screen_layout_data.screen_width;
   float screen_height = (float)screen_layout_data.screen_height;

   float top_screen_x = 0.0f;
   float top_screen_y = 0.0f;
   float top_screen_scale = 1.0f;

   float bottom_screen_x = 0.0f;
   float bottom_screen_y = 0.0f;
   float bottom_screen_scale = 1.0f;

   switch (screen_layout_data.displayed_layout)
   {
      case ScreenLayout::TopBottom:
         bottom_screen_y = screen_height;
         break;
      case ScreenLayout::BottomTop:
         top_screen_y = screen_height;
         break;
      case ScreenLayout::LeftRight:
         bottom_screen_x = screen_width;
         break;
      case ScreenLayout::RightLeft:
         top_screen_x = screen_width;
         break;
      case ScreenLayout::TopOnly:
         bottom_screen_y = screen_height; // Meh, let's just hide it
         break;
      case ScreenLayout::BottomOnly:
         top_screen_y = screen_height; // ditto
         break;
      case ScreenLayout::HybridTop:
         top_screen_scale = (float)screen_layout_data.hybrid_ratio;

         bottom_screen_x = screen_width * screen_layout_data.hybrid_ratio;
         bottom_screen_y = screen_height * (screen_layout_data.hybrid_ratio - 1);

         break;
      case ScreenLayout::HybridBottom:
         bottom_screen_scale = (float)screen_layout_data.hybrid_ratio;

         top_screen_x = screen_width * screen_layout_data.hybrid_ratio;
         top_screen_y = screen_height * (screen_layout_data.hybrid_ratio - 1);

         break;
   }

   #define SETVERTEX(i, x, y, t_x, t_y) \
      screen_vertices[(4 * i) + 0] = x; \
      screen_vertices[(4 * i) + 1] = y; \
      screen_vertices[(4 * i) + 2] = t_x; \
      screen_vertices[(4 * i) + 3] = t_y;

   // top screen
   SETVERTEX(0, top_screen_x, top_screen_y, 0.0f, 0.0f); // top left
   SETVERTEX(1, top_screen_x, top_screen_y + screen_height * top_screen_scale, 0.0f, VIDEO_HEIGHT); // bottom left
   SETVERTEX(2, top_screen_x + screen_width * top_screen_scale, top_screen_y + screen_height * top_screen_scale, VIDEO_WIDTH,  VIDEO_HEIGHT); // bottom right
   SETVERTEX(3, top_screen_x, top_screen_y, 0.0f, 0.0f); // top left
   SETVERTEX(4, top_screen_x + screen_width * top_screen_scale, top_screen_y, VIDEO_WIDTH, 0.0f); // top right
   SETVERTEX(5, top_screen_x + screen_width * top_screen_scale, top_screen_y + screen_height * top_screen_scale, VIDEO_WIDTH,  VIDEO_HEIGHT); // bottom right

   // bottom screen
   SETVERTEX(6, bottom_screen_x, bottom_screen_y, 0.0f, VIDEO_HEIGHT); // top left
   SETVERTEX(7, bottom_screen_x, bottom_screen_y + screen_height * bottom_screen_scale, 0.0f, VIDEO_HEIGHT * 2.0f); // bottom left
   SETVERTEX(8, bottom_screen_x + screen_width * bottom_screen_scale, bottom_screen_y + screen_height * bottom_screen_scale, VIDEO_WIDTH,  VIDEO_HEIGHT * 2.0f); // bottom right
   SETVERTEX(9, bottom_screen_x, bottom_screen_y, 0.0f, VIDEO_HEIGHT); // top left
   SETVERTEX(10, bottom_screen_x + screen_width * bottom_screen_scale, bottom_screen_y, VIDEO_WIDTH, VIDEO_HEIGHT); // top right
   SETVERTEX(11, bottom_screen_x + screen_width * bottom_screen_scale, bottom_screen_y + screen_height * bottom_screen_scale, VIDEO_WIDTH, VIDEO_HEIGHT * 2.0f); // bottom right

   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(screen_vertices), screen_vertices);
}

void render_opengl_frame(bool sw)
{
   glsm_ctl(GLSM_CTL_STATE_BIND, NULL);

   int frontbuf = GPU::FrontBuffer;
   bool virtual_cursor = cursor_enabled(&input_state);

   glBindFramebuffer(GL_FRAMEBUFFER, glsm_get_current_framebuffer());

   if(refresh_opengl) setup_opengl_frame_state();

   if(virtual_cursor)
   {
      GL_ShaderConfig.cursorPos[0] = input_state.touch_x - CURSOR_SIZE;
      GL_ShaderConfig.cursorPos[1] = input_state.touch_y - CURSOR_SIZE;
      GL_ShaderConfig.cursorPos[2] = input_state.touch_x + CURSOR_SIZE;
      GL_ShaderConfig.cursorPos[3] = input_state.touch_y + CURSOR_SIZE;

      glBindBuffer(GL_UNIFORM_BUFFER, ubo);
      void* unibuf = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
      if (unibuf) memcpy(unibuf, &GL_ShaderConfig, sizeof(GL_ShaderConfig));
      glUnmapBuffer(GL_UNIFORM_BUFFER);
   }

   glDisable(GL_DEPTH_TEST);
   glDisable(GL_STENCIL_TEST);
   glDisable(GL_BLEND);
   glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   glViewport(0, 0, screen_layout_data.buffer_width, screen_layout_data.buffer_height);

   OpenGL::UseShaderProgram(sw ? screen_shader : shader);

   glClearColor(0, 0, 0, 1);
   glClear(GL_COLOR_BUFFER_BIT);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, screen_framebuffer_texture);

   if (sw)
   {
      if (GPU::Framebuffer[frontbuf][0] && GPU::Framebuffer[frontbuf][1])
      {
         glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 192, GL_RGBA_INTEGER,
                        GL_UNSIGNED_BYTE, GPU::Framebuffer[frontbuf][0]);
         glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 192, 256, 192, GL_RGBA_INTEGER,
                        GL_UNSIGNED_BYTE, GPU::Framebuffer[frontbuf][1]);
      }
   }
   else
   {
      if (GPU::Framebuffer[frontbuf][0] && GPU::Framebuffer[frontbuf][1])
      {
         glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256*3 + 1, 192, GL_RGBA_INTEGER,
                        GL_UNSIGNED_BYTE, GPU::Framebuffer[frontbuf][0]);
         glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 192, 256*3 + 1, 192, GL_RGBA_INTEGER,
                        GL_UNSIGNED_BYTE, GPU::Framebuffer[frontbuf][1]);
      }
   }

   glActiveTexture(GL_TEXTURE1);
   if(!sw) GPU3D::GLRenderer::SetupAccelFrame();

   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBindVertexArray(vao);
   glDrawArrays(GL_TRIANGLES, 0, 4*3);

   glFlush();

   glsm_ctl(GLSM_CTL_STATE_UNBIND, NULL);

   video_cb(RETRO_HW_FRAME_BUFFER_VALID, screen_layout_data.buffer_width, screen_layout_data.buffer_height, 0);
}
