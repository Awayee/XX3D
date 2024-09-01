#include "Asset/Public/TextureAsset.h"
#include "Core/Public/File.h"
#include "Core/Public/Log.h"
#include <lz4.h>
#include <zlib.h>

namespace Asset {

	uint32 GetTextureByteSize(ETextureAssetType type, uint32 width, uint32 height) {
		uint32 pixelSize = width * height;
		switch(type) {
		case ETextureAssetType::RGBA8_2D: return pixelSize * 4;
		case ETextureAssetType::RG8_2D: return pixelSize * 2;
		case ETextureAssetType::RGBA8_Cube: return pixelSize * 4 * 6;
		case ETextureAssetType::R8_2D: return pixelSize;
		default:
			LOG_ERROR("Texture type is not supported! %i", type);
		}
	}

	bool TextureAsset::Load(File::RFile& in) {
		in.seekg(0);
		in.read(BYTE_PTR(&Width), sizeof(uint32));
		in.read(BYTE_PTR(&Height), sizeof(uint32));
		in.read(BYTE_PTR(&Type), sizeof(uint8));
		in.read(BYTE_PTR(&CompressMode), sizeof(uint8));
		const uint32 byteSize = GetTextureByteSize(Type, Width, Height);
		Pixels.Resize(byteSize);

		uint32 compressedByteSize;
		in.read(BYTE_PTR(&compressedByteSize), sizeof(uint32));
		TArray<uint8> compressedData(compressedByteSize);
		in.read(BYTE_PTR(compressedData.Data()), compressedByteSize);
		if (ETextureCompressMode::None == CompressMode) {
			Pixels.Swap(compressedData);
		}
		else if (ETextureCompressMode::LZ4 == CompressMode) {
			LZ4_decompress_safe(BYTE_PTR(compressedData.Data()), BYTE_PTR(Pixels.Data()), (int)compressedByteSize, (int)byteSize);
		}
		else if(ETextureCompressMode::Zlib == CompressMode) {
			uLongf originalSize;
			int res = uncompress(Pixels.Data(), &originalSize, (const Bytef*)compressedData.Data(), compressedByteSize);
			if(res != Z_OK) {
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
	bool TextureAsset::Save(File::WFile& out) {

		out.write(CBYTE_PTR(&Width), sizeof(uint32));
		out.write(CBYTE_PTR(&Height), sizeof(uint32));
		out.write(CBYTE_PTR(&Type), sizeof(uint8));
		out.write(CBYTE_PTR(&CompressMode), sizeof(ETextureCompressMode));

		if (ETextureCompressMode::None == CompressMode) {
			out.write(CBYTE_PTR(Pixels.Data()), Pixels.Size());
		}
		else if (ETextureCompressMode::LZ4 == CompressMode) {
			uint64 compressBound = LZ4_compressBound((int)Pixels.Size());
			TArray<char> compressedData(compressBound);
			uint32 compressedSize = LZ4_compress_default(CBYTE_PTR(Pixels.Data()), compressedData.Data(), (int)Pixels.Size(), (int)compressBound);
			compressedData.Resize(compressedSize);
			out.write(CBYTE_PTR(&compressedSize), sizeof(uint32));
			out.write(compressedData.Data(), compressedSize);
		}
		else if (ETextureCompressMode::Zlib == CompressMode) {
			auto dstLen = compressBound(Pixels.Size());
			TArray<Bytef> compressedData(dstLen);
			int res = compress(compressedData.Data(), &dstLen, Pixels.Data(), Pixels.Size());
			if(res != Z_OK) {
				LOG_WARNING("zlib compress failed!");
				return false;
			}
			out.write(CBYTE_PTR(&dstLen), sizeof(uint32));
			out.write((const char*)compressedData.Data(), dstLen);
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
