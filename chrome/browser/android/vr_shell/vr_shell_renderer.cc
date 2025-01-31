// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr_shell/vr_shell_renderer.h"

#include <math.h>
#include <algorithm>
#include <string>

#include "chrome/browser/android/vr_shell/vr_gl_util.h"

namespace {

#define RECTANGULAR_TEXTURE_BUFFER(left, right, bottom, top) \
  { left, bottom, left, top, right, bottom, left, top, right, top, right, \
    bottom }

static constexpr float kHalfHeight = 0.5f;
static constexpr float kHalfWidth = 0.5f;
static constexpr float kTextureQuadPosition[18] = {
    -kHalfWidth, kHalfHeight,  0.0f, -kHalfWidth, -kHalfHeight, 0.0f,
    kHalfWidth,  kHalfHeight,  0.0f, -kHalfWidth, -kHalfHeight, 0.0f,
    kHalfWidth,  -kHalfHeight, 0.0f, kHalfWidth,  kHalfHeight,  0.0f};
static constexpr int kPositionDataSize = 3;
// Number of vertices passed to glDrawArrays().
static constexpr int kVerticesNumber = 6;

static constexpr float kTexturedQuadTextureCoordinates[12] =
    RECTANGULAR_TEXTURE_BUFFER(0.0f, 1.0f, 0.0f, 1.0f);

static constexpr int kTextureCoordinateDataSize = 2;

static constexpr float kWebVrVertices[16] = {
  //   x     y    u,   v
    -1.f,  1.f, 0.f, 0.f,
    -1.f, -1.f, 0.f, 1.f,
     1.f, -1.f, 1.f, 1.f,
     1.f,  1.f, 1.f, 0.f };
static constexpr int kWebVrVerticesSize = sizeof(float) * 16;

// Reticle constants
static constexpr float kRingDiameter = 1.0f;
static constexpr float kInnerHole = 0.0f;
static constexpr float kInnerRingEnd = 0.177f;
static constexpr float kInnerRingThickness = 0.14f;
static constexpr float kMidRingEnd = 0.177f;
static constexpr float kMidRingOpacity = 0.22f;
static constexpr float kReticleColor[] = {1.0f, 1.0f, 1.0f, 1.0f};

// Laser constants
static constexpr float kFadeEnd = 0.535;
static constexpr float kFadePoint = 0.5335;
static constexpr float kLaserColor[] = {1.0f, 1.0f, 1.0f, 0.5f};
static constexpr int kLaserDataWidth = 48;
static constexpr int kLaserDataHeight = 1;

// Background constants.
static constexpr float kGroundTileSize = 2.5f;
static constexpr float kGroundMaxSize = 25.0f;
static constexpr float kGroundYPosition = -2.0f;

// Laser texture data, 48x1 RGBA.
// TODO(mthiesse): As we add more resources for VR Shell, we should put them
// in Chrome's resource files.
static const unsigned char kLaserData[] =
    "\xff\xff\xff\x01\xff\xff\xff\x02\xbf\xbf\xbf\x04\xcc\xcc\xcc\x05\xdb\xdb"
    "\xdb\x07\xcc\xcc\xcc\x0a\xd8\xd8\xd8\x0d\xd2\xd2\xd2\x11\xce\xce\xce\x15"
    "\xce\xce\xce\x1a\xce\xce\xce\x1f\xcd\xcd\xcd\x24\xc8\xc8\xc8\x2a\xc9\xc9"
    "\xc9\x2f\xc9\xc9\xc9\x34\xc9\xc9\xc9\x39\xc9\xc9\xc9\x3d\xc8\xc8\xc8\x41"
    "\xcb\xcb\xcb\x44\xee\xee\xee\x87\xfa\xfa\xfa\xc8\xf9\xf9\xf9\xc9\xf9\xf9"
    "\xf9\xc9\xfa\xfa\xfa\xc9\xfa\xfa\xfa\xc9\xf9\xf9\xf9\xc9\xf9\xf9\xf9\xc9"
    "\xfa\xfa\xfa\xc8\xee\xee\xee\x87\xcb\xcb\xcb\x44\xc8\xc8\xc8\x41\xc9\xc9"
    "\xc9\x3d\xc9\xc9\xc9\x39\xc9\xc9\xc9\x34\xc9\xc9\xc9\x2f\xc8\xc8\xc8\x2a"
    "\xcd\xcd\xcd\x24\xce\xce\xce\x1f\xce\xce\xce\x1a\xce\xce\xce\x15\xd2\xd2"
    "\xd2\x11\xd8\xd8\xd8\x0d\xcc\xcc\xcc\x0a\xdb\xdb\xdb\x07\xcc\xcc\xcc\x05"
    "\xbf\xbf\xbf\x04\xff\xff\xff\x02\xff\xff\xff\x01";

#define SHADER(Src) #Src
#define OEIE_SHADER(Src) "#extension GL_OES_EGL_image_external : require\n" #Src
#define VOID_OFFSET(x) reinterpret_cast<void*>(x)

const char* GetShaderSource(vr_shell::ShaderID shader) {
  switch (shader) {
    case vr_shell::ShaderID::TEXTURE_QUAD_VERTEX_SHADER:
    case vr_shell::ShaderID::RETICLE_VERTEX_SHADER:
    case vr_shell::ShaderID::LASER_VERTEX_SHADER:
      return SHADER(
          /* clang-format off */
          uniform mat4 u_ModelViewProjMatrix;
          attribute vec4 a_Position;
          attribute vec2 a_TexCoordinate;
          varying vec2 v_TexCoordinate;

          void main() {
            v_TexCoordinate = a_TexCoordinate;
            gl_Position = u_ModelViewProjMatrix * a_Position;
          }
          /* clang-format on */);
    case vr_shell::ShaderID::BACKGROUND_VERTEX_SHADER:
      return SHADER(
          /* clang-format off */
          uniform mat4 u_ModelViewProjMatrix;
          uniform float u_SceneRadius;
          attribute vec4 a_Position;
          varying vec2 v_GridPosition;

          void main() {
            v_GridPosition = a_Position.xz / u_SceneRadius;
            gl_Position = u_ModelViewProjMatrix * a_Position;
          }
          /* clang-format on */);
    case vr_shell::ShaderID::TEXTURE_QUAD_FRAGMENT_SHADER:
      return OEIE_SHADER(
          /* clang-format off */
          precision highp float;
          uniform samplerExternalOES u_Texture;
          uniform vec4 u_CopyRect;  // rectangle
          varying vec2 v_TexCoordinate;
          uniform lowp vec4 color;
          uniform mediump float opacity;

          void main() {
            vec2 scaledTex =
                vec2(u_CopyRect[0] + v_TexCoordinate.x * u_CopyRect[2],
                     u_CopyRect[1] + v_TexCoordinate.y * u_CopyRect[3]);
            lowp vec4 color = texture2D(u_Texture, scaledTex);
            gl_FragColor = vec4(color.xyz, color.w * opacity);
          }
          /* clang-format on */);
    case vr_shell::ShaderID::WEBVR_VERTEX_SHADER:
      return SHADER(
          /* clang-format off */
          attribute vec4 a_Position;
          varying vec2 v_TexCoordinate;

          void main() {
            // Pack the texcoord into the position to avoid state changes.
            v_TexCoordinate = a_Position.zw;
            gl_Position = vec4(a_Position.xy, 0.0, 1.0);
          }
          /* clang-format on */);
    case vr_shell::ShaderID::WEBVR_FRAGMENT_SHADER:
      return OEIE_SHADER(
          /* clang-format off */
          precision highp float;
          uniform samplerExternalOES u_Texture;
          varying vec2 v_TexCoordinate;

          void main() {
            gl_FragColor = texture2D(u_Texture, v_TexCoordinate);
          }
          /* clang-format on */);
    case vr_shell::ShaderID::RETICLE_FRAGMENT_SHADER:
      return SHADER(
          /* clang-format off */
          varying mediump vec2 v_TexCoordinate;
          uniform lowp vec4 color;
          uniform mediump float ring_diameter;
          uniform mediump float inner_hole;
          uniform mediump float inner_ring_end;
          uniform mediump float inner_ring_thickness;
          uniform mediump float mid_ring_end;
          uniform mediump float mid_ring_opacity;

          void main() {
            mediump float r = length(v_TexCoordinate - vec2(0.5, 0.5));
            mediump float color_radius = inner_ring_end * ring_diameter;
            mediump float color_feather_radius =
                inner_ring_thickness * ring_diameter;
            mediump float hole_radius =
                inner_hole * ring_diameter - color_feather_radius;
            mediump float color1 = clamp(
                1.0 - (r - color_radius) / color_feather_radius, 0.0, 1.0);
            mediump float hole_alpha =
                clamp((r - hole_radius) / color_feather_radius, 0.0, 1.0);

            mediump float black_radius = mid_ring_end * ring_diameter;
            mediump float black_feather =
                1.0 / (ring_diameter * 0.5 - black_radius);
            mediump float black_alpha_factor =
                mid_ring_opacity * (1.0 - (r - black_radius) * black_feather);
            mediump float alpha = clamp(
                min(hole_alpha, max(color1, black_alpha_factor)), 0.0, 1.0);
            lowp vec3 color_rgb = color1 * color.xyz;
            gl_FragColor = vec4(color_rgb, color.w * alpha);
          }
          /* clang-format on */);
    case vr_shell::ShaderID::LASER_FRAGMENT_SHADER:
      return SHADER(
          /* clang-format off */
          varying mediump vec2 v_TexCoordinate;
          uniform sampler2D texture_unit;
          uniform lowp vec4 color;
          uniform mediump float fade_point;
          uniform mediump float fade_end;

          void main() {
            mediump vec2 uv = v_TexCoordinate;
            mediump float front_fade_factor = 1.0 -
                clamp(1.0 - (uv.y - fade_point) / (1.0 - fade_point), 0.0, 1.0);
            mediump float back_fade_factor =
                clamp((uv.y - fade_point) / (fade_end - fade_point), 0.0, 1.0);
            mediump float total_fade = front_fade_factor * back_fade_factor;
            lowp vec4 texture_color = texture2D(texture_unit, uv);
            lowp vec4 final_color = color * texture_color;
            gl_FragColor = vec4(final_color.xyz, final_color.w * total_fade);
          }
          /* clang-format on */);
    case vr_shell::ShaderID::BACKGROUND_FRAGMENT_SHADER:
      return SHADER(
          /* clang-format off */
          precision highp float;
          varying vec2 v_GridPosition;
          uniform vec4 u_CenterColor;
          uniform vec4 u_EdgeColor;

          void main() {
            float edgeColorWeight = clamp(length(v_GridPosition), 0.0, 1.0);
            float centerColorWeight = 1.0 - edgeColorWeight;
            gl_FragColor = u_CenterColor * centerColorWeight +
                u_EdgeColor * edgeColorWeight;
          }
          /* clang-format on */);
    default:
      LOG(ERROR) << "Shader source requested for unknown shader";
      return "";
  }
}

#undef RECTANGULAR_TEXTURE_BUFFER
}  // namespace

