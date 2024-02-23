#version 460

in vec3 aFragPos;
in vec3 aNormal;

out vec4 oPixelColor;

struct sLight {
  vec4 mPos;
  vec4 mColor;
  float mAtten;
  float mAmbCoeff;
  float d1;
  float d2;
};

layout (std140, binding=0) uniform uSharedData {
  mat4 mProj;
  mat4 mView;
  mat4 mModel;
};

layout (std140, binding=1) uniform uLitSimpleData {
  sLight mLight;
  vec4 mAmbColor;
  vec4 mViewPos;
};

void main() {
  // Calculate ambient color
  vec3 ambColor = mAmbColor.xyz * mLight.mAmbCoeff;
  
  // Calculate diffuse color
  vec3 norm = normalize(aNormal);
  vec3 lightDir = normalize(mLight.mPos.xyz - aFragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffColor = mLight.mColor.xyz * diff;
  
  // Calculate specular color
  float specular = 0.2;
  
  vec3 viewDir = normalize(mViewPos.xyz - aFragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);
  vec3 specColor = mLight.mColor.xyz * specular * spec;
  
  vec3 result = ambColor + diffColor + specColor;
  oPixelColor = vec4(result.xyz, 1.0);
}