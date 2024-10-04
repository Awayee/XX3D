#pragma once
#include "Core/Public/File.h"
#include "Core/Public/String.h"
#include <document.h>
namespace Json {
	using Type = rapidjson::Type;
	using Value = rapidjson::Value;
	using Document = rapidjson::Document;

	class ValueWriter {
	public:
		ValueWriter(Type type, Document& document);
		ValueWriter(Type type, ValueWriter& rhs);
		~ValueWriter() = default;
		ValueWriter(const ValueWriter& rhs) = delete;
		ValueWriter(ValueWriter&& rhs) noexcept;
		ValueWriter& operator=(const ValueWriter& rhs) = delete;
		ValueWriter& operator=(ValueWriter&& rhs) noexcept;
		template<typename T>
		void AddMember(const char* name, T val) {
			m_Value.AddMember(Document::StringRefType(name), val, m_Document.GetAllocator());
		}
		void AddMember(const char* name, ValueWriter& val);
		void AddString(const char* name, const XString& str);
		void AddFloatArray(const char* name, const float* pData, uint32 count);
		void PushBack(ValueWriter& val);
		void Write(const char* name);
	private:
		rapidjson::Value m_Value;
		Document& m_Document;
	};

	bool ReadFile(const char* file, Document& doc, bool isBinary);
	bool WriteFile(const char* file, const Document& doc, bool isBinary, bool pretty=true);

	void AddStringMember(Document& doc, const char* name, const XString& str, Document::AllocatorType& a);
	void AddString(Value& obj, const char* name, const XString& str, Document::AllocatorType& a);

	void AddFloatArray(Value& obj, const char* name, const float* pData, uint32 count, Document::AllocatorType& a);
	void LoadFloatArray(const Value& obj, float* pData, uint32 count);
}	