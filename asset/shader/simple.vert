#version 460

layout (location = 0) in vec3 aPos;

layout (std140, binding=0) uniform uSharedData {
  mat4 mProj;
  mat4 mView;
  mat4 mModel;
};

void main() {
  gl_Position = mProj * mView * mModel * vec4(aPos.xyz, 1.0);
}
