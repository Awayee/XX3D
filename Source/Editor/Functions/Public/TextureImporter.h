#pragma once
#include "Asset/Public/TextureAsset.h"
#include <stb_image.h>

class TextureImporter {
private:
	Engine::ATextureAsset* m_Asset;
	XXString m_SaveFile;//relative path
public:
	TextureImporter(Engine::ATextureAsset* asset, const char* saveFile): m_Asset(asset), m_SaveFile(saveFile){}
	//import from external files
	bool Import(const char* fullPath);
	bool Save();
};