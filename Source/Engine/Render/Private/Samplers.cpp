#include "Render/Public/Samplers.h"
namespace Engine {

	RHISampler* DefaultSampler::GetSampler(ESamplerFilter filter, ESamplerAddressMode addressMode) {
		return m_Samplers[(uint8)addressMode][(uint8)filter].Get();
	}

	DefaultSampler::DefaultSampler() {
		RHISamplerDesc desc;
		for(uint8 addressMode=0; addressMode < (uint8)ESamplerAddressMode::MaxNum; ++addressMode) {
			desc.AddressU = desc.AddressV = desc.AddressW = (ESamplerAddressMode)addressMode;
			for(uint8 filter=0; filter<(uint8)ESamplerFilter::MaxNum; ++filter) {
				desc.Filter = (ESamplerFilter)filter;
				m_Samplers[addressMode][filter] = RHI::Instance()->CreateSampler(desc);
			}
		}
	}

	DefaultSampler::~DefaultSampler() {
	}
}
