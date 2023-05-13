#include "Asset/Public/TextureAsset.h"

#include "lz4.h"
#include "Core/Public/File.h"
#include "Core/Public/macro.h"

enum class ETexCompressMode : uint8 {
	NONE=0,
	LZ4,
	CXIMAGE
};

const ETexCompressMode COMPRESS_MODE = ETexCompressMode::LZ4;

bool ATextureAsset::Load(const char* file) {
	PARSE_PROJECT_ASSET(file);

	File::Read f(file, std::ios::binary);
	if(!f.is_open()) {
		LOG("[ATextureAsset::Load] Failed to open file: %s", file);
		return false;
	}
	f.seekg(0);
	f.read(BYTE_PTR(&Width), sizeof(uint32));
	f.read(BYTE_PTR(&Height), sizeof(uint32));
	f.read(BYTE_PTR(&Channels), sizeof(uint8));
	const uint32 byteSize = Width * Height * Channels;
	Pixels.resize(byteSize);

	ETexCompressMode compressMode;
	f.read(BYTE_PTR(&compressMode), sizeof(ETexCompressMode));
	if(ETexCompressMode::NONE == compressMode) {
		f.read(BYTE_PTR(Pixels.data()), byteSize);
	}
	else if(ETexCompressMode::LZ4 == compressMode) {
		uint32 compressedByteSize;
		f.read(BYTE_PTR(&compressedByteSize), sizeof(uint32));
		TVector<char> compressedData(compressedByteSize);
		f.read(compressedData.data(), compressedByteSize);
		LZ4_decompress_safe(compressedData.data(), BYTE_PTR(Pixels.data()), (int)compressedByteSize, (int)byteSize);
	}
	else {
		LOG("[ATextureAsset::Load] Unknown compress mode %u", compressMode);
		return false;
	}

	return true;
}
bool ATextureAsset::Save(const char* file) {
	PARSE_PROJECT_ASSET(file);
	File::Write f(file, std::ios::binary | std::ios::out);
	if (!f.is_open()) {
		LOG("[ATextureAsset::Save] Failed to open file: %s", file);
		return false;
	}
	ETexCompressMode compressMode = COMPRESS_MODE;

	f.write(CBYTE_PTR(&Width), sizeof(uint32));
	f.write(CBYTE_PTR(&Height), sizeof(uint32));
	f.write(CBYTE_PTR(&Channels), sizeof(uint8));
	f.write(CBYTE_PTR(&compressMode), sizeof(ETexCompressMode));

	if(ETexCompressMode::NONE == compressMode) {
		f.write(CBYTE_PTR(Pixels.data()), Pixels.size());
	}
	else if (ETexCompressMode::LZ4 == compressMode) {
		uint64 compressBound = LZ4_compressBound((int)Pixels.size());
		TVector<char> compressedData(compressBound);
		uint32 compressedSize = LZ4_compress_default(CBYTE_PTR(Pixels.data()), compressedData.data(), (int)Pixels.size(), (int)compressBound);
		compressedData.resize(compressedSize);
		f.write(CBYTE_PTR(&compressedSize), sizeof(uint32));
		f.write(compressedData.data(), compressedSize);
	}
	else {
		LOG("[ATextureAsset::Save] Unknown compress mode %u", compressMode);
		return false;
	}
	return true;
}

ATextureAsset::~ATextureAsset() {
}
