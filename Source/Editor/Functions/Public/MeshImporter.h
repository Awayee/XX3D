#pragma once
#include "Asset/Public/MeshAsset.h"

class MeshImporter {
private:
	Engine::AMeshAsset* m_Asset;
	XXString m_SaveFile;//relative path
public:
	MeshImporter(Engine::AMeshAsset* asset, const char* saveFile);
	//import from external files
	bool Import(const char* fullPath);
	bool ImportGLB(const char* file);
	bool ImportFBX(const char* file);
	bool Save();
};