#include "Asset/Public/MaterialAsset.h"
#include "Core/Public/Json.h"
#include "Core/Public/EnumClass.h"

namespace Asset {

	constexpr bool IS_BINARY = false;

	bool MaterialTemplateAsset::Load(File::PathStr filePath) {
		Json::Document doc(Json::Type::kObjectType);
		if (!Json::ReadFile(filePath, doc, IS_BINARY)) {
			return false;
		}
		if (doc.HasMember("TransparencyMode")) {
			TransparencyMode = (ETransparencyMode)doc["TransparencyMode"].GetUint();
		}
		if (doc.HasMember("Scalars")) {
			const Json::Value& params = doc["Scalars"];
			Scalars.Resize(params.Size());
			for(uint32 i=0; i<params.Size(); ++i) {
				Scalars[i] = params[i].GetFloat();
			}
		}
		if (doc.HasMember("Vectors")) {
			const Json::Value& params = doc["Vectors"];
			const uint32 vectorSize = params.Size() / 4;
			Vectors.Resize(vectorSize);
			for (uint32 i = 0; i < vectorSize; ++i) {
				Vectors[i] = { params[i * 4].GetFloat(),
					params[i * 4 + 1].GetFloat(),
					params[i * 4 + 2].GetFloat(),
					params[i * 4 + 3].GetFloat(), };
			}
		}
		if (doc.HasMember("Textures")) {
			const Json::Value& params = doc["Textures"];
			const uint32 textureSize = params.Size() / 2;
			Textures.Resize(textureSize);
			for (uint32 i = 0; i < textureSize; ++i) {
				Textures[i].Texture = params[i*2].GetString();
				Textures[i].SamplerCode = params[i*2+1].GetUint();
			}
		}

		if (doc.HasMember("Parameters")) {
			const Json::Value& params = doc["Parameters"];
			constexpr uint32 memSize = 5;
			const uint32 paramSize = params.Size() / memSize;
			Parameters.Resize(paramSize);
			for(uint32 i=0; i<paramSize; ++i) {
				Parameters[i].Name = params[i * memSize].GetString();
				Parameters[i].ParamType = (EMaterialParamType)params[i * memSize + 1].GetUint();
				Parameters[i].AssignType = (EMaterialParamAssign)params[i * memSize + 2].GetUint();
				Parameters[i].OutputType = (EMaterialParamOutput)params[i * memSize + 3].GetUint();
				Parameters[i].Offset = (uint16)params[i * memSize + 4].GetUint();
			}
		}
		return true;
	}

	bool MaterialTemplateAsset::Save(File::PathStr filePath) {
		if(Parameters.IsEmpty()) {
			return false;
		}
		Json::Document doc(Json::Type::kObjectType);
		doc.AddMember("TransparencyMode", (uint32)TransparencyMode, doc.GetAllocator());
		if (Scalars.Size()) {
			Json::ValueWriter scalarWriter(Json::Type::kArrayType, doc);
			for(float scalar: Scalars) {
				scalarWriter.PushBack(scalar);
			}
			scalarWriter.Write("Scalars");
		}
		if (Vectors.Size()) {
			Json::ValueWriter vectorWriter(Json::Type::kArrayType, doc);
			for (auto& vector : Vectors) {
				vectorWriter.PushBack(vector.X);
				vectorWriter.PushBack(vector.Y);
				vectorWriter.PushBack(vector.Z);
				vectorWriter.PushBack(vector.W);
			}
			vectorWriter.Write("Vectors");
		}
		if (Textures.Size()) {
			Json::ValueWriter textureWriter(Json::Type::kArrayType, doc);
			for (auto& param : Textures) {
				textureWriter.PushBack(param.Texture);
				textureWriter.PushBack(param.SamplerCode);
			}
			textureWriter.Write("Textures");
		}

		Json::ValueWriter paramsWriter(Json::Type::kArrayType, doc);
		for(const auto& desc: Parameters) {
			paramsWriter.PushBack(desc.Name);
			paramsWriter.PushBack((uint32)desc.ParamType);
			paramsWriter.PushBack((uint32)desc.AssignType);
			paramsWriter.PushBack((uint32)desc.OutputType);
			paramsWriter.PushBack(desc.Offset);
		}
		paramsWriter.Write("Parameters");
		return Json::WriteFile(filePath, doc, false, true);
	}

