#include "Asset/Public/LevelAsset.h"
#include "Core/Public/Json.h"

namespace Engine {

	bool ALevelAsset::Load(File::RFile& in) {
		Json::Document doc;
		if (!Json::ReadFile(in, doc)) {
			return false;
		}
		if (doc.HasMember("Camera")) {
			const rapidjson::Value& cameraParam = doc["Camera"].GetObject();
			Json::LoadFloatArray(cameraParam["Eye"], CameraParam.Eye.Data(), 3);
			Json::LoadFloatArray(cameraParam["At"], CameraParam.At.Data(), 3);
			Json::LoadFloatArray(cameraParam["Up"], CameraParam.Up.Data(), 3);
			CameraParam.Near = cameraParam["Near"].GetFloat();
			CameraParam.Far = cameraParam["Far"].GetFloat();
			CameraParam.Fov = cameraParam["Fov"].GetFloat();
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
		return true;
	}

	bool ALevelAsset::Save(File::WFile& out) {
		Json::Document doc;
		doc.SetObject();
		// camera
		Json::Value cameraVal(Json::Type::kObjectType);
		auto& a = doc.GetAllocator();
		Json::AddFloatArray(cameraVal, "Eye", CameraParam.Eye.Data(), 3, a);
		Json::AddFloatArray(cameraVal, "At", CameraParam.At.Data(), 3, a);
		Json::AddFloatArray(cameraVal, "Up", CameraParam.Up.Data(), 3, a);
		cameraVal.AddMember("Near", CameraParam.Near, a);
		cameraVal.AddMember("Far", CameraParam.Far, a);
		cameraVal.AddMember("Fov", CameraParam.Fov, a);
		doc.AddMember("Camera", cameraVal, a);
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
		return Json::WriteFile(out, doc);
	}

}