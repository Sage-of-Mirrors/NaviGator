#pragma once

#include "types.h"

namespace RDR1Util {
	struct RDR1Track {
		std::string name;
		std::vector<glm::vec3> points;
	};

	void ExtractTrainPoints(std::filesystem::path wsiPath);
}
