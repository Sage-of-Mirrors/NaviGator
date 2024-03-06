#pragma once

#include "types.h"

struct AOptions {
	AOptions();

	std::filesystem::path mLastOpenedDir;

	std::filesystem::path mLastOpenedRailroadDir;
	std::filesystem::path mLastSavedRailroadDir;

	static void Load();
	static void Save();
};

extern AOptions OPTIONS;
