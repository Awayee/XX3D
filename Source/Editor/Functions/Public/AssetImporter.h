#pragma once
#include "Asset/Public/MeshAsset.h"

bool ImportMeshAsset(const XString& srcFile, const XString& dstFile);

bool ImportTexture2DAsset(const XString& srcFile, const XString& dstFile);

bool ImportTextureCubeAsset(TConstArrayView<XString> srcFiles, const XString& dstFile);