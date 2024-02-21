#include "application/AGatorApplication.hpp"
#include "application/AInput.hpp"
#include "application/AGatorContext.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

#include <string>
#include <iostream>
#include <vector>


AGatorContext* GatorContext = nullptr;

void DealWithGLErrors(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
		return;
	}

	std::cout << "GL CALLBACK: " << message << std::endl;
}

AGatorApplication::AGatorApplication() {
	mWindow = nullptr;
	mContext = nullptr;
}

bool AGatorApplication::Setup() {
	// Initialize GLFW
	if (!glfwInit())
		return false;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	mWindow = glfwCreateWindow(640, 480, "NaviGator", nullptr, nullptr);
	if (mWindow == nullptr) {
		glfwTerminate();
		return false;
	}

	glfwSetKeyCallback(mWindow, AInput::GLFWKeyCallback);
	glfwSetCursorPosCallback(mWindow, AInput::GLFWMousePositionCallback);
	glfwSetMouseButtonCallback(mWindow, AInput::GLFWMouseButtonCallback);
	glfwSetScrollCallback(mWindow, AInput::GLFWMouseScrollCallback);
	glfwSetDropCallback(mWindow, GLFWDropCallback);

	glfwMakeContextCurrent(mWindow);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glClearColor(0.5f, 1.0f, 0.5f, 1.0f);
	glfwSwapInterval(1);

	//glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(DealWithGLErrors, nullptr);

	// Initialize imgui
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

	io.Fonts->AddFontDefault();

	ImFontConfig config;
	config.MergeMode = true;
	config.GlyphMinAdvanceX = 13.0f;

	static const ImWchar icon_ranges[] = { 0xE000 , 0x10fffd , 0 };
	io.Fonts->AddFontFromFileTTF("asset/font/MaterialSymbolsRounded.ttf", 13.0f, &config, icon_ranges);

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
	ImGui_ImplOpenGL3_Init("#version 150");

	// Create viewer context
	mContext = new AGatorContext();
	mContext->OnGLInitialized();

	GatorContext = mContext;
	return true;
}

bool AGatorApplication::Teardown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	
	glfwDestroyWindow(mWindow);
	glfwTerminate();

	delete mContext;

	return true;
}

bool AGatorApplication::Execute(float deltaTime) {
	// Try to make sure we return an error if anything's fucky
	if (mContext == nullptr || mWindow == nullptr || glfwWindowShouldClose(mWindow))
		return false;

	// Update viewer context
	mContext->Update(deltaTime);

	// Begin actual rendering
	glfwMakeContextCurrent(mWindow);
	glfwPollEvents();

	AInput::UpdateInputState();

	int xPos, yPos;
	glfwGetWindowPos(mWindow, &xPos, &yPos);
	mContext->SetAppPosition(xPos, yPos);

	// The context renders both the ImGui elements and the background elements.
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Update buffer size
	int width, height;
	glfwGetFramebufferSize(mWindow, &width, &height);
	glViewport(0, 0, width, height);

	// Clear buffers
	glClearColor(0.353f, 0.294f, 0.647f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render viewer context
	mContext->Render(deltaTime);

	// Render imgui
	ImGui::Render();

	mContext->PostRender(deltaTime);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}

	// Swap buffers
	glfwSwapBuffers(mWindow);

	return true;
}

void GLFWDropCallback(GLFWwindow* window, int count, const char* paths[]) {
	if (GatorContext == nullptr || count <= 0) {
		return;
	}

	GatorContext->OnFileDropped(paths[0]);
}
