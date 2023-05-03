#include "Asset/Public/ImageAsset.h"
#include "Core/Public/Json.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define IMAGE_CHANNELS 4
void AImageAsset::Load(const char* file) {
	stbi_load(file, &Width, &Height, &Channels, IMAGE_CHANNELS);
}
void AImageAsset::Save(const char* file) {
}

AImageAsset::~AImageAsset() {
	stbi_image_free(Pixels);
}
