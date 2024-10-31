#pragma once
#include "Core/Public/String.h"
#include "AssetCommon.h"
#include "Math/Public/Math.h"
#include "Core/Public/Container.h"

namespace Asset {

	enum class ETransparencyMode : uint8 {
		Opaque = 0,
		Masked,
		Transparency
	};

	enum class EMaterialParamType: uint8 {
		Scalar,
		Vector,
		Texture,
	};

	enum class EMaterialParamAssign : uint8 {
		Equal,
		Add,
		Multiply
	};

	enum class EMaterialParamOutput: uint8 {
		X,
		XY,
		XYZ,
		XYZW,
	};

	struct MaterialTemplateAsset: public AssetBase {
		ETransparencyMode TransparencyMode;
		TArray<float> Scalars;
		TArray<Math::FVector4> Vectors;
		struct TextureParameter {
			XString Texture;
			uint32 SamplerCode;
		};
		TArray<TextureParameter> Textures;// scalar params are treated as components of vectors
		struct ParameterDesc {
			XString Name;
			EMaterialParamType ParamType;
			EMaterialParamAssign AssignType;
			EMaterialParamOutput OutputType;
			uint32 Offset;// Scalar - offset in float; Vector - offset in FVector4
		};
		// witch param is used to lighting in shader  TODO optional for more shading models.
		TArray<ParameterDesc> Parameters;
		bool Load(File::PathStr filePath) override;
		bool Save(File::PathStr filePath) override;
		static void InitDefault(MaterialTemplateAsset& asset);
	};
	struct MaterialAsset : public AssetBase {
		XString Template;
		// (value, override index)
		TArray<Math::FVector4> VectorOverrides;
		TArray<float> ScalarOverrides;
		TArray<XString> TextureOverrides;
		bool Load(File::PathStr path) override;
		bool Save(File::PathStr path) override;
		bool CheckAndFixParameters(const MaterialTemplateAsset& templateAsset);
	};
}
