#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/Container.h"
#include "RHI/Public/RHI.h"
#include "Core/Public/Algorithm.h"
#include "Render/Public/ShaderCompiler.h"

namespace Render {

	class GlobalShader{
	public:
		GlobalShader(const XString& shaderFile, const XString& entry, EShaderStageFlags type, TConstArrayView<ShaderCompiler::Macro> macros, uint32 permutationID);
		~GlobalShader() = default;
		RHIShader* GetRHI();
	private:
		RHIShaderPtr m_RHIShader;
	};

	class GlobalShaderMap {
		SINGLETON_INSTANCE(GlobalShaderMap);
	public:
		template<typename T> T* GetShader(const typename T::ShaderPermutation& p) {
			TArray<ShaderCompiler::Macro> macros;
			p.Apply(macros);
			uint64 hash = T::Hash;
			const uint32 permutationID = p.GetPermutationID();
			hash = GetTypeHash64BasedOn(permutationID, hash);
			GlobalShader* shader;
			if (auto iter = m_ShaderMap.find(hash); iter != m_ShaderMap.end()) {
				shader = iter->second.Get();
			}
			else {
				shader = m_ShaderMap.emplace(hash, TUniquePtr<T>(new T(macros, permutationID))).first->second.Get();
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
		GlobalShaderMap() = default;
		~GlobalShaderMap() = default;
	};

	// ================================ Shader permutation control ===================================

	// Controls whether the macro is defined.
	struct MacroDefineSwitch {
		const char* Name;
		bool Flag;
		MacroDefineSwitch(const char* name, bool defaultVal) : Name(name), Flag(defaultVal) {}
		MacroDefineSwitch& operator=(bool flag) {
			Flag = flag;
			return *this;
		}
		static constexpr uint32 PermutationSize() { return 2; }
		uint32 PermutationID() const { return Flag ? 1 : 0; }
		void Apply(TArray<ShaderCompiler::Macro>& macros) const { if (Flag) { macros.PushBack({ Name, {} }); } }
	};

	// Controls the int macro value
	template<int32 ValueCount, int32 ValueStart = 0>
	struct MacroDefineInt {
		const char* Name;
		int32 Value;
		MacroDefineInt(const char* name, int32 defaultVal) : Name(name) {
			CHECK(defaultVal >= ValueStart && defaultVal < ValueStart + ValueCount);
			Value = defaultVal;
		}
		MacroDefineInt& operator=(int32 value) {
			CHECK(value >= ValueStart && value < ValueStart + ValueCount);
			Value = value;
			return *this;
		}
		static constexpr uint32 PermutationSize() { return ValueCount; }
		uint32 PermutationID() const { return Value - ValueStart; }
		void Apply(TArray<ShaderCompiler::Macro>& macros) const { macros.PushBack({ Name, ToString(Value) }); }
	};

	struct ShaderPermutationEmpty {
		uint32 GetPermutationID() const { return 0; }
		static uint32 GetPermutationSize() { return 0; }
		void Apply(TArray<ShaderCompiler::Macro>& macros) const {/*Do nothing*/ }
	};

#define SHADER_PERMUTATION_DEFINE_BEGIN(MemName)\
		uint32 GetPermutationID() const { return MemName.PermutationID();}\
		static uint32 GetPermutationSize() { return Render::MacroDefineSwitch::PermutationSize();}\
		void Apply(TArray<ShaderCompiler::Macro>& macros) const{MemName.Apply(macros);}

#define SHADER_PERMUTATION_DEFINE_NEXT(MemName, PreMemName)\
		uint32 GetPermutationID() const{return MemName.PermutationID() + MemName.PermutationSize() * ShaderPermutation##PreMemName::GetPermutationID();}\
		static uint32 GetPermutationSize() {return Render::MacroDefineSwitch::PermutationSize() * ShaderPermutation##PreMemName::GetPermutationSize();}\
		void Apply(TArray<ShaderCompiler::Macro>& macros) const{ShaderPermutation##PreMemName::Apply(macros); MemName.Apply(macros);}

#define SHADER_PERMUTATION_BEGIN_SWITCH(MemName, DefaultValue)\
	private: struct ShaderPermutation##MemName{\
		Render::MacroDefineSwitch MemName{#MemName, DefaultValue};\
		SHADER_PERMUTATION_DEFINE_BEGIN(MemName)\
	}
#define SHADER_PERMUTATION_NEXT_SWITCH(MemName, DefaultValue, PreMemName)\
	private: struct ShaderPermutation##MemName: ShaderPermutation##PreMemName{\
		Render::MacroDefineSwitch MemName{#MemName, DefaultValue};\
		SHADER_PERMUTATION_DEFINE_NEXT(MemName, PreMemName)\
	}
#define SHADER_PERMUTATION_BEGIN_INT(MemName, ValueStart, ValueCount, DefaultValue)\
	private: struct ShaderPermutation##MemName{\
		Render::MacroDefineInt<ValueCount, ValueStart> MemName{#MemName, DefaultValue};\
		SHADER_PERMUTATION_DEFINE_BEGIN(MemName)\
	}
#define SHADER_PERMUTATION_NEXT_INT(MemName, ValueStart, ValueCount, DefaultValue, PreMemName)\
	private: struct ShaderPermutation##MemName: ShaderPermutation##PreMemName{\
		Render::MacroDefineInt<ValueCount, ValueStart> MemName{#MemName, DefaultValue};\
		SHADER_PERMUTATION_DEFINE_NEXT(MemName, PreMemName)\
	}
#define SHADER_PERMUTATION_END(PreMemName) public: typedef ShaderPermutation##PreMemName ShaderPermutation

#define SHADER_PERMUTATION_EMPTY() public: typedef Render::ShaderPermutationEmpty ShaderPermutation

#define GLOBAL_SHADER_IMPLEMENT(cls, fileName, entryName, type)\
	public:\
		cls(TConstArrayView<ShaderCompiler::Macro> macros, uint32 permutationID): Render::GlobalShader(fileName, entryName, type, macros, permutationID) {}\
		inline static uint64 Hash{DataArrayHash64(entryName, StringSize(entryName), DataArrayHash64(fileName, StringSize(fileName)))}
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