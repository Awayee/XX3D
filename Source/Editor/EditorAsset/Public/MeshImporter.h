#pragma once
#include "Asset/Public/MeshAsset.h"

class AMeshImporter {
private:
	AMeshAsset* m_Asset;
	File::FPath m_Path;
public:
	AMeshImporter(AMeshAsset* asset);
	//import from external files
	bool Import(const char* file);
	bool ImportGLB(const char* file);
	bool ImportFBX(const char* file);
	bool SavePrimitives();
};