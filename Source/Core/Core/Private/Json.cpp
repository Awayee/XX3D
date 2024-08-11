#include <writer.h>
#include <prettywriter.h>
#include "Core/Public/Log.h"
#include "Core/Public/Json.h"
#include "Core/Public/String.h"

namespace Json {
	bool ReadFile(const char* file, Document& doc) {
		File::RFile in(file, File::EFileMode::Binary);
		if(!in.is_open()) {
			return false;
		}
		bool ok = ReadFile(in, doc);
		in.close();
		return ok;
	}

	bool ReadFile( File::RFile& in, Document& doc) {
		const XString content{ std::istream_iterator<char>(in), std::istream_iterator<char>() };
		return !doc.Parse(content.c_str()).HasParseError();
	}

	bool WriteFile(const char* file, const Document& doc, bool pretty) {
		File::WFile out;
		out.open(file, File::EFileMode::Write);

		if (!out.good()) {
			LOG_INFO("Failed to write json file: %s", file);
			return false;
		}
		bool ok = WriteFile(out, doc, pretty);
		out.close();
		return ok;

	}

	bool WriteFile(File::WFile& out, const Document& doc, bool pretty) {
		rapidjson::StringBuffer buffer;
		if (pretty) {
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
		return true;
	}

	void AddStringMember(Document& doc, const char* name, const std::string& str, Document::AllocatorType& a) {
		Value v;
		v.SetString(str.c_str(), str.size(), a);
		Document::StringRefType nameStr(name);
		doc.AddMember(nameStr, v, a);
	}

	void AddString(Value& obj, const char* name, const std::string& str, Document::AllocatorType& a) {
		Value v;
		v.SetString(str.c_str(), str.size(), a);
		Document::StringRefType nameStr(name);
		obj.AddMember(nameStr, v, a);
	}

	void AddFloatArray(Value& obj, const char* name, const float* pData, uint32 count, Document::AllocatorType& a) {

		Value arrayVal(Type::kArrayType);
		Value val;
		for(uint32 i=0; i<count; ++i) {
			val.SetFloat(pData[i]);
			arrayVal.PushBack(val, a);
		}
		Document::StringRefType nameStr(name);
		obj.AddMember(nameStr, arrayVal, a);
	}

	void LoadFloatArray(const Value& obj, float* pData, uint32 count) {
		for(uint32 i=0; i<count; ++i) {
			pData[i] = obj[i].GetFloat();
		}
	}

}
