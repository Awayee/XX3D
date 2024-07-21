#pragma once
#include "Core/Public/TSingleton.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHI.h"

namespace Render {
	class DefaultSampler: public TSingleton<DefaultSampler> {
	public:
		RHISampler* GetSampler(ESamplerFilter filter, ESamplerAddressMode addressMode);
	private:
		friend TSingleton<DefaultSampler>;
		DefaultSampler();
		~DefaultSampler() override;
		TStaticArray<TStaticArray<TUniquePtr<RHISampler>, (uint8)ESamplerFilter::MaxNum>, (uint8)ESamplerAddressMode::MaxNum> m_Samplers;
	};
}