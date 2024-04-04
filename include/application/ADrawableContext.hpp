#pragma once

#include "types.h"
#include "application/ACamera.hpp"

struct CDrawable;

class ADrawableContext {
	shared_vector<CDrawable> mDrawables;

public:
	ADrawableContext();
	~ADrawableContext();

	void LoadDrawable(std::filesystem::path filePath);
};