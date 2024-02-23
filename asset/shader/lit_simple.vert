#version 460

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNrm;

out vec3 aFragPos;
out vec3 aNormal;

layout (std140, binding=0) uniform uSharedData {
  mat4 mProj;
  mat4 mView;
  mat4 mModel;
};

struct sLight {
  vec4 mPos;
  vec4 mColor;
  float mAtten;
  float mAmbCoeff;
  float d1;
  float d2;
};

layout (std140, binding=1) uniform uLitSimpleData {
  sLight mLight;
  vec4 mAmbColor;
  vec4 mViewPos;
};

void main() {
  gl_Position = mProj * mView * mModel * vec4(aPos.xyz, 1.0);
  aFragPos = (mModel * vec4(aPos.xyz, 1.0)).xyz;
  aNormal = aNrm;
}
