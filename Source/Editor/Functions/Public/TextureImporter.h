#pragma once
#include "Asset/Public/TextureAsset.h"
#include <stb_image.h>

class TextureImporter {
private:
	Asset::TextureAsset* m_Asset;
	XString m_SaveFile;//relative path
public:
	TextureImporter(Asset::TextureAsset* asset, const char* saveFile): m_Asset(asset), m_SaveFile(saveFile){}
	//import from external files
	bool Import(const char* fullPath);
	bool Save();
};