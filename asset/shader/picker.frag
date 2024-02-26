#version 460

out uint oPixelValue;

layout (std140, binding=0) uniform uSharedData {
  mat4 mProj;
  mat4 mView;
  mat4 mModel;
};

uniform uint uObjectId = 0;

void main() {
  oPixelValue = uObjectId;
}