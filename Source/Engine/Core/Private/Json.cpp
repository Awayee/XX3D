#include <writer.h>
#include <prettywriter.h>
#include <fstream>
#include "Core/Public/macro.h"
#include "Core/Public/Json.h"
namespace Json {
	bool ReadFile(const char* file, Document& doc) {
		std::ifstream in(file);
		if (!in.is_open()) {
			LOG("Failed to load json file: %s", file);
			return false;
		}
		std::string fileContent{ std::istream_iterator<char>(in), std::istream_iterator<char>() };
		in.close();
		return !doc.Parse(fileContent.c_str()).HasParseError();
	}

	bool WriteFile(const char* file, const Document& doc, bool pretty) {
		std::ofstream out(file);

		if (out.fail()) {
			LOG("Failed to write json file: %s", file);
			return false;
		}

		rapidjson::StringBuffer buffer;
		if(pretty) {
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
			doc.Accept(writer);
			auto content = buffer.GetString();
			out << content;
		}
		else {
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			doc.Accept(writer);
			auto content = buffer.GetString();
			out << content;
		}
		out.close();
		return true;
	}

	void AddString(Value& obj, const char* name, const std::string& str, Document::AllocatorType& a) {
		Value v;
		v.SetString(str.c_str(), str.size());
		Document::StringRefType nameStr(name);
		obj.AddMember(nameStr, v, a);
	}

	void LoadVector3(const Value& obj, Math::FVector3& vector) {
		vector.x = obj[0].GetFloat();
		vector.y = obj[1].GetFloat();
		vector.z = obj[2].GetFloat();
	}

	void AddVector3(Value& obj, const char* name, Math::FVector3& vector, Document::AllocatorType& a) {
		Value v(Type::kArrayType);
		v.PushBack(vector.x, a);
		v.PushBack(vector.y, a);
		v.PushBack(vector.z, a);
		Document::StringRefType nameStr(name);
		obj.AddMember(nameStr, v, a);
	}


}
