#pragma once
#include "Asset/Public/MeshAsset.h"

class MeshImporter {
private:
	Asset::MeshAsset* m_Asset;
	XString m_SaveFile;//relative path
public:
	MeshImporter(Asset::MeshAsset* asset, const char* saveFile);
	//import from external files
	bool Import(const char* fullPath);
	bool ImportGLB(const char* file);
	bool ImportFBX(const char* file);
	bool Save();
};