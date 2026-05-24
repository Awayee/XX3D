#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/Container.h"
#include "RHI/Public/RHI.h"
#include "Core/Public/Algorithm.h"
#include "Render/Public/ShaderCompiler.h"

namespace Render {

	class GlobalShader: public RHIShaderBindingInterface{
	public:
		GlobalShader(XStringView shaderFile, XStringView entry, EShaderStageFlags type, TConstArrayView<ShaderCompiler::Macro> macros, uint32 permutationID);
		~GlobalShader() = default;
		RHIShader* GetRHI();
		virtual void GetBindings(RHIShaderBindingSet& bindingSet) override {};
	protected:
		RHIShaderPtr m_RHIShader;
	};

	class GlobalShaderMap {
		SINGLETON_INSTANCE(GlobalShaderMap);
	public:
		template<typename T> T* GetShader(const typename T::ShaderPermutation& p) {
			uint64 hash = T::Hash;
			hash = GetTypeHash64BasedOn(p.GetPermutationID(), hash);
			GlobalShader* shader;
			if (auto iter = m_ShaderMap.find(hash); iter != m_ShaderMap.end()) {
				shader = iter->second.Get();
			}
			else {
				shader = m_ShaderMap.emplace(hash, TUniquePtr<T>(new T(p))).first->second.Get();
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
		bool operator==(bool flag) const {
			return Flag == flag;
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
		bool operator==(int32 value) const {
			return Value== value;
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

	template<typename T>
	TArray<ShaderCompiler::Macro> GetPermutationMacros(T p) {
		TArray<ShaderCompiler::Macro> macros;
		p.Apply(macros);
		return macros;
	}

	// Shader permutation
#define BEGIN_SHADER_PERMUTATION private:\
		typedef Render::ShaderPermutationEmpty

#define SHADER_PERMUTATION_SWITCH(name, defaultValue) PrevOfShaderPermutation##name;\
		struct ShaderPermutation##name: PrevOfShaderPermutation##name{\
			Render::MacroDefineSwitch name{#name, defaultValue};\
			uint32 GetPermutationID() const{\
				return name.PermutationID() + name.PermutationSize() * PrevOfShaderPermutation##name::GetPermutationID();\
			}\
			static uint32 GetPermutationSize() {\
				return Render::MacroDefineSwitch::PermutationSize() * ShaderPermutation##name::GetPermutationSize();\
			}\
			void Apply(TArray<ShaderCompiler::Macro>& macros) const{\
				PrevOfShaderPermutation##name::Apply(macros); name.Apply(macros);\
			}\
		}; typedef ShaderPermutation##name

#define SHADER_PERMUTATION_INT(name, valueStart, valueCount, defaultValue) PrevOfShaderPermutation##name;\
		struct ShaderPermutation##name: PrevOfShaderPermutation##name{\
			Render::MacroDefineInt<valueCount, valueStart> name{#name, DefaultValue};\
			uint32 GetPermutationID() const {\
				return name.PermutationID() + name.PermutationSize() * PrevOfShaderPermutation##name::GetPermutationID();\
			}\
			static uint32 GetPermutationSize() {\
				return Render::MacroDefineInt<valueCount, valueStart>::PermutationSize() * ShaderPermutation##name::GetPermutationSize();\
			}\
			void Apply(TArray<ShaderCompiler::Macro>& macros) const{\
				PrevOfShaderPermutation##name::Apply(macros); name.Apply(macros);\
			}\
		}; typedef ShaderPermutation##name

#define END_SHADER_PERMUTATION ShaderPermutationEnd;\
		public: typedef ShaderPermutationEnd ShaderPermutation;

#define SHADER_PERMUTATION_EMPTY() public: typedef Render::ShaderPermutationEmpty ShaderPermutation

	// Shader binding
#define BEGIN_SHADER_BINDING private:\
		struct sShaderBindingBegin{\
			inline static constexpr uint8 sBindingSet = 0;\
			static void* GetBindings(RHIShaderBindingSet&, EShaderStageFlags, const ShaderPermutation&){return nullptr ;}\
		}; typedef sShaderBindingBegin

#define SHADER_BINDING_SET(set) PrevOfShaderBindingSet_##set;\
		static_assert(set < RHIShaderBinding::MAX_SET);\
		private:\
		struct sShaderBindingSet_##set {\
			inline static constexpr uint8 sBindingSet = set;\
			static void* GetBindings(RHIShaderBindingSet&, EShaderStageFlags, const ShaderPermutation&){return PrevOfShaderBindingSet_##set::GetBindings;}\
		}; typedef sShaderBindingSet_##set

#define SHADER_BINDING(binding, type, name, numElements) PrefOfShaderBinding##name;\
		static_assert(binding < RHIShaderBinding::MAX_BINDING);\
		public:\
		inline static RHIShaderBinding name{binding, PrefOfShaderBinding##name::sBindingSet, EBindingType::type, numElements};\
		private:\
		struct sShaderBinding##name{\
			inline static constexpr uint8 sBindingSet=PrefOfShaderBinding##name::sBindingSet;\
			static void* GetBindings(RHIShaderBindingSet& bindingSet, EShaderStageFlags stage, const ShaderPermutation& p) {\
				bindingSet.AddBinding(name, stage);\
				return PrefOfShaderBinding##name::GetBindings; \
			}\
		}; typedef sShaderBinding##name

#define SHADER_BINDING_WITH_MACRO(binding, type, name, numElements, macro, macroValue) PrefOfShaderBinding##name;\
		static_assert(binding < RHIShaderBinding::MAX_BINDING);\
		public:\
		inline static RHIShaderBinding name{binding, PrefOfShaderBinding##name::sBindingSet, EBindingType::type, numElements};\
		private:\
		struct sShaderBinding##name{\
			inline static constexpr uint8 sBindingSet=PrefOfShaderBinding##name::sBindingSet;\
			static void* GetBindings(RHIShaderBindingSet& bindingSet, EShaderStageFlags stage, const ShaderPermutation& p) {\
				if(p.macro == macroValue) {bindingSet.AddBinding(name, stage); }\
				return PrefOfShaderBinding##name::GetBindings; \
			}\
		};typedef sShaderBinding##name

#define END_SHADER_BINDING sShaderBindingEnd;\
		static void GetBindingsStatic(RHIShaderBindingSet& bindingSet, EShaderStageFlags stage, const ShaderPermutation& p) {\
			typedef void*(*AppendBindingFunc)(RHIShaderBindingSet&, EShaderStageFlags, const ShaderPermutation&);\
			AppendBindingFunc func = sShaderBindingEnd::GetBindings;\
			do{ func = (AppendBindingFunc)func(bindingSet, stage, p);}\
			while(func != nullptr);\
		};

#define SHADER_BINDING_EMPTY() static void GetBindingsStatic(RHIShaderBindingSet& bindingSet, EShaderStageFlags stage, const ShaderPermutation& p) {}

#define GLOBAL_SHADER_IMPLEMENT(cls, fileName, entryName, type)\
	public:\
		inline static uint64 Hash{DataArrayHash64(entryName, StringSize(entryName), DataArrayHash64(fileName, StringSize(fileName)))};\
		cls(const ShaderPermutation& p): Render::GlobalShader(fileName, entryName, type, GetPermutationMacros(p), p.GetPermutationID()), m_Permutation(p) {}\
		virtual void GetBindings(RHIShaderBindingSet& bindingSet){\
			GetBindingsStatic(bindingSet, m_RHIShader->GetStage(), m_Permutation);\
		}\
	private:\
		ShaderPermutation m_Permutation
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