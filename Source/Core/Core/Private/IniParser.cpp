#include "Core/Public/IniParser.h"
#include "Core/Public/Log.h"

// Remove spaces side of line
static XStringView TrimStringView(XStringView InStringView) {
    size_t Start = InStringView.find_first_not_of(" \t\r\n");
    if (Start == XStringView::npos) {
        return {};
    }
    size_t End = InStringView.find_last_not_of(" \t\r\n");
    return InStringView.substr(Start, End - Start + 1);
}

XXIniSection::XXIniSection(const XString& InSectionName) : SectionName(InSectionName)  {
}

XXIniSection::~XXIniSection() {
}

bool XXIniSection::HasKey(const char* Key) const {
    return KeyMap.find(Key) != KeyMap.end();
}

XString XXIniSection::GetString(const char* Key) const {
    return XString{ GetValueData(Key) };
}

int32 XXIniSection::GetInt(const char* Key) const {
    return StringToNum<int32>(GetValueData(Key));
}

float XXIniSection::GetFloat(const char* Key) const {
    return StringToNum<float>(GetValueData(Key));
}

bool XXIniSection::GetBool(const char* Key) const {
    return StringToNum<bool>(GetValueData(Key));
}

void XXIniSection::WriteToString(XString& Output) const {
    if(KeyMap.empty()) {
        return;
    }
    uint32 StrSize = SectionName.empty() ? 0 : (uint32)SectionName.size() + 3u;
    for(const auto& It: KeyMap) {
        StrSize += ((uint32)It.first.size() + It.second.Size + 2);
    }
    Output.clear();
    Output.reserve(StrSize);
    if(!SectionName.empty()) {
        Output.push_back('[');
        Output.append(SectionName);
        Output.push_back(']');
        Output.push_back('\n');
    }
    for(const auto& It : KeyMap) {
        Output.append(It.first);
        Output.push_back('=');
        const char* ValueStr = &MetaData[It.second.Index];
        const uint32 ValueSize = It.second.Size;
        Output.append(ValueStr, ValueSize);
        Output.push_back('\n');
    }
}

void XXIniSection::RemoveKey(const char* Key) {
    if(auto It = KeyMap.find(Key); It != KeyMap.end()) {
        FreeRanges.PushBack(It->second);
        KeyMap.erase(Key);
    }
}

void XXIniSection::SetString(const char* Key, const XString& Value) {
    SetKeyValueData(Key, Value);
}

void XXIniSection::SetInt32(const char* Key, int32 Value) {
    SetKeyValueData(Key, ToString<int32>(Value));
}

void XXIniSection::SetUint32(const char* Key, uint32 Value) {
    SetKeyValueData(Key, ToString<uint32>(Value));
}

void XXIniSection::SetFloat(const char* Key, float Value) {
    SetKeyValueData(Key, ToString<float>(Value));
}

void XXIniSection::SetBool(const char* Key, bool Value) {
    SetKeyValueData(Key, ToString<int32>(Value));
}

void XXIniSection::SetKeyValueData(const char* Key, const XString& Value) {
    if(Value.empty()) {
        return;
    }
    const uint32 StrSize = (uint32)Value.size();
    uint32 MetaDataIndex = INVALID_INDEX;
    if(auto It = KeyMap.find(Key); It != KeyMap.end()) {
        StrRange& Range = It->second;
        // not enough
        if(Range.Size < StrSize) {
            FreeRanges.PushBack(Range);
            MetaDataIndex = MetaData.Size();
            MetaData.Resize(MetaData.Size() + StrSize);
            Range.Index = MetaDataIndex;
            Range.Size = StrSize;
        }
        else {
            // Free the reset range
            const StrRange FreeRange{ Range.Index + StrSize, Range.Size - StrSize };
            if (FreeRange.Size == 0) {
                FreeRanges.PushBack(StrRange{ Range.Index + StrSize, Range.Size - StrSize });
            }
            MetaDataIndex = Range.Index;
            Range.Size = StrSize;
        }

    }
    else {
        // Find an available range
        for(uint32 i=0; i<FreeRanges.Size(); ++i) {
            StrRange& FreeRange = FreeRanges[i];
	        if(FreeRange.Size >= StrSize) {
                MetaDataIndex = FreeRange.Index;
                FreeRange.Index += StrSize;
                FreeRange.Size -= StrSize;
                if(FreeRange.Size == 0) {
                    FreeRanges.SwapRemoveAt(i);
                }
                break;
	        }
        }
        if(MetaDataIndex == INVALID_INDEX) {
            MetaDataIndex = MetaData.Size();
            MetaData.Resize(MetaData.Size() + StrSize);
        }
        KeyMap.insert({ Key, StrRange{MetaDataIndex, StrSize} });
    }
    memcpy(&MetaData[MetaDataIndex], Value.data(), Value.size() * sizeof(Value[0]));

}

