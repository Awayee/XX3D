#include "Asset/Public/LevelAsset.h"
#include "Core/Public/Json.h"

bool ALevelAsset::Load(File::Read& in) {
	Json::Document doc;
	if(!Json::ReadFile(in, doc)) {
		return false;
	}
	if (doc.HasMember("Camera")) {
		const rapidjson::Value& cameraParam = doc["Camera"].GetObject();
		Json::LoadVector3(cameraParam["Eye"], CameraParam.Eye);
		Json::LoadVector3(cameraParam["At"], CameraParam.At);
		Json::LoadVector3(cameraParam["Up"], CameraParam.Up);
		CameraParam.Near = cameraParam["Near"].GetFloat();
		CameraParam.Far = cameraParam["Far"].GetFloat();
		CameraParam.Fov = cameraParam["Fov"].GetFloat();
	}
	if (doc.HasMember("Meshes")) {
		const Json::Value& objects = doc["Objects"];
		Meshes.Resize(objects.Size());
		for (uint32 i = 0; i < objects.Size(); ++i) {
			const Json::Value& meshVal = objects[i].GetObject();
			Meshes[i].Name = meshVal["Name"].GetString();
			Meshes[i].File = meshVal["File"].GetString();
			Json::LoadVector3(meshVal["Position"], Meshes[i].Position);
			Json::LoadVector3(meshVal["Scale"], Meshes[i].Scale);
			Json::LoadVector3(meshVal["Rotation"], Meshes[i].Euler);
		}
	}
	return true;
}

bool ALevelAsset::Save(File::Write& out) {
	Json::Document doc;
	doc.SetObject();
	// camera
	Json::Value cameraVal;
	auto& a = doc.GetAllocator();
	Json::AddVector3(cameraVal, "Eye", CameraParam.Eye, a);
	Json::AddVector3(cameraVal, "At", CameraParam.At, a);
	Json::AddVector3(cameraVal, "Up", CameraParam.Up, a);
	cameraVal.AddMember("Near", CameraParam.Near, a);
	cameraVal.AddMember("Far", CameraParam.Far, a);
	cameraVal.AddMember("Fov", CameraParam.Fov, a);
	doc.AddMember("Camera", cameraVal, a);
	// meshes
	Json::Value objectsVal;
	objectsVal.SetArray();
	for (auto& mesh : Meshes) {
		Json::Value meshVal;
		Json::AddString(meshVal, "Name", mesh.Name, a);
		Json::AddString(meshVal, "File", mesh.File, a);
		Json::AddVector3(meshVal, "Position", mesh.Position, a);
		Json::AddVector3(meshVal, "Scale", mesh.Scale, a);
		Json::AddVector3(meshVal, "Rotation", mesh.Euler, a);
		objectsVal.PushBack(meshVal, a);
	}
	doc.AddMember("Objects", objectsVal, a);
	return Json::WriteFile(out, doc);
}
