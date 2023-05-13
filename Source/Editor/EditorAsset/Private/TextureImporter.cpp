#include "EditorAsset/Public/TextureImporter.h"
#include "Core/Public/macro.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool TextureImporter::Import(const char* fullPath) {
	int width, height, channels;
	constexpr int desiredChannels = STBI_rgb_alpha;
	uint8* pixels = stbi_load(fullPath, &width, &height, &channels, desiredChannels);
	if(!pixels) {
		LOG("loaded image is empty!");
		return false;
	}
	m_Asset->Width = width;
	m_Asset->Height = height;
	m_Asset->Channels = desiredChannels;
	uint64 byteSize = width * height * desiredChannels;
	m_Asset->Pixels.resize(byteSize);
	memcpy(m_Asset->Pixels.data(), pixels, byteSize);
	stbi_image_free(pixels);
	return true;
}

bool TextureImporter::Save() {
	return m_Asset->Save(m_SaveFile.c_str());
}
