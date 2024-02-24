#version 460

out vec4 oPixelColor;

layout (std140, binding=0) uniform uSharedData {
  mat4 mProj;
  mat4 mView;
  mat4 mModel;
};

uniform vec4 uBaseColor = vec4(0.0, 0.0, 0.0, 1.0);

void main() {
  oPixelColor = uBaseColor;
}