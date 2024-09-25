#pragma once
#include "D3D12Util.h"
#include "D3D12Descriptor.h"
#include "Core/Public/String.h"
#include "RHI/Public/RHIResources.h"

class D3D12Device;
enum class ETexDescriptorType {
	SRV = 0,
	UAV,
	RTV,
	DSV,
	Count
};

class D3D12Buffer: public RHIBuffer {
public:
	D3D12Buffer(const RHIBufferDesc& desc, ID3D12Device* device);
	~D3D12Buffer() override = default;
	void UpdateData(const void* data, uint32 byteSize, uint32 offset) override;
	void SetName(const char* name) override;
	ID3D12Resource* GetResource() { return m_Resource.Get(); }
protected:
	TDXPtr<ID3D12Resource> m_Resource;
};

// buffer with descriptors
class D3D12BufferImpl : public D3D12Buffer {
public:
	D3D12BufferImpl(const RHIBufferDesc& desc, D3D12Device* device);
	~D3D12BufferImpl() override;
	void UpdateData(const void* data, uint32 byteSize, uint32 offset) override;
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCBV();
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV();

private:
	D3D12Device* m_Device;
	StaticDescriptorHandle m_CBV;
	StaticDescriptorHandle m_UAV;
};

class ResourceState {
public:
	void Initialize(uint32 subResCount);
	void SetState(D3D12_RESOURCE_STATES state);
	void SetSubResState(uint32 subResIndex, D3D12_RESOURCE_STATES state);
	D3D12_RESOURCE_STATES GetSubResState(uint32 subResIndex) const; // if m_CoverSubRes, always return m_ResState
	uint32 GetSubResCount() const { return m_SubResStates.Size(); }
private:
	uint32 m_ResState : 31;
	bool   m_CoverSubRes : 1;
	TArray<D3D12_RESOURCE_STATES> m_SubResStates;
};

class D3D12Texture: public RHITexture {
public:
	D3D12Texture(const RHITextureDesc& desc);
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptor(ETexDescriptorType type, RHITextureSubRes subRes) = 0;
	virtual ID3D12Resource* GetResource() = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptor(ETexDescriptorType type) { return GetDescriptor(type, GetDesc().GetDefaultSubRes()); }
	ResourceState& GetResState() { return m_ResState; }
protected:
	ResourceState m_ResState;
};

class D3D12TextureImpl: public D3D12Texture {
public:
	D3D12TextureImpl(const RHITextureDesc& desc, D3D12Device* device);
	~D3D12TextureImpl() override;
	void UpdateData(const void* data, uint32 byteSize, RHITextureSubRes subRes, IOffset3D offset) override;
	void SetName(const char* name) override;
	ID3D12Resource* GetResource() override { return m_Resource.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptor(ETexDescriptorType type, RHITextureSubRes subRes) override;
private:
	D3D12Device* m_Device;
	TDXPtr<ID3D12Resource> m_Resource;
	struct SubResDescriptor {
		RHITextureSubRes SubRes;
		StaticDescriptorHandle Descriptor;
	};
	TStaticArray<TArray<TArray<SubResDescriptor>>, EnumCast(ETexDescriptorType::Count)> m_Descriptors; // sorted by array layer for searching
	void CreateSRV(RHITextureSubRes subRes, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
	void CreateUAV(RHITextureSubRes subRes, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
	void CreateRTV(RHITextureSubRes subRes, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
	void CreateDSV(RHITextureSubRes subRes, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
};

class D3D12Sampler: public RHISampler {
public:
	D3D12Sampler(const RHISamplerDesc& desc, D3D12Device* device);
	~D3D12Sampler() override;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle();
private:
	D3D12Device* m_Device;
	StaticDescriptorHandle m_Descriptor;
};

class D3D12Fence: public RHIFence {
public:
	explicit D3D12Fence(ID3D12Device* device);
	~D3D12Fence()override = default;
	void SetName(const char* name) override;
	void Wait() override;
	void Reset() override;
	void ResetValue(uint64 val);
	uint64 GetTargetValue() const { return m_TargetValue; }
	ID3D12Fence* GetFence();
private:
	uint64 m_TargetValue;
	TDXPtr<ID3D12Fence> m_Fence;
};

class D3D12Shader: public RHIShader {
public:
	D3D12Shader(EShaderStageFlags stage, const void* byteData, uint32 byteSize);
	~D3D12Shader() = default;
	TConstArrayView<uint8> GetBytes();
private:
	TArray<uint8> m_Bytes;
};
