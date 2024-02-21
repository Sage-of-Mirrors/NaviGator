#include "ui/UViewport.hpp"

#include <glad/glad.h>
#include <imgui.h>

#include <string>


constexpr int TEX_COLOR = 0;
constexpr int TEX_DEPTH = 1;

constexpr float COLOR_RESET[] = { 0.5f, 0.5f, 0.5f, 1.0f };
constexpr float DEPTH_RESET = 1.0f;


UViewport::UViewport(std::string name) : mViewportName(name), mViewportSize(1, 1) {
    CreateFramebuffer();
}

UViewport::~UViewport() {
    Clear();
}

void UViewport::Clear() {
    glDeleteFramebuffers(1, &mFBO);
    glDeleteTextures(2, mTexIds);
}

void UViewport::CreateFramebuffer() {
    // Generate framebuffer
    glCreateFramebuffers(1, &mFBO);

    // Generate color texture
    glCreateTextures(GL_TEXTURE_2D, 2, mTexIds);
    glTextureStorage2D(mTexIds[TEX_COLOR], 1, GL_RGB8, mViewportSize.x, mViewportSize.y);
    glTextureParameteri(mTexIds[TEX_COLOR], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(mTexIds[TEX_COLOR], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Generate depth texture
    glTextureStorage2D(mTexIds[TEX_DEPTH], 1, GL_DEPTH_COMPONENT32F, mViewportSize.x, mViewportSize.y);

    // Attach textures to framebuffer
    glNamedFramebufferTexture(mFBO, GL_COLOR_ATTACHMENT0, mTexIds[TEX_COLOR], 0);
    glNamedFramebufferTexture(mFBO, GL_DEPTH_ATTACHMENT, mTexIds[TEX_DEPTH], 0);

    // Specify color buffer
    glNamedFramebufferDrawBuffer(mFBO, GL_COLOR_ATTACHMENT0);
}

void UViewport::ResizeViewport() {
    ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();
    ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
    mViewportSize.x = contentRegionMax.x - contentRegionMin.x;
    mViewportSize.y = contentRegionMax.y - contentRegionMin.y;

    ImVec2 windowPos = ImGui::GetCursorScreenPos();
    mViewportPos.x = windowPos.x;
    mViewportPos.y = windowPos.y;

    Clear();
    CreateFramebuffer();
}

void UViewport::RenderUI(float deltaTime) {
    std::string name = mViewportName;

    if (name.empty()) {
        name = "Viewport##" + std::to_string(mTexIds[TEX_COLOR]);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });

// Window begin
    ImGui::Begin(name.c_str(), &bIsOpen, ImGuiWindowFlags_NoTitleBar);

    // This would normally be in an Update() function, but
    // the camera needs access to this ImGui window's input data.
    if (ImGui::IsWindowFocused()) {
        mCamera.Update(deltaTime);
    }

    ResizeViewport();
    ImGui::Image((void*)mTexIds[TEX_COLOR], { mViewportSize.x, mViewportSize.y }, { 0, 1 }, { 1, 0 });

    ImGui::End();
// Window end

    ImGui::PopStyleVar();
}

//void UViewport::RenderScene(AJ3DContext* ctx, float deltaTime) {
//    // Bind FBO
//    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
//    glViewport(0, 0, mViewportSize.x, mViewportSize.y);
//
//    glDepthMask(GL_TRUE);
//    glClearBufferfv(GL_COLOR, 0, COLOR_RESET);
//    glClearBufferfv(GL_DEPTH, 0, &DEPTH_RESET);
//
//    ctx->Render(mCamera, deltaTime);
//
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//}
