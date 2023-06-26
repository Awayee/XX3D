#pragma once
#include "Core/Public/TVector.h"
#include "Core/Public/String.h"
#include "Math/Public/Vector.h"
#include "Core/Public/TypeDefine.h"
#include "Core/Public/File.h"

#define PARSE_PROJECT_ASSET(f)\
	char f##__s[128]; strcpy(f##__s, PROJECT_ASSETS); strcat(f##__s, f); f=f##__s

#define PARSE_ENGINE_ASSET(f)\
	char f##__s[128]; strcpy(f##__s, ENGINE_ASSETS); strcat(f##__s, f); f=f##__s

#define CBYTE_PTR reinterpret_cast<const char*>
#define BYTE_PTR reinterpret_cast<char*>

struct AAssetBase {

	virtual bool Load(File::Read& in) = 0;
	virtual bool Save(File::Write& out) = 0;
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