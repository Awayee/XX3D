#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/Container.h"
#include "HLSLCompiler/HLSLCompiler.h"
#include "RHI/Public/RHI.h"
#include "Util/Public/Algorithm.h"

namespace Render {
	using ShaderDefine = HLSLCompiler::SPVDefine;
	using ShaderStage = HLSLCompiler::ESPVShaderStage;
	class GlobalShader {
	public:
		GlobalShader(const XString& shaderFile, const XString& entry, EShaderStageFlags type, TConstArrayView<ShaderDefine> defines, uint32 permutationID);
		virtual ~GlobalShader() = default;
		RHIShader* GetRHI();
	private:
		RHIShaderPtr m_RHIShader;
	};

	class HLSLCompilerBase {
	public:
		virtual ~HLSLCompilerBase() = default;
		virtual XString GetCompileOutputFile(const XString& hlslFile, const XString& entryPoint, uint32 permutationID) = 0;
		virtual bool Compile(const XString& hlslFile, const XString& entryPoint, ShaderStage stage, TConstArrayView<ShaderDefine> defines, uint32 permutationID) = 0;
	};

	class GlobalShaderMap {
		SINGLETON_INSTANCE(GlobalShaderMap);
	public:
		template<typename T> T* GetShader(const typename T::ShaderPermutation& p) {
			TArray<ShaderDefine> defines;
			p.Apply(defines);
			uint64 hash = T::Hash;
			const uint32 permutationID = p.GetPermutationID();
			hash = TypeHash(permutationID, hash);
			GlobalShader* shader;
			if (auto iter = m_ShaderMap.find(hash); iter != m_ShaderMap.end()) {
				shader = iter->second.Get();
			}
			else {
				shader = m_ShaderMap.emplace(hash, TUniquePtr<T>(new T(defines, permutationID))).first->second.Get();
			}
			return (T*)shader;
		}

		// use default permutation
		template<typename T> T* GetShader() {
			typename T::ShaderPermutation p{};
			return GetShader<T>(p);
		}
	private:
		TMap<uint64, TUniquePtr<GlobalShader>> m_ShaderMap;
		GlobalShaderMap();
		~GlobalShaderMap() = default;
	};

	// ================================ Shader permutation control ===================================

	// Controls whether the macro is defined.
	struct ShaderDefineSwitch {
		const char* Name;
		bool Flag;
		ShaderDefineSwitch(const char* name, bool defaultVal) : Name(name), Flag(defaultVal) {}
		ShaderDefineSwitch& operator=(bool flag) {
			Flag = flag;
			return *this;
		}
		static constexpr uint32 PermutationSize() { return 2; }
		uint32 PermutationID() const { return Flag ? 1 : 0; }
		void Apply(TArray<ShaderDefine>& defines) const { if (Flag) { defines.PushBack({ Name, "" }); } }
	};

	// Controls the int macro value
	template<int32 ValueCount, int32 ValueStart = 0>
	struct ShaderDefineInt {
		const char* Name;
		int32 Value;
		ShaderDefineInt(const char* name, int32 defaultVal) : Name(name) {
			CHECK(defaultVal >= ValueStart && defaultVal < ValueStart + ValueCount);
			Value = defaultVal;
		}
		ShaderDefineInt& operator=(int32 value) {
			CHECK(value >= ValueStart && value < ValueStart + ValueCount);
			Value = value;
			return *this;
		}
		static constexpr uint32 PermutationSize() { return ValueCount; }
		uint32 PermutationID() const { return Value - ValueStart; }
		void Apply(TArray<ShaderDefine>& defines) const { defines.PushBack({ Name, ToString(Value) }); }
	};