XStringView XXIniSection::GetValueData(const char* Key) const {
    if(auto It = KeyMap.find(Key); It != KeyMap.end()) {
        return XStringView{ &MetaData[It->second.Index], It->second.Size };
    }
    return {};
}

XXIniParser::XXIniParser(): DefaultSection(""){
}

XXIniParser::~XXIniParser() {
}

bool XXIniParser::ReadFile(const char *FileName) {
    File::ReadFile IniFile{ FileName, false };
    if (!IniFile.IsOpen()) {
        LOG_WARNING("Failed to load read ini file %s", FileName);
        return false;
    }
    XString Line;
    XXIniSection* CurrentSection = &DefaultSection;
    while (IniFile.GetLine(Line)) {
        ReadLineData(Line, &CurrentSection);
    }
    return true;
}

bool XXIniParser::ReadFile(const XString& FileName) {
    return ReadFile(FileName.c_str());
}

bool XXIniParser::ReadFile(const File::FPath& File) {
    return ReadFile(File.string().c_str());
}

bool XXIniParser::ReadString(const XString& Content) {
    return false;
}

const XXIniSection* XXIniParser::GetSection(const char* SectionName) const {
    if(!SectionName) {
        return &DefaultSection;
    }
    if(const auto It = SectionsMap.find(SectionName); It!= SectionsMap.end()) {
        return It->second.Get();
    }
    return nullptr;
}

bool XXIniParser::HasKey(const char* SectionName, const char* Key) const {
    if(const XXIniSection* Section = GetSection(SectionName)) {
        return Section->HasKey(Key);
    }
    return false;
}

XString XXIniParser::GetString(const char* SectionName, const char* Key) const {
    if(const XXIniSection* Section = GetSection(SectionName)) {
        return Section->GetString(Key);
    }
    return XString{};
}

int32 XXIniParser::GetInt(const char* SectionName, const char* Key) const {
    if (const XXIniSection* Section = GetSection(SectionName)) {
        return Section->GetInt(Key);
    }
    return 0;
}

float XXIniParser::GetFloat(const char* SectionName, const char* Key) const {
    if (const XXIniSection* Section = GetSection(SectionName)) {
        return Section->GetFloat(Key);
    }
    return 0.0f;
}

bool XXIniParser::GetBool(const char* SectionName, const char* Key) const {
    if (const XXIniSection* Section = GetSection(SectionName)) {
        return Section->GetBool(Key);
    }
    return false;
}

XXIniSection* XXIniParser::GetSection(const char* SectionName) {
    if (!SectionName) {
        return &DefaultSection;
    }
    if (const auto It = SectionsMap.find(SectionName); It != SectionsMap.end()) {
        return It->second.Get();
    }
    return nullptr;
}

void XXIniParser::RemoveKey(const char* SectionName, const char* Key) {
    if(XXIniSection* Section = GetSection(SectionName)) {
        Section->RemoveKey(Key);
    }
}

void XXIniParser::SetString(const char* SectionName, const char* Key, const XString& Value) {
    if (XXIniSection* Section = GetOrAddSection(SectionName)) {
        Section->SetString(Key, Value);
    }
}

void XXIniParser::SetInt(const char* SectionName, const char* Key, int Value) {
    if (XXIniSection* Section = GetOrAddSection(SectionName)) {
        Section->SetInt32(Key, Value);
    }
}

void XXIniParser::SetFloat(const char* SectionName, const char* Key, float Value) {
    if (XXIniSection* Section = GetOrAddSection(SectionName)) {
        Section->SetFloat(Key, Value);
    }
}