namespace vr_shell {

BaseRenderer::BaseRenderer(ShaderID vertex_id, ShaderID fragment_id) {
  std::string error;
  GLuint vertex_shader_handle = CompileShader(
      GL_VERTEX_SHADER, GetShaderSource(vertex_id), error);
  CHECK(vertex_shader_handle) << error;

  GLuint fragment_shader_handle = CompileShader(
      GL_FRAGMENT_SHADER, GetShaderSource(fragment_id), error);
  CHECK(fragment_shader_handle) << error;

  program_handle_ = CreateAndLinkProgram(vertex_shader_handle,
                                         fragment_shader_handle, error);
  CHECK(program_handle_) << error;

  // Once the program is linked the shader objects are no longer needed
  glDeleteShader(vertex_shader_handle);
  glDeleteShader(fragment_shader_handle);

  position_handle_ = glGetAttribLocation(program_handle_, "a_Position");
  tex_coord_handle_ = glGetAttribLocation(program_handle_, "a_TexCoordinate");
}

BaseRenderer::~BaseRenderer() = default;

void BaseRenderer::PrepareToDraw(GLuint view_proj_matrix_handle,
                                 const gvr::Mat4f& view_proj_matrix) {
  glUseProgram(program_handle_);

  // Pass in model view project matrix.
  glUniformMatrix4fv(view_proj_matrix_handle, 1, false,
                     MatrixToGLArray(view_proj_matrix).data());

  // Pass in texture coordinate.
  glVertexAttribPointer(tex_coord_handle_, kTextureCoordinateDataSize,
                        GL_FLOAT, false, 0, kTexturedQuadTextureCoordinates);
  glEnableVertexAttribArray(tex_coord_handle_);

  glVertexAttribPointer(position_handle_, kPositionDataSize, GL_FLOAT, false, 0,
                        kTextureQuadPosition);
  glEnableVertexAttribArray(position_handle_);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

TexturedQuadRenderer::TexturedQuadRenderer()
    : BaseRenderer(TEXTURE_QUAD_VERTEX_SHADER, TEXTURE_QUAD_FRAGMENT_SHADER) {
  model_view_proj_matrix_handle_ =
      glGetUniformLocation(program_handle_, "u_ModelViewProjMatrix");
  tex_uniform_handle_ = glGetUniformLocation(program_handle_, "u_Texture");
  copy_rect_uniform_handle_ =
      glGetUniformLocation(program_handle_, "u_CopyRect");
  opacity_handle_ = glGetUniformLocation(program_handle_, "opacity");
}

void TexturedQuadRenderer::Draw(int texture_data_handle,
                                const gvr::Mat4f& view_proj_matrix,
                                const Rectf& copy_rect,
                                float opacity) {
  PrepareToDraw(model_view_proj_matrix_handle_, view_proj_matrix);

  // Link texture data with texture unit.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_data_handle);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glUniform1i(tex_uniform_handle_, 0);
  glUniform4fv(copy_rect_uniform_handle_, 1, (float*)(&copy_rect));
  glUniform1f(opacity_handle_, opacity);

  glDrawArrays(GL_TRIANGLES, 0, kVerticesNumber);

  glDisableVertexAttribArray(position_handle_);
  glDisableVertexAttribArray(tex_coord_handle_);
}

TexturedQuadRenderer::~TexturedQuadRenderer() = default;

WebVrRenderer::WebVrRenderer() :
    BaseRenderer(WEBVR_VERTEX_SHADER, WEBVR_FRAGMENT_SHADER) {
  tex_uniform_handle_ = glGetUniformLocation(program_handle_, "u_Texture");

  // TODO(bajones): Figure out why this need to be restored.
  GLint old_buffer;
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &old_buffer);

  glGenBuffersARB(1, &vertex_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, kWebVrVerticesSize, kWebVrVertices,
      GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, old_buffer);
}

// Draw the stereo WebVR frame
void WebVrRenderer::Draw(int texture_handle) {
  // TODO(bajones): Figure out why this need to be restored.
  GLint old_buffer;
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &old_buffer);

