#include <writer.h>
#include <prettywriter.h>
#include "Core/Public/Log.h"
#include "Core/Public/Json.h"
#include "Core/Public/String.h"

namespace Json {
	ValueWriter::ValueWriter(Type type, Document& document):m_Value(rapidjson::Value(type)), m_Document(document) {}

	ValueWriter::ValueWriter(Type type, ValueWriter& rhs):m_Value(rapidjson::Value(type)), m_Document(rhs.m_Document) {}

	ValueWriter::ValueWriter(ValueWriter&& rhs) noexcept: m_Value(MoveTemp(rhs.m_Value)), m_Document(rhs.m_Document) {
	}

	ValueWriter& ValueWriter::operator=(ValueWriter&& rhs) noexcept {
		m_Value = MoveTemp(rhs.m_Value);
		m_Document = MoveTemp(rhs.m_Document);
		return *this;
	}

	void ValueWriter::AddMember(const char* name, ValueWriter& val) {
		m_Value.AddMember(Document::StringRefType(name), val.m_Value, m_Document.GetAllocator());
	}

	void ValueWriter::AddString(const char* name, const XString& str) {
		Value v;
		v.SetString(str.c_str(), (unsigned)str.size(), m_Document.GetAllocator());
		m_Value.AddMember(Document::StringRefType(name), v, m_Document.GetAllocator());
	}

	void ValueWriter::AddFloatArray(const char* name, const float* pData, uint32 count) {
		Value arrayVal(Type::kArrayType);
		for (uint32 i = 0; i < count; ++i) {
			arrayVal.PushBack(pData[i], m_Document.GetAllocator());
		}
		m_Value.AddMember(Document::StringRefType(name), arrayVal, m_Document.GetAllocator());
	}

	void ValueWriter::PushBack(ValueWriter& val) {
		CHECK(m_Document == val.m_Document);
		m_Value.PushBack(val.m_Value, m_Document.GetAllocator());
	}

	void ValueWriter::Write(const char* name) {
		m_Document.AddMember(Document::StringRefType(name), m_Value, m_Document.GetAllocator());
	}

	bool ReadFile(const char* file, Document& doc, bool isBinary) {
		if (File::ReadFileWithSize f(file, isBinary); f.IsOpen()) {
			XString content; content.resize(f.ByteSize());
			f.Read(content.data(), f.ByteSize());
			auto& parse = doc.Parse(content.c_str());
			if(parse.HasParseError()) {
				LOG_WARNING("Json::ReadFile has parse error: %d", parse.GetParseError());
				return false;
			}
			return true;
		}
		return false;
	}

	bool WriteFile(const char* file, const Document& doc, bool isBinary, bool pretty) {
		if(File::WriteFile f(file, isBinary); f.IsOpen()) {
			rapidjson::StringBuffer buffer;
			if(pretty) {
				rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
				doc.Accept(writer);
				f.Write(buffer.GetString(), (uint32)buffer.GetSize());
			}
			else {
				rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
				doc.Accept(writer);
				f.Write(buffer.GetString(), (uint32)buffer.GetSize());
			}
			return true;
		}
		LOG_WARNING("Json::WriteFile failed to write file: %s", file);
		return false;

	}

	void AddStringMember(Document& doc, const char* name, const XString& str, Document::AllocatorType& a) {
		Value v;
		v.SetString(str.c_str(), str.size(), a);
		Document::StringRefType nameStr(name);
		doc.AddMember(nameStr, v, a);
	}

	void AddString(Value& obj, const char* name, const XString& str, Document::AllocatorType& a) {
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
