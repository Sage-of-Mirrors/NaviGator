#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <memory>

template<typename T>
using shared_vector = std::vector<std::shared_ptr<T>>;