  glUseProgram(program_handle_);

  // Bind vertex attributes
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

  glEnableVertexAttribArray(position_handle_);

  glVertexAttribPointer(position_handle_, VERTEX_ELEMENTS, GL_FLOAT, false,
      VERTEX_STRIDE, VOID_OFFSET(VERTEX_OFFSET));

  // Bind texture. Ideally this should be a 1:1 pixel copy. (Or even more
  // ideally, a zero copy reuse of the texture.) For now, we're using an
  // undersized render target for WebVR, so GL_LINEAR makes it look slightly
  // less chunky. TODO(klausw): change this to GL_NEAREST once we're doing
  // a 1:1 copy since that should be more efficient.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_handle);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glUniform1i(tex_uniform_handle_, 0);

  // Blit texture to buffer
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  glDisableVertexAttribArray(position_handle_);

  glBindBuffer(GL_ARRAY_BUFFER, old_buffer);
}

// Note that we don't explicitly delete gl objects here, they're deleted
// automatically when we call ShutdownGL, and deleting them here leads to
// segfaults.
WebVrRenderer::~WebVrRenderer() = default;

ReticleRenderer::ReticleRenderer()
    : BaseRenderer(RETICLE_VERTEX_SHADER, RETICLE_FRAGMENT_SHADER) {
  model_view_proj_matrix_handle_ =
      glGetUniformLocation(program_handle_, "u_ModelViewProjMatrix");
  color_handle_ = glGetUniformLocation(program_handle_, "color");
  ring_diameter_handle_ =
      glGetUniformLocation(program_handle_, "ring_diameter");
  inner_hole_handle_ = glGetUniformLocation(program_handle_, "inner_hole");
  inner_ring_end_handle_ =
      glGetUniformLocation(program_handle_, "inner_ring_end");
  inner_ring_thickness_handle_ =
      glGetUniformLocation(program_handle_, "inner_ring_thickness");
  mid_ring_end_handle_ = glGetUniformLocation(program_handle_, "mid_ring_end");
  mid_ring_opacity_handle_ =
      glGetUniformLocation(program_handle_, "mid_ring_opacity");
}

