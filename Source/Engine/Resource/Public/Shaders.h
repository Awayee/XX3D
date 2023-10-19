#pragma once
#include "Core/Public/File.h"
#include "Core/Public/String.h"

namespace Engine {
	inline void LoadShaderFile(const char* file, TVector<char>& code) {
		char shaderPath[128];
		StrCopy(shaderPath, SHADER_PATH);
		StrAppend(shaderPath, file);
		File::LoadFileCode(shaderPath, code);
	}
}