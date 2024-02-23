#include "util/fileutil.hpp"

#include <fstream>

std::string UFileUtil::LoadShaderText(std::string shaderName) {
    std::filesystem::path shaderPath = std::filesystem::current_path() / "asset" / "shader" / shaderName;
    if (!std::filesystem::exists(shaderPath)) {
        return "";
    }

    std::ifstream shaderFile(shaderPath);

    // From https://stackoverflow.com/a/2912614
    return std::string(std::istreambuf_iterator<char>(shaderFile), std::istreambuf_iterator<char>());
}