void ReticleRenderer::Draw(const gvr::Mat4f& view_proj_matrix) {
  PrepareToDraw(model_view_proj_matrix_handle_, view_proj_matrix);

  glUniform4f(color_handle_, kReticleColor[0], kReticleColor[1],
              kReticleColor[2], kReticleColor[3]);
  glUniform1f(ring_diameter_handle_, kRingDiameter);
  glUniform1f(inner_hole_handle_, kInnerHole);
  glUniform1f(inner_ring_end_handle_, kInnerRingEnd);
  glUniform1f(inner_ring_thickness_handle_, kInnerRingThickness);
  glUniform1f(mid_ring_end_handle_, kMidRingEnd);
  glUniform1f(mid_ring_opacity_handle_, kMidRingOpacity);

  glDrawArrays(GL_TRIANGLES, 0, kVerticesNumber);

  glDisableVertexAttribArray(position_handle_);
  glDisableVertexAttribArray(tex_coord_handle_);
}

ReticleRenderer::~ReticleRenderer() = default;

LaserRenderer::LaserRenderer()
    : BaseRenderer(LASER_VERTEX_SHADER, LASER_FRAGMENT_SHADER) {
  model_view_proj_matrix_handle_ =
      glGetUniformLocation(program_handle_, "u_ModelViewProjMatrix");
  texture_unit_handle_ =
      glGetUniformLocation(program_handle_, "texture_unit");
  color_handle_ = glGetUniformLocation(program_handle_, "color");
  fade_point_handle_ =
      glGetUniformLocation(program_handle_, "fade_point");
  fade_end_handle_ = glGetUniformLocation(program_handle_, "fade_end");

  glGenTextures(1, &texture_data_handle_);
  glBindTexture(GL_TEXTURE_2D, texture_data_handle_);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kLaserDataWidth, kLaserDataHeight, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, kLaserData);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void LaserRenderer::Draw(const gvr::Mat4f& view_proj_matrix) {
  PrepareToDraw(model_view_proj_matrix_handle_, view_proj_matrix);

  // Link texture data with texture unit.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_data_handle_);

  glUniform1i(texture_unit_handle_, 0);
  glUniform4f(color_handle_, kLaserColor[0], kLaserColor[1], kLaserColor[2],
              kLaserColor[3]);
  glUniform1f(fade_point_handle_, kFadePoint);
  glUniform1f(fade_end_handle_, kFadeEnd);

  glDrawArrays(GL_TRIANGLES, 0, kVerticesNumber);

  glDisableVertexAttribArray(position_handle_);
  glDisableVertexAttribArray(tex_coord_handle_);
}

