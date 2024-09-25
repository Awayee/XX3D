#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/Container.h"
#include "Core/Public/Algorithm.h"
#include "RHI/Public/RHI.h"

namespace Render {
	class GlobalShader {
	public:
		GlobalShader(const XString& shaderFile, const XString& entry, EShaderStageFlags type);
		virtual ~GlobalShader();
		RHIShader* GetRHI();
	private:
		RHIShaderPtr m_RHIShader;
	};

#define IMPLEMENT_GLOBAL_SHADER(cls, fileName, entryName, type)\
	class cls: public Render::GlobalShader{\
	public:\
		cls(): GlobalShader(fileName, entryName, type) {}\
		static uint64 GetHash(){\
			static uint64 s_Hash = 0;\
			if(s_Hash==0){\
				XString f(fileName), e(entryName);\
				s_Hash = DataArrayHash(f.data(), f.size(), 0); s_Hash = DataArrayHash(e.data(), e.size(), s_Hash);\
			}\
			return s_Hash;\
		}\
	};

	class GlobalShaderMap {
		SINGLETON_INSTANCE(GlobalShaderMap);
	public:
		template <typename T> T* GetShader() {
			uint64 hash = T::GetHash();
			if(auto iter = m_ShaderMap.find(hash); iter != m_ShaderMap.end()) {
				return (T*)(iter->second.Get());
			}
			T* shader = new T();
			m_ShaderMap.emplace(T::GetHash(), TUniquePtr<T>(shader));
			return shader;
		}
	private:
		TMap<uint64, TUniquePtr<GlobalShader>> m_ShaderMap;
		GlobalShaderMap();
		~GlobalShaderMap();
	};
}