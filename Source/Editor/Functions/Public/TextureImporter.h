#pragma once
#include "Asset/Public/TextureAsset.h"
#include <stb_image.h>

class TextureImporter {
private:
	ATextureAsset* m_Asset;
	String m_SaveFile;//relative path
public:
	TextureImporter(ATextureAsset* asset, const char* saveFile): m_Asset(asset), m_SaveFile(saveFile){}
	//import from external files
	bool Import(const char* fullPath);
	bool Save();
};