LaserRenderer::~LaserRenderer() = default;

BackgroundRenderer::BackgroundRenderer()
    : BaseRenderer(BACKGROUND_VERTEX_SHADER, BACKGROUND_FRAGMENT_SHADER) {
  model_view_proj_matrix_handle_ =
      glGetUniformLocation(program_handle_, "u_ModelViewProjMatrix");
  scene_radius_handle_ = glGetUniformLocation(program_handle_, "u_SceneRadius");
  center_color_handle_ = glGetUniformLocation(program_handle_, "u_CenterColor");
  edge_color_handle_ = glGetUniformLocation(program_handle_, "u_EdgeColor");

  // Make ground grid.
  int groundTilesNumber = kGroundMaxSize / kGroundTileSize;
  scene_radius_ = groundTilesNumber * kGroundTileSize * 0.5f;
  int groundLinesNumber = 2 * (groundTilesNumber + 1);
  ground_grid_lines_.resize(groundLinesNumber);

  for (int i = 0; i < groundLinesNumber - 1; i += 2) {
    float position =
        -scene_radius_ + (i / 2) * scene_radius_ * 2.0f / groundTilesNumber;

    // Line parallel to the z axis.
    Line3d& zLine = ground_grid_lines_[i];
    // Line parallel to the x axis.
    Line3d& xLine = ground_grid_lines_[i + 1];

    zLine.start.x = position;
    zLine.start.y = kGroundYPosition;
    zLine.start.z = -scene_radius_;
    zLine.end.x = position;
    zLine.end.y = kGroundYPosition;
    zLine.end.z = scene_radius_;
    xLine.start.x = -scene_radius_;
    xLine.start.y = kGroundYPosition;
    xLine.start.z = position;
    xLine.end.x = scene_radius_;
    xLine.end.y = kGroundYPosition;
    xLine.end.z = position;
  }

  // Make plane for ground and ceilings.
  // clang-format off
  ground_ceiling_plane_positions_ = {
      // First triangle.
      -scene_radius_, 0.0f,  scene_radius_,
       scene_radius_, 0.0f,  scene_radius_,
      -scene_radius_, 0.0f, -scene_radius_,
      // Second triangle.
      -scene_radius_, 0.0f, -scene_radius_,
       scene_radius_, 0.0f,  scene_radius_,
       scene_radius_, 0.0f, -scene_radius_};
  // clang-format on

  // Make the transform to draw the plane either on the ground or the ceiling.
  SetIdentityM(ground_plane_transform_mat_);
  TranslateMRight(ground_plane_transform_mat_, ground_plane_transform_mat_,
                  0.0f, kGroundYPosition - 0.1f, 0.0f);

  SetIdentityM(ceiling_plane_transform_mat_);
  gvr::Quatf rotation_quat = QuatFromAxisAngle({1.0f, 0.0f, 0.0f}, M_PI);
  ceiling_plane_transform_mat_ =
      MatrixMul(ceiling_plane_transform_mat_, QuatToMatrix(rotation_quat));
  TranslateMRight(ceiling_plane_transform_mat_, ceiling_plane_transform_mat_,
                  0.0f, kGroundYPosition - 0.1f, 0.0f);
}

