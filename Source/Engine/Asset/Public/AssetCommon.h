#pragma once
#include "Math/Public/Vector.h"
#include "Core/Public/Defines.h"
#include "Core/Public/File.h"

namespace Asset {	
	struct AssetBase {
		virtual bool Load(File::PathStr filePath) = 0;
		virtual bool Save(File::PathStr filePath) = 0;
		virtual ~AssetBase() = default;
	};

	struct UnknownAsset: public AssetBase {
		bool Load(File::PathStr in) override { return true; }
		bool Save(File::PathStr out) override { return true; }
		~UnknownAsset() override = default;
	};

	struct AssetVertex {
		Math::FVector3 Position;
		Math::FVector3 Normal;
		Math::FVector3 Tangent;
		Math::FVector2 UV;
	};

	typedef uint32 IndexType;
}