	void MaterialTemplateAsset::InitDefault(MaterialTemplateAsset& asset) {
		asset.TransparencyMode = ETransparencyMode::Opaque;
		asset.Textures.Reset();
		asset.Scalars = { 0.5f, 0.5f }; // roughness, metallic
		asset.Vectors = {
			{1.0f, 1.0f, 1.0f, 1.0f}, // base color
			{0.0f, 0.0f, 0.0f, 0.0f}, // normal
		};
		asset.Parameters = {
			{"BaseColor", EMaterialParamType::Vector, EMaterialParamAssign::Equal, EMaterialParamOutput::XYZ, 0},
			{"Normal", EMaterialParamType::Vector, EMaterialParamAssign::Add, EMaterialParamOutput::XYZ, 1},
			{"Roughness", EMaterialParamType::Scalar, EMaterialParamAssign::Equal, EMaterialParamOutput::X, 0},
			{"Metallic", EMaterialParamType::Scalar, EMaterialParamAssign::Equal, EMaterialParamOutput::X, 1},
		};
	}

	bool MaterialAsset::Load(File::PathStr path) {
		Json::Document doc(Json::Type::kObjectType);
		if(!Json::ReadFile(path, doc, IS_BINARY)) {
			return false;
		}
		if(doc.HasMember("Template")) {
			Template = doc["Template"].GetString();
		}
		if(doc.HasMember("Scalars")) {
			const Json::Value& values = doc["Scalars"];
			ScalarOverrides.Resize(values.Size());
			for(uint32 i=0; i<values.Size(); ++i) {
				ScalarOverrides[i] = values[i].GetFloat();
			}
		}
		if(doc.HasMember("Vectors")) {
			const Json::Value& values = doc["Vectors"];
			const uint32 valueSize = values.Size() / 4;
			VectorOverrides.Resize(valueSize);
			for(uint32 i=0; i< valueSize; ++i) {
				VectorOverrides[i] = { values[i * 4].GetFloat(),
					values[i * 4 + 1].GetFloat(),
					values[i * 4 + 2].GetFloat(),
					values[i * 4 + 3].GetFloat() };
			}
		}
		if(doc.HasMember("Textures")) {
			const Json::Value& values = doc["Textures"];
			TextureOverrides.Resize(values.Size());
			for(uint32 i=0; i< values.Size(); ++i) {
				TextureOverrides[i] = values[i].GetString();
			}
		}
		return true;
	}

	bool MaterialAsset::Save(File::PathStr path) {
		Json::Document doc(Json::Type::kObjectType);
		Json::AddStringMember(doc, "Template", Template, doc.GetAllocator());
		if (ScalarOverrides.Size()) {
			Json::ValueWriter scalarWriter(Json::Type::kArrayType, doc);
			for(float value: ScalarOverrides) {
				scalarWriter.PushBack(value);
			}
		}
		if (VectorOverrides.Size()) {
			Json::ValueWriter vectorWriter(Json::Type::kArrayType, doc);
			for (auto& value : VectorOverrides) {
				vectorWriter.PushBack(value.X);
				vectorWriter.PushBack(value.Y);
				vectorWriter.PushBack(value.Z);
				vectorWriter.PushBack(value.W);
			}
			vectorWriter.Write("Vectors");
		}
		if (TextureOverrides.Size()) {
			Json::ValueWriter textureWriter(Json::Type::kArrayType, doc);
			for (auto& tex : TextureOverrides) {
				textureWriter.PushBack(tex);
			}
			textureWriter.Write("Textures");
		}
		return Json::WriteFile(path, doc, IS_BINARY);
	}

	bool MaterialAsset::CheckAndFixParameters(const MaterialTemplateAsset& templateAsset) {
		if(ScalarOverrides.Size() == templateAsset.Scalars.Size() &&
			VectorOverrides.Size() == templateAsset.Vectors.Size() &&
			TextureOverrides.Size() == templateAsset.Vectors.Size() ) {
			return true;
		}
		const auto& templateScalars = templateAsset.Scalars;
		if (const uint32 targetSize = ScalarOverrides.Size(); targetSize != templateScalars.Size()) {
			ScalarOverrides.Resize(templateScalars.Size());
			for (uint32 i = targetSize; i < templateScalars.Size(); ++i) {
				ScalarOverrides[i] = templateScalars[i];
			}
		}
		const auto& templateVectors = templateAsset.Vectors;
		if (const uint32 targetSize = VectorOverrides.Size(); targetSize != templateVectors.Size()) {
			VectorOverrides.Resize(templateVectors.Size());
			for (uint32 i = targetSize; i < templateScalars.Size(); ++i) {
				VectorOverrides[i] = templateVectors[i];
			}
		}
		const auto& templateTextures = templateAsset.Textures;
		if (const uint32 targetSize = TextureOverrides.Size(); targetSize != templateTextures.Size()) {
			TextureOverrides.Resize(templateTextures.Size());
			for (uint32 i = targetSize; i < templateTextures.Size(); ++i) {
				TextureOverrides[i] = templateTextures[i].Texture;
			}
		}
		return false;
	}
}
