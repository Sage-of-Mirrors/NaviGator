#pragma once

#include "application/AApplication.hpp"


struct GLFWwindow;
class AGatorContext;

class AGatorApplication : public AApplication {
	GLFWwindow* mWindow;
	AGatorContext* mContext;

	virtual bool Execute(float deltaTime) override;

public:
	AGatorApplication();
	virtual ~AGatorApplication() {}

	virtual bool Setup() override;
	virtual bool Teardown() override;
};

void GLFWDropCallback(GLFWwindow* window, int count, const char* paths[]);
