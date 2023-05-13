#pragma once
#include "Asset/Public/MeshAsset.h"

class MeshImporter {
private:
	AMeshAsset* m_Asset;
	String m_SaveFile;
public:
	MeshImporter(AMeshAsset* asset, const char* saveFile);
	//import from external files
	bool Import(const char* fullPath);
	bool ImportGLB(const char* file);
	bool ImportFBX(const char* file);
	bool Save();
};