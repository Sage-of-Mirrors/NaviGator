#pragma once

#include "types.h"

class AApplication {

	virtual bool Execute(float deltaTime) = 0;
public:
	AApplication() {}
	virtual ~AApplication() {}

	void Run();

	virtual bool Setup() = 0;
	virtual bool Teardown() = 0;
};
