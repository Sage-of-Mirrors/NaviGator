#include "application/ADrawableContext.hpp"

#include <librdr3.hpp>
#include <drawable/drawable.hpp>

ADrawableContext::ADrawableContext() {

}

ADrawableContext::~ADrawableContext() {

}

void ADrawableContext::LoadDrawable(std::filesystem::path filePath) {
	ZoneNamed(LoadDrawable, true);

	mDrawables.push_back(librdr3::ImportYdr(filePath.generic_string()));


}
