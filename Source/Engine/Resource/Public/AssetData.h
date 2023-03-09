#pragma once
#include "Core/Public/Container.h"
#include "Core/Public/String.h"
#include "Core/Public/Math/Math.h"
#include "Core/Public/TypeDefine.h"

struct FVertex {
	Math::FVector3 Position;
	Math::FVector2 UV;
	Math::FVector3 Normal;
	Math::FVector3 Tangent;
};

typedef uint32 IndexType;

struct SPrimitiveData {
	TVector<FVertex> Vertices;
	TVector<uint32>  Indices;
	TVector<String>  Textures;
};