#pragma once
#include "Asset/Public/MeshAsset.h"
#include "Asset/Public/TextureAsset.h"

bool ImportMeshAsset(const XString& srcFile, const XString& dstFile);

bool ImportTexture2DAsset(const XString& srcFile, const XString& dstFile, int downsize, Asset::ETextureCompressMode compressMode);

bool ImportTextureCubeAsset(TConstArrayView<XString> srcFiles, const XString& dstFile, int downsize, Asset::ETextureCompressMode compressMode);