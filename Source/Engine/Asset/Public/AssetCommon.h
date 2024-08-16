#pragma once
#include "Core/Public/String.h"
#include "Math/Public/Vector.h"
#include "Core/Public/Defines.h"
#include "Core/Public/File.h"
#include "Core/Public/String.h"

namespace Asset {
	#define PARSE_PROJECT_ASSET(f)\
		char f##__s[128]; StrCopy(f##__s, PROJECT_ASSETS); strcat(f##__s, f); f=f##__s

	#define PARSE_ENGINE_ASSET(f)\
		char f##__s[128]; StrCopy(f##__s, ENGINE_ASSETS); strcat(f##__s, f); f=f##__s

	#define CBYTE_PTR reinterpret_cast<const char*>
	#define BYTE_PTR reinterpret_cast<char*>

	struct AssetBase {

		virtual bool Load(File::RFile& in) = 0;
		virtual bool Save(File::WFile& out) = 0;
		virtual ~AssetBase() = default;
	};

	struct UnknownAsset: public AssetBase {
		bool Load(File::RFile& in) override { return true; }
		bool Save(File::WFile& out) override { return true; }
		~UnknownAsset() override = default;
	};

	struct AssetVertex {
		Math::FVector3 Position;
		Math::FVector3 Normal;
		Math::FVector3 Tangent;
		Math::FVector2 UV;
	};

	typedef uint32 IndexType;

	struct SPrimitiveData {
		TArray<AssetVertex> Vertices;
		TArray<uint32>  Indices;
		TArray<XString>  Textures;
	};
}