void XXIniParser::SetBool(const char* SectionName, const char* Key, bool Value) {
    if (XXIniSection* Section = GetOrAddSection(SectionName)) {
        Section->SetBool(Key, Value);
    }
}

XXIniSection* XXIniParser::GetOrAddSection(const char* SectionName) {
    XXIniSection* Section = GetSection(SectionName);
    if(!Section) {
        Section = SectionsMap.insert({ SectionName, TUniquePtr<XXIniSection>(new XXIniSection(SectionName)) }).first->second.Get();
    }
    return Section;
}

void XXIniParser::RemoveSection(const char* SectionName) {
    SectionsMap.erase(SectionName);
}

void XXIniParser::WriteToString(XString& Content) {
    // Write default string
    {
        XString SectionString;
        DefaultSection.WriteToString(SectionString);
        Content.append(SectionString);
    }
    TArray<const XXIniSection*> SectionsArray; SectionsArray.Reserve((uint32)SectionsMap.size());
    for(auto& It : SectionsMap) {
        SectionsArray.PushBack(It.second.Get());
    }
    SectionsArray.Sort([](const XXIniSection* L, const XXIniSection* R) {return L->SectionName < R->SectionName; });
    for(const XXIniSection* Section: SectionsArray) {
        XString SectionString;
        Section->WriteToString(SectionString);
        Content.append(SectionString);
    }
}

bool XXIniParser::WriteToFile(const char* FileName) {
    File::WriteFile File{ FileName, false };
    if(!File.IsOpen()) {
        return false;
    }
    XString Content;
    WriteToString(Content);
    File.Write(Content.data(), (uint32)Content.size());
    return true;
}

void XXIniParser::ReadLineData(XStringView Line, XXIniSection** InoutSectionPtr) {

    Line = TrimStringView(Line);

    // Skip empty line
    if (Line.empty() || Line[0] == ';' || Line[0] == '#') {
        return;
    }

    // Parse section
    if (Line.front() == '[' && Line.back() == ']') {
        XString SectionName = XString{ TrimStringView(Line.substr(1, Line.size() - 2)) };
        *InoutSectionPtr = GetOrAddSection(SectionName.c_str());
    }

    // parse key and value
    else if (size_t EqualIndex = Line.find('='); EqualIndex != XString::npos) {
        XString Key = XString{ TrimStringView(Line.substr(0, EqualIndex)) };
        XString Value = XString{ TrimStringView(Line.substr(EqualIndex + 1)) };
        (*InoutSectionPtr)->SetKeyValueData(Key.c_str(), Value);
    }
}

#if 0
void TestIniParser() {
    // TODO test IniParser
    const char* IniFileName = "C:/ProjectConfigTest.ini";
    XXIniParser Ini;
    Ini.ReadFile(IniFileName);
    XXIniParser::XXIniSection* ProjectSection = Ini.GetOrAddSection("Project");
    ProjectSection->RemoveKey("StartLevel");
    ProjectSection->SetString("GameLevel", "XXXXX.level");
    ProjectSection->SetFloat("LevelSize", 1000.0f);
    XXIniParser::XXIniSection* RenderingSection = Ini.GetOrAddSection("Rendering");
    RenderingSection->SetBool("EnableRenderDoc", true);
    RenderingSection->SetString("RHIType", "D3D11");
    XXIniParser::XXIniSection* AnimSection = Ini.GetOrAddSection("Anim");
    XXIniParser::XXIniSection* PhysicsSection = Ini.GetOrAddSection("Physics");
    PhysicsSection->SetFloat("Gravity", 9.8f);
    PhysicsSection->SetBool("Enable Collision", false);
    PhysicsSection->SetInt32("MaxCollisionNum", 10000);
    PhysicsSection->SetUint32("MaxRigidsNum", 1024);

    XString PhysicsContent;
    PhysicsSection->WriteToString(PhysicsContent);
    XString IniContent;
    Ini.WriteToString(IniContent);
    LOG_DEBUG("%s", IniContent.c_str());
    if (!Ini.WriteToFile(IniFileName)) {
        LOG_ERROR("Failed to write file %s", IniFileName);
    }
}
#endif