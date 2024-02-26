#include "ui/UViewportPicker.hpp"
#include "ubo/common.hpp"
#include "util/fileutil.hpp"

#include <glad/glad.h>

namespace {
    constexpr uint32_t DATA_RESET = 0;
    constexpr float DEPTH_RESET = 1.0f;

    constexpr int TEX_DATA = 0;
    constexpr int TEX_DEPTH = 1;

    uint32_t mWidth;
    uint32_t mHeight;

    uint32_t mFBO = 0;
    uint32_t mTexObjs[2] = { 0, 0 };

    uint32_t mProgram = 0;
    uint32_t mObjectIdUniform = 0;

    void CreateShader() {
        // Compile vertex shader
        std::string vertTxt = UFileUtil::LoadShaderText("picker.vert");
        const char* vertTxtChars = vertTxt.data();

        uint32_t vertHandle = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertHandle, 1, &vertTxtChars, NULL);
        glCompileShader(vertHandle);

        // Compile fragment shader
        std::string fragTxt = UFileUtil::LoadShaderText("picker.frag");
        const char* fragTxtChars = fragTxt.data();

        uint32_t fragHandle = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragHandle, 1, &fragTxtChars, NULL);
        glCompileShader(fragHandle);

        // Generate shader program
        mProgram = glCreateProgram();
        glAttachShader(mProgram, vertHandle);
        glAttachShader(mProgram, fragHandle);
        glLinkProgram(mProgram);

        // Clean up
        glDetachShader(mProgram, vertHandle);
        glDetachShader(mProgram, fragHandle);
        glDeleteShader(vertHandle);
        glDeleteShader(fragHandle);

        UCommonUniformBuffer::LinkShaderToUBO(mProgram);
        mObjectIdUniform = glGetUniformLocation(mProgram, "uObjectId");
    }

    void CreateFramebuffer(uint32_t width, uint32_t height) {
        mWidth = width;
        mHeight = height;

        // Generate framebuffer
        glCreateFramebuffers(1, &mFBO);

        // Generate data texture
        glCreateTextures(GL_TEXTURE_2D, 2, mTexObjs);
        glTextureStorage2D(mTexObjs[TEX_DATA], 1, GL_R32UI, mWidth, mHeight);
        glTextureParameteri(mTexObjs[TEX_DATA], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(mTexObjs[TEX_DATA], GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Generate depth texture
        glTextureStorage2D(mTexObjs[TEX_DEPTH], 1, GL_DEPTH_COMPONENT32F, mWidth, mHeight);

        // Attach textures to framebuffer
        glNamedFramebufferTexture(mFBO, GL_COLOR_ATTACHMENT0, mTexObjs[TEX_DATA], 0);
        glNamedFramebufferTexture(mFBO, GL_DEPTH_ATTACHMENT, mTexObjs[TEX_DEPTH], 0);
    }

    void DeleteFramebuffer() {
        glDeleteFramebuffers(1, &mFBO);
        glDeleteTextures(2, mTexObjs);
    }
}

void UViewportPicker::CreatePicker(uint32_t width, uint32_t height) {
    CreateShader();
    CreateFramebuffer(width, height);
}

void UViewportPicker::ResizePicker(uint32_t width, uint32_t height) {
    DeleteFramebuffer();
    CreateFramebuffer(width, height);
}

void UViewportPicker::DestroyPicker() {
    DeleteFramebuffer();
    glDeleteProgram(mProgram);
}

void UViewportPicker::BindBuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glViewport(0, 0, mWidth, mHeight);

    glDepthMask(true);
    glClearBufferuiv(GL_COLOR, 0, &DATA_RESET);
    glClearBufferfv(GL_DEPTH, 0, &DEPTH_RESET);

    glUseProgram(mProgram);
}

void UViewportPicker::UnbindBuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

void UViewportPicker::SetIdUniform(uint32_t id) {
    glUniform1ui(mObjectIdUniform, id);
}

uint32_t UViewportPicker::Query(uint32_t pX, uint32_t pY) {
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glViewport(0, 0, mWidth, mHeight);

    uint32_t pixelValue = 0;
    glReadPixels(pX, pY, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &pixelValue);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return pixelValue;
}