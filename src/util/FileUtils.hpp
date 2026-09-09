#pragma once
#include <string>
namespace FileUtils {
    std::string getDefaultOutputDir();
    std::string generateFilename();
    bool ensureDir(const std::string& p);
    bool hasEnoughSpace(const std::string& dir, uint64_t needBytes);
}
