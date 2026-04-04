#pragma once
#include "Core/Public/IniParser.h"
#include "Core/Public/TArray.h"
#include "Core/Public/Defines.h"

namespace Engine {
#define DEFINE_CONFIG_STRING(Key)

#define DEFINE_CONFIG_INT(Key, DefaultVal)

#define DEFINE_CONFIG_FLOAT(Key, DefaultVal)

#define DEFINE_CONFIG_BOOL(Key, DefaultVal)

#define CONFIG_PROPERTY_BEING(Cls)\
	private:\
	typedef void (Cls::*LoadConfigFunc)(const XXIniParser&);\
	typedef void (Cls::*SaveConfigFunc)(XXIniParser&);\
	struct ConfigFuncPair { LoadConfigFunc LoadFunc; SaveConfigFunc SaveFunc; };\
	inline static TArray<ConfigFuncPair> LoadConfigFuncs;\
	inline static uint32 AddConfigFunc(LoadConfigFunc LoadFunc, SaveConfigFunc SaveFunc) {\
		uint32 ConfigID = LoadConfigFuncs.Size();\
		LoadConfigFuncs.PushBack({ LoadFunc, SaveFunc });\
		return ConfigID;\
	}\
	inline static uint32 ShrinkConfigArray() {\
		LoadConfigFuncs.Shrink();\
		return LoadConfigFuncs.Size();\
	}\
	public:\
	void LoadConfig(const XXIniParser& IniParser) {\
		for (const auto& FuncPair : LoadConfigFuncs) {\
			(this->*FuncPair.LoadFunc)(IniParser);\
		}\
	}\
	void SaveConfig(XXIniParser& IniParser) {\
		for (const auto& FuncPair : LoadConfigFuncs) {\
			(this->*FuncPair.SaveFunc)(IniParser);\
		}\
	}\

#define CONFIG_PROPERTY(SectionName, Type, KeyName, GetSetName, DefaultValue)\
	public:\
	Type KeyName{DefaultValue};\
	private:\
	void Load##KeyName##Config(const XXIniParser& IniParser) {\
		if (IniParser.HasKey(#SectionName, #KeyName)) {\
			(KeyName) = (Type)IniParser.Get##GetSetName(#SectionName, #KeyName);\
		}\
	}\
	void Save##KeyName##Config(XXIniParser& IniParser) { IniParser.Set##GetSetName(#SectionName, #KeyName, KeyName); }\
	inline static uint32 KeyName##ConfigID{ AddConfigFunc(&Load##KeyName##Config, &Save##KeyName##Config) }

#define CONFIG_PROPERTY_ENUM(SectionName, Type, KeyName, DefaultValue)\
	public:\
	Type KeyName{DefaultValue};\
	private:\
	void Load##KeyName##Config(const XXIniParser& IniParser) {\
		if (IniParser.HasKey(#SectionName, #KeyName)) {\
			(KeyName) = (Type)IniParser.GetInt(#SectionName, #KeyName);\
		}\
	}\
	void Save##KeyName##Config(XXIniParser& IniParser) { IniParser.SetInt(#SectionName, #KeyName, (int)(KeyName)); }\
	inline static uint32 KeyName##ConfigID{ AddConfigFunc(&Load##KeyName##Config, &Save##KeyName##Config) }

#define CONFIG_PROPERTY_STRING(SectionName, KeyName, DefaultValue) CONFIG_PROPERTY(SectionName, XString, KeyName, String, DefaultValue)
#define CONFIG_PROPERTY_INT(SectionName, KeyName, DefaultValue)    CONFIG_PROPERTY(SectionName, int, KeyName, Int, DefaultValue)
#define CONFIG_PROPERTY_UINT(SectionName, KeyName, DefaultValue)   CONFIG_PROPERTY(SectionName, uint32, KeyName, Int, DefaultValue)
#define CONFIG_PROPERTY_FLOAT(SectionName, KeyName, DefaultValue)  CONFIG_PROPERTY(SectionName, float, KeyName, Float, DefaultValue)
#define CONFIG_PROPERTY_BOOL(SectionName, KeyName, DefaultValue)   CONFIG_PROPERTY(SectionName, bool, KeyName, Bool, DefaultValue)

#define CONFIG_PROPERTY_END(Cls)\
	private:\
	inline static uint32 NumConfig{ Cls::ShrinkConfigArray()};\


// demo code
#if 0
	struct MyConfig {
		CONFIG_PROPERTY_BEING(MyConfig)
			CONFIG_PROPERTY_BOOL(Section0, BoolValue);
			CONFIG_PROPERTY_FLOAT(Section1, FloatValue);
			CONFIG_PROPERTY_STRING(Section1, StringValue);
		CONFIG_PROPERTY_END(MyConfig);
	};
#endif
}