	struct ShaderPermutationEmpty {
		uint32 GetPermutationID() const { return 0; }
		static uint32 GetPermutationSize() { return 0; }
		void Apply(TArray<ShaderDefine>& defines) const {/*Do nothing*/ }
	};

#define SHADER_PERMUTATION_DEFINE_START(MemName)\
		uint32 GetPermutationID() const { return MemName.PermutationID();}\
		static uint32 GetPermutationSize() { return Render::ShaderDefineSwitch::PermutationSize();}\
		void Apply(TArray<Render::ShaderDefine>& defines) const{MemName.Apply(defines);}

#define SHADER_PERMUTATION_DEFINE_NEXT(MemName, PreMemName)\
		uint32 GetPermutationID() const{return MemName.PermutationID() + MemName.PermutationSize() * ShaderPermutation##PreMemName::GetPermutationID();}\
		static uint32 GetPermutationSize() {return Render::ShaderDefineSwitch::PermutationSize() * ShaderPermutation##PreMemName::GetPermutationSize();}\
		void Apply(TArray<Render::ShaderDefine>& defines) const{ShaderPermutation##PreMemName::Apply(defines); MemName.Apply(defines);}

#define SHADER_PERMUTATION_BEGIN_SWITCH(MemName, DefaultValue)\
	private: struct ShaderPermutation##MemName{\
		Render::ShaderDefineSwitch MemName{#MemName, DefaultValue};\
		SHADER_PERMUTATION_DEFINE_START(MemName)\
	}
#define SHADER_PERMUTATION_NEXT_SWITCH(MemName, DefaultValue, PreMemName)\
	private: struct ShaderPermutation##MemName: ShaderPermutation##PreMemName{\
		Render::ShaderDefineSwitch MemName{#MemName, DefaultValue};\
		SHADER_PERMUTATION_DEFINE_NEXT(MemName, PreMemName)\
	}
#define SHADER_PERMUTATION_BEGIN_INT(MemName, ValueStart, ValueCount, DefaultValue)\
	private: struct ShaderPermutation##MemName{\
		Render::ShaderDefineInt<ValueCount, ValueStart> MemName{#MemName, DefaultValue};\
		SHADER_PERMUTATION_DEFINE_START(MemName)\
	}
#define SHADER_PERMUTATION_NEXT_INT(MemName, ValueStart, ValueCount, DefaultValue, PreMemName)\
	private: struct ShaderPermutation##MemName: ShaderPermutation##PreMemName{\
		Render::ShaderDefineInt<ValueCount, ValueStart> MemName{#MemName, DefaultValue};\
		SHADER_PERMUTATION_DEFINE_NEXT(MemName, PreMemName)\
	}
#define SHADER_PERMUTATION_END(PreMemName) public: typedef ShaderPermutation##PreMemName ShaderPermutation

#define SHADER_PERMUTATION_EMPTY() public: typedef Render::ShaderPermutationEmpty ShaderPermutation

#define GLOBAL_SHADER_IMPLEMENT(cls, fileName, entryName, type)\
	public:\
		cls(TConstArrayView<Render::ShaderDefine> defines, uint32 permutationID): Render::GlobalShader(fileName, entryName, type, defines, permutationID) {}\
		inline static uint64 Hash{DataArrayHash(entryName, StringSize(entryName), DataArrayHash(fileName, StringSize(fileName)))}
}

// example code
#if false
class ExampleShader : public Render::GlobalShader {
	GLOBAL_SHADER_IMPLEMENT(ExampleShader, "Example.hlsl", "MainCS", EShaderStageFlags::Compute);
	SHADER_PERMUTATION_BEGIN_SWITCH(MACRO_NAME0, false);
	SHADER_PERMUTATION_NEXT_SWITCH(MACRO_NAME1, false, MACRO_NAME0);
	SHADER_PERMUTATION_NEXT_INT(MACRO_NAME2, 0, 8, 1, MACRO_NAME1);
	SHADER_PERMUTATION_END(MACRO_NAME2);
};
void Example() {
	ExampleShader::ShaderPermutation p;
	p.MACRO_NAME2 = 5;
	p.MACRO_NAME1 = true;
	ExampleShader* shader = Render::GlobalShaderMap::Instance()->GetShader<ExampleShader>(p);
	//...
}
#endif