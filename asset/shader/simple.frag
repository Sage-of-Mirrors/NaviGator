#version 460

out vec4 oPixelColor;

layout (std140, binding=0) uniform uSharedData {
  mat4 mProj;
  mat4 mView;
  mat4 mModel;
};

void main() {
  oPixelColor = vec4(1.0, 0.0, 0.0, 1.0);
}