void BackgroundRenderer::Draw(const gvr::Mat4f& view_proj_matrix) {
  glUseProgram(program_handle_);

  // Pass in model view project matrix.
  glUniformMatrix4fv(model_view_proj_matrix_handle_, 1, false,
                     MatrixToGLArray(view_proj_matrix).data());

  // Tell shader the grid size so that it can calculate the fading.
  int groundTilesNumber = kGroundMaxSize / kGroundTileSize;
  glUniform1f(scene_radius_handle_, scene_radius_);

  // Set the edge color to the fog color so that it seems to fade out.
  glUniform4f(edge_color_handle_, kFogBrightness, kFogBrightness,
              kFogBrightness, 1.0f);

  // Draw the ground grid.
  glEnableVertexAttribArray(position_handle_);
  glVertexAttribPointer(position_handle_, kPositionDataSize, GL_FLOAT, false, 0,
                        (float*)ground_grid_lines_.data());
  glUniform4f(center_color_handle_, kGridBrightness, kGridBrightness,
              kGridBrightness, 1.0f);
  int groundVerticesNumber = 4 * (groundTilesNumber + 1);
  glDrawArrays(GL_LINES, 0, groundVerticesNumber);

  // Draw the ground plane.
  gvr::Mat4f transformed_matrix =
      MatrixMul(view_proj_matrix, ground_plane_transform_mat_);
  glUniformMatrix4fv(model_view_proj_matrix_handle_, 1, false,
                     MatrixToGLArray(transformed_matrix).data());
  glUniform4f(center_color_handle_, kGroundCeilingBrightness,
              kGroundCeilingBrightness, kGroundCeilingBrightness, 1.0f);
  glVertexAttribPointer(position_handle_, kPositionDataSize, GL_FLOAT, false, 0,
                        ground_ceiling_plane_positions_.data());
  glDrawArrays(GL_TRIANGLES, 0, kVerticesNumber);

  // Draw the ceiling plane.
  transformed_matrix =
      MatrixMul(view_proj_matrix, ceiling_plane_transform_mat_);
  glUniformMatrix4fv(model_view_proj_matrix_handle_, 1, false,
                     MatrixToGLArray(transformed_matrix).data());
  glDrawArrays(GL_TRIANGLES, 0, kVerticesNumber);

  glDisableVertexAttribArray(position_handle_);
}

BackgroundRenderer::~BackgroundRenderer() = default;

VrShellRenderer::VrShellRenderer()
    : textured_quad_renderer_(new TexturedQuadRenderer),
      webvr_renderer_(new WebVrRenderer),
      reticle_renderer_(new ReticleRenderer),
      laser_renderer_(new LaserRenderer),
      background_renderer_(new BackgroundRenderer) {}

VrShellRenderer::~VrShellRenderer() = default;

}  // namespace vr_shell
