#pragma once
#include "Core/Public/File.h"
#include "Core/Public/String.h"
#include "Core/Public/Container.h"
#include "Core/Public/TUniquePtr.h"


class XXIniSection {
public:
    XXIniSection(const XString& InSectionName);
    ~XXIniSection();
    // Get
    bool    HasKey(const char* Key) const;
    XString GetString(const char* Key) const;
    int32   GetInt(const char* Key) const;
    float   GetFloat(const char* Key) const;
    bool    GetBool(const char* Key) const;
    // Set
    void    RemoveKey(const char* Key);
    void    SetString(const char* Key, const XString& Value);
    void    SetInt32(const char* Key, int32 Value);
    void    SetUint32(const char* Key, uint32 Value);
    void    SetFloat(const char* Key, float Value);
    void    SetBool(const char* Key, bool Value);
    // Write
    void    WriteToString(XString& Output) const;
private:
    struct StrRange { uint32 Index, Size; };
    XString SectionName;
    TArray<char> MetaData; // "value1\0value1\0value2\0..."
    TArray<StrRange> FreeRanges;
    TMap<XString, StrRange> KeyMap;
    friend class XXIniParser;
    void SetKeyValueData(const char* Key, const XString& Value);
    XStringView GetValueData(const char* Key) const; // Do not save the return value!
};

class XXIniParser {
public:
    XXIniParser();
    ~XXIniParser();
    // Load
    bool    ReadFile(const char* FileName);
    bool    ReadFile(const XString& FileName);
    bool    ReadFile(const File::FPath& File);
    bool    ReadString(const XString& Content);
    // Get
    const   XXIniSection* GetSection(const char* SectionName) const;
    bool    HasKey(const char* SectionName, const char* Key) const;
    XString GetString(const char* SectionName, const char* Key) const;
    int32   GetInt(const char* SectionName, const char* Key) const;
    float   GetFloat(const char* SectionName, const char* Key) const;
    bool    GetBool(const char* SectionName, const char* Key) const;
    // Set
    XXIniSection* GetSection(const char* SectionName);
    XXIniSection* GetOrAddSection(const char* SectionName);
    void RemoveSection(const char* SectionName);
    void RemoveKey(const char* SectionName, const char* Key);
    void SetString(const char* SectionName, const char* Key, const XString& Value);
    void SetInt(const char* SectionName, const char* Key, int Value);
    void SetFloat(const char* SectionName, const char* Key, float Value);
    void SetBool(const char* SectionName, const char* Key, bool Value);
    // Write
    void WriteToString(XString& Content);
    bool WriteToFile(const char* FileName);
private:
    TMap<XString, TUniquePtr<XXIniSection>> SectionsMap;
    XXIniSection DefaultSection; // ""
    void ReadLineData(XStringView Line, XXIniSection** InoutSectionPtr);
};