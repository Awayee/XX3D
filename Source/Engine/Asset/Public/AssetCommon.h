#pragma once
#include "Core/Public/Container.h"
#include "Core/Public/String.h"
#include "Math/Public/Vector.h"
#include "Core/Public/TypeDefine.h"

struct AAssetBase {
	virtual void Load(const char* file) = 0;
	virtual void Save(const char* file) = 0;
	virtual ~AAssetBase() = default;
};

struct FVertex {
	Math::FVector3 Position;
	Math::FVector3 Normal;
	Math::FVector3 Tangent;
	Math::FVector2 UV;
};

typedef uint32 IndexType;

struct SPrimitiveData {
	TVector<FVertex> Vertices;
	TVector<uint32>  Indices;
	TVector<String>  Textures;
};

enum class ECompressMode : uint8 {
	NONE,
	LZ4,
};