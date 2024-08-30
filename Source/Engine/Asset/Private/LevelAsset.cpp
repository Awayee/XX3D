#include "Asset/Public/LevelAsset.h"
#include "Core/Public/Json.h"

namespace Asset {

	bool LevelAsset::Load(File::RFile& in) {
		Json::Document doc;
		if (!Json::ReadFile(in, doc)) {
			return false;
		}
		if (doc.HasMember("Camera")) {
			const rapidjson::Value& cameraParam = doc["Camera"].GetObject();
			Json::LoadFloatArray(cameraParam["Eye"], CameraData.Eye.Data(), 3);
			Json::LoadFloatArray(cameraParam["At"], CameraData.At.Data(), 3);
			Json::LoadFloatArray(cameraParam["Up"], CameraData.Up.Data(), 3);
			CameraData.ProjType = cameraParam["ProjType"].GetInt();
			CameraData.Near = cameraParam["Near"].GetFloat();
			CameraData.Far = cameraParam["Far"].GetFloat();
			CameraData.Fov = cameraParam["Fov"].GetFloat();
			CameraData.ViewSize = cameraParam["ViewSize"].GetFloat();
		}
		if (doc.HasMember("DirectionalLight")) {
			const rapidjson::Value& directionalLightParam = doc["DirectionalLight"].GetObject();
			Json::LoadFloatArray(directionalLightParam["Rotation"], DirectionalLightData.Rotation.Data(), 3);
			Json::LoadFloatArray(directionalLightParam["Color"], DirectionalLightData.Color.Data(), 3);
		}
		if (doc.HasMember("Meshes")) {
			const Json::Value& objects = doc["Meshes"];
			Meshes.Resize(objects.Size());
			for (uint32 i = 0; i < objects.Size(); ++i) {
				const Json::Value& meshVal = objects[i].GetObject();
				Meshes[i].Name = meshVal["Name"].GetString();
				Meshes[i].File = meshVal["File"].GetString();
				Json::LoadFloatArray(meshVal["Position"], Meshes[i].Position.Data(), 3);
				Json::LoadFloatArray(meshVal["Scale"], Meshes[i].Scale.Data(), 3);
				Json::LoadFloatArray(meshVal["Rotation"], Meshes[i].Rotation.Data(), 3);
			}
		}
		if(doc.HasMember("SkyBox")) {
			SkyBox = doc["SkyBox"].GetString();
		}
		return true;
	}

	bool LevelAsset::Save(File::WFile& out) {
		Json::Document doc;
		doc.SetObject();
		// camera
		Json::Value cameraVal(Json::Type::kObjectType);
		auto& a = doc.GetAllocator();
		Json::AddFloatArray(cameraVal, "Eye", CameraData.Eye.Data(), 3, a);
		Json::AddFloatArray(cameraVal, "At", CameraData.At.Data(), 3, a);
		Json::AddFloatArray(cameraVal, "Up", CameraData.Up.Data(), 3, a);
		cameraVal.AddMember("ProjType", CameraData.ProjType, a);
		cameraVal.AddMember("Near", CameraData.Near, a);
		cameraVal.AddMember("Far", CameraData.Far, a);
		cameraVal.AddMember("Fov", CameraData.Fov, a);
		cameraVal.AddMember("ViewSize", CameraData.ViewSize, a);
		doc.AddMember("Camera", cameraVal, a);
		// directional light
		Json::Value dLightVal(Json::Type::kObjectType);
		Json::AddFloatArray(dLightVal, "Rotation", DirectionalLightData.Rotation.Data(), 3, a);
		Json::AddFloatArray(dLightVal, "Color", DirectionalLightData.Color.Data(), 3, a);
		doc.AddMember("DirectionalLight", dLightVal, a);
		// meshes
		Json::Value objectsVal(Json::Type::kObjectType);
		objectsVal.SetArray();
		for (auto& mesh : Meshes) {
			Json::Value meshVal(Json::Type::kObjectType);
			Json::AddString(meshVal, "Name", mesh.Name, a);
			Json::AddString(meshVal, "File", mesh.File, a);
			Json::AddFloatArray(meshVal, "Position", mesh.Position.Data(), 3, a);
			Json::AddFloatArray(meshVal, "Scale", mesh.Scale.Data(), 3, a);
			Json::AddFloatArray(meshVal, "Rotation", mesh.Rotation.Data(), 3, a);
			objectsVal.PushBack(meshVal, a);
		}
		doc.AddMember("Meshes", objectsVal, a);
		// sky box
		Json::AddStringMember(doc, "SkyBox", SkyBox, a);
		return Json::WriteFile(out, doc);
	}

}