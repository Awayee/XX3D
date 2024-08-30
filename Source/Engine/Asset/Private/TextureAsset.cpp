#include "Asset/Public/TextureAsset.h"
#include "Asset/Public/TextureAsset.h"
#include "lz4.h"
#include "Core/Public/File.h"
#include "Core/Public/Log.h"

namespace Asset {
	enum class ETexCompressMode : uint8 {
		NONE = 0,
		LZ4,
		CXIMAGE
	};

	uint32 GetTextureByteSize(ETextureAssetType type, uint32 width, uint32 height) {
		uint32 pixelSize = width * height;
		switch(type) {
		case ETextureAssetType::RGBA8_2D: return pixelSize * 4;
		case ETextureAssetType::RGB8_2D: return pixelSize * 3;
		case ETextureAssetType::RG8_2D: return pixelSize * 2;
		case ETextureAssetType::RGBA8_Cube: return pixelSize * 4 * 6;
		case ETextureAssetType::R8_2D: return pixelSize;
		default:
			LOG_ERROR("Texture type is not supported! %i", type);
		}
	}

	const ETexCompressMode COMPRESS_MODE = ETexCompressMode::LZ4;

	bool TextureAsset::Load(File::RFile& in) {
		in.seekg(0);
		in.read(BYTE_PTR(&Width), sizeof(uint32));
		in.read(BYTE_PTR(&Height), sizeof(uint32));
		in.read(BYTE_PTR(&Type), sizeof(uint8));
		const uint32 byteSize = GetTextureByteSize(Type, Width, Height);
		Pixels.Resize(byteSize);

		ETexCompressMode compressMode;
		in.read(BYTE_PTR(&compressMode), sizeof(ETexCompressMode));
		if (ETexCompressMode::NONE == compressMode) {
			in.read(BYTE_PTR(Pixels.Data()), byteSize);
		}
		else if (ETexCompressMode::LZ4 == compressMode) {
			uint32 compressedByteSize;
			in.read(BYTE_PTR(&compressedByteSize), sizeof(uint32));
			TArray<char> compressedData(compressedByteSize);
			in.read(compressedData.Data(), compressedByteSize);
			LZ4_decompress_safe(compressedData.Data(), BYTE_PTR(Pixels.Data()), (int)compressedByteSize, (int)byteSize);
		}
		else {
			LOG_INFO("[TextureAssetBaseLoad] Unknown compress mode %u", compressMode);
			return false;
		}

		return true;
	}
	bool TextureAsset::Save(File::WFile& out) {
		ETexCompressMode compressMode = COMPRESS_MODE;

		out.write(CBYTE_PTR(&Width), sizeof(uint32));
		out.write(CBYTE_PTR(&Height), sizeof(uint32));
		out.write(CBYTE_PTR(&Type), sizeof(uint8));
		out.write(CBYTE_PTR(&compressMode), sizeof(ETexCompressMode));

		if (ETexCompressMode::NONE == compressMode) {
			out.write(CBYTE_PTR(Pixels.Data()), Pixels.Size());
		}
		else if (ETexCompressMode::LZ4 == compressMode) {
			uint64 compressBound = LZ4_compressBound((int)Pixels.Size());
			TArray<char> compressedData(compressBound);
			uint32 compressedSize = LZ4_compress_default(CBYTE_PTR(Pixels.Data()), compressedData.Data(), (int)Pixels.Size(), (int)compressBound);
			compressedData.Resize(compressedSize);
			out.write(CBYTE_PTR(&compressedSize), sizeof(uint32));
			out.write(compressedData.Data(), compressedSize);
		}
		else {
			LOG_INFO("[TextureAssetBaseSave] Unknown compress mode %u", compressMode);
			return false;
		}
		return true;
	}

	TextureAsset::~TextureAsset() {
	}
}
