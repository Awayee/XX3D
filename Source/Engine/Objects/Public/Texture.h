#pragma once
#include "Asset/Public/TextureAsset.h"
#include "Core/Public/TSingleton.h"

class TextureMgr: public TSingleton<TextureMgr> {
public:

private:
	TextureMgr();
	~TextureMgr();
};