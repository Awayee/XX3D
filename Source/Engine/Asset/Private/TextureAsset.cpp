#include "Asset/Public/TextureAsset.h"
#include "lz4.h"
#include "Core/Public/File.h"
#include "Core/Public/Log.h"

namespace Engine {
	enum class ETexCompressMode : uint8 {
		NONE = 0,
		LZ4,
		CXIMAGE
	};

	const ETexCompressMode COMPRESS_MODE = ETexCompressMode::LZ4;

	bool ATextureAsset::Load(File::RFile& in) {
		in.seekg(0);
		in.read(BYTE_PTR(&Width), sizeof(uint32));
		in.read(BYTE_PTR(&Height), sizeof(uint32));
		in.read(BYTE_PTR(&Channels), sizeof(uint8));
		const uint32 byteSize = Width * Height * Channels;
		Pixels.Resize(byteSize);

		ETexCompressMode compressMode;
		in.read(BYTE_PTR(&compressMode), sizeof(ETexCompressMode));
		if (ETexCompressMode::NONE == compressMode) {
			in.read(BYTE_PTR(Pixels.Data()), byteSize);
		}
		else if (ETexCompressMode::LZ4 == compressMode) {
			uint32 compressedByteSize;
			in.read(BYTE_PTR(&compressedByteSize), sizeof(uint32));
			TVector<char> compressedData(compressedByteSize);
			in.read(compressedData.Data(), compressedByteSize);
			LZ4_decompress_safe(compressedData.Data(), BYTE_PTR(Pixels.Data()), (int)compressedByteSize, (int)byteSize);
		}
		else {
			LOG_INFO("[ATextureAsset::Load] Unknown compress mode %u", compressMode);
			return false;
		}

		return true;
	}
	bool ATextureAsset::Save(File::WFile& out) {
		ETexCompressMode compressMode = COMPRESS_MODE;

		out.write(CBYTE_PTR(&Width), sizeof(uint32));
		out.write(CBYTE_PTR(&Height), sizeof(uint32));
		out.write(CBYTE_PTR(&Channels), sizeof(uint8));
		out.write(CBYTE_PTR(&compressMode), sizeof(ETexCompressMode));

		if (ETexCompressMode::NONE == compressMode) {
			out.write(CBYTE_PTR(Pixels.Data()), Pixels.Size());
		}
		else if (ETexCompressMode::LZ4 == compressMode) {
			uint64 compressBound = LZ4_compressBound((int)Pixels.Size());
			TVector<char> compressedData(compressBound);
			uint32 compressedSize = LZ4_compress_default(CBYTE_PTR(Pixels.Data()), compressedData.Data(), (int)Pixels.Size(), (int)compressBound);
			compressedData.Resize(compressedSize);
			out.write(CBYTE_PTR(&compressedSize), sizeof(uint32));
			out.write(compressedData.Data(), compressedSize);
		}
		else {
			LOG_INFO("[ATextureAsset::Save] Unknown compress mode %u", compressMode);
			return false;
		}
		return true;
	}

	ATextureAsset::~ATextureAsset() {
	}
	
}
