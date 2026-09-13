#pragma once
#include <string>
class GJGameLevel;
namespace practiceme {
std::string getLevelKey(GJGameLevel* level);
std::string getLevelDisplayName(GJGameLevel* level);
int getLevelID(GJGameLevel* level);
}
