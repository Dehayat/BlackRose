#pragma once
#pragma once
#include <string>

#include "Animation/AnimationAsset.h"

class AnimationImporter {
private:
	static std::string ReadToEndOfLine(const std::string& fileString, int startPos);
	static std::string ReadToSpace(const std::string& fileString, int startPos);
public:
	static Animation* LoadAnimation(const std::string& fileName);
};