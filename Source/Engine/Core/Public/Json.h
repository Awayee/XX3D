#pragma once
#include "Math/Public/Math.h"
#include "Core/Public/File.h"
#include <document.h>
#include <String>
namespace Json {
	using Type = rapidjson::Type;
	using Value = rapidjson::Value;
	using Document = rapidjson::Document;

	bool ReadFile(const char* file, Document& doc);
	bool ReadFile(File::RFile& file, Document& doc);
	bool WriteFile(const char* file, const Document& doc, bool pretty=true);
	bool WriteFile(File::WFile& out, const Document& doc, bool pretty=true);

	void AddStringMember(Document& doc, const char* name, const std::string& str, Document::AllocatorType& a);
	void AddString(Value& obj, const char* name, const std::string& str, Document::AllocatorType& a);

	void AddFloatArray(Value& obj, const char* name, const float* pData, uint32 count, Document::AllocatorType& a);
	void LoadFloatArray(const Value& obj, float* pData, uint32 count);
}	