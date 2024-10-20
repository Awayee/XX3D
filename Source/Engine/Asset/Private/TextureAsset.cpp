#include "Asset/Public/TextureAsset.h"
#include "Core/Public/File.h"
#include "Core/Public/Log.h"
#include <lz4.h>
#include <zlib.h>

namespace Asset {

	uint32 GetTextureByteSize(ETextureAssetType type, uint32 width, uint32 height) {
		uint32 pixelSize = width * height;
		switch(type) {
		case ETextureAssetType::RGBA8Srgb_2D: return pixelSize * 4;
		case ETextureAssetType::RG8_2D: return pixelSize * 2;
		case ETextureAssetType::RGBA8Srgb_Cube: return pixelSize * 4 * 6;
		case ETextureAssetType::R8_2D: return pixelSize;
		default:
			LOG_FATAL("Texture type is not supported! %i", type);
		}
	}

	bool TextureAsset::Load(File::PathStr filePath) {
		File::ReadFile f(filePath, true);
		if(!f.IsOpen()) {
			LOG_WARNING("[TextureAsset::Load] Failed to load file %s", filePath);
			return false;
		}
		f.Read(&Width, sizeof(uint32));
		f.Read(&Height, sizeof(uint32));
		f.Read(&Type, sizeof(uint8));
		f.Read(&CompressMode, sizeof(uint8));
		const uint32 byteSize = GetTextureByteSize(Type, Width, Height);
		Pixels.Resize(byteSize);

		uint32 compressedByteSize;
		f.Read(&compressedByteSize, sizeof(uint32));
		TArray<uint8> compressedData(compressedByteSize);
		f.Read(compressedData.Data(), compressedByteSize);
		if (ETextureCompressMode::None == CompressMode) {
			Pixels.Swap(compressedData);
		}
		else if (ETextureCompressMode::LZ4 == CompressMode) {
			LZ4_decompress_safe((const int8*)compressedData.Data(), (int8*)Pixels.Data(), (int)compressedByteSize, (int)byteSize);
		}
		else if (ETextureCompressMode::Zlib == CompressMode) {
			uLongf originalSize;
			int res = uncompress(Pixels.Data(), &originalSize, (const Bytef*)compressedData.Data(), compressedByteSize);
			if (res != Z_OK) {
				LOG_WARNING("zlib uncompress failed!");
				return false;
			}
		}
		else {
			LOG_INFO("[TextureAssetBaseLoad] Unknown compress mode %u", CompressMode);
			return false;
		}

		return true;
	}

	bool TextureAsset::Save(File::PathStr filePath) {
		File::WriteFile out(filePath, true);
		if (!out.IsOpen()) {
			LOG_WARNING("[TextureAsset::Save] Failed to save file %s", filePath);
			return false;
		}
		out.Write(&Width, sizeof(uint32));
		out.Write(&Height, sizeof(uint32));
		out.Write(&Type, sizeof(uint8));
		out.Write(&CompressMode, sizeof(ETextureCompressMode));

		if (ETextureCompressMode::None == CompressMode) {
			out.Write(Pixels.Data(), Pixels.Size());
		}
		else if (ETextureCompressMode::LZ4 == CompressMode) {
			uint64 compressBound = LZ4_compressBound((int)Pixels.Size());
			TArray<char> compressedData(compressBound);
			uint32 compressedSize = LZ4_compress_default((const int8*)Pixels.Data(), compressedData.Data(), (int)Pixels.Size(), (int)compressBound);
			compressedData.Resize(compressedSize);
			out.Write(&compressedSize, sizeof(uint32));
			out.Write(compressedData.Data(), compressedSize);
		}
		else if (ETextureCompressMode::Zlib == CompressMode) {
			auto dstLen = compressBound(Pixels.Size());
			TArray<Bytef> compressedData(dstLen);
			int res = compress(compressedData.Data(), &dstLen, Pixels.Data(), Pixels.Size());
			if (res != Z_OK) {
				LOG_WARNING("zlib compress failed!");
				return false;
			}
			out.Write(&dstLen, sizeof(uint32));
			out.Write(compressedData.Data(), dstLen);
		}
		else {
			LOG_INFO("[TextureAssetBaseSave] Unknown compress mode %u", CompressMode);
			return false;
		}
		return true;
	}

	TextureAsset::~TextureAsset() {
	}
}
