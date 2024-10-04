#pragma once
#include "D3D12Util.h"
#include "D3D12Descriptor.h"
#include "RHI/Public/RHIResources.h"

class D3D12Device;

// map RHI slot to d3d12 slot
class D3D12PipelineLayout {
public:
	struct DescriptorSlot {
		EDynamicDescriptorType HeapType;
		uint32 SlotIndex;
	};
	NON_COPYABLE(D3D12PipelineLayout);
	NON_MOVEABLE(D3D12PipelineLayout);
	D3D12PipelineLayout() = default;
	~D3D12PipelineLayout() = default;
	bool InitLayout(const TArray<RHIShaderParamSetLayout>& layouts, ID3D12Device* device);
	DescriptorSlot GetDescriptorSlot(uint32 set, uint32 binding) const;
	uint32 GetDescriptorCount(EDynamicDescriptorType heapType) const { return m_DescriptorCounts[EnumCast(heapType)]; }
	const TArray<EDynamicDescriptorType>& GetDescriptorTables() const { return m_DescriptorTables; }
	ID3D12RootSignature* GetRootSignature() { return m_RootSignature; }
private:
	TStaticArray<uint32, EnumCast(EDynamicDescriptorType::Count)> m_DescriptorCounts{ 0 };
	TArray<EDynamicDescriptorType> m_DescriptorTables;
	TArray<TArray<DescriptorSlot>> m_MapLayoutToHeap;
	TDXPtr<ID3D12RootSignature> m_RootSignature;
};

class D3D12GraphicsPipelineState: public RHIGraphicsPipelineState {
public:
	D3D12GraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, D3D12Device* device);
	~D3D12GraphicsPipelineState() override = default;
	ID3D12RootSignature* GetRootSignature() { return m_Layout.GetRootSignature(); }
	ID3D12PipelineState* GetPipelineState() { return m_Pipeline.Get(); }
	D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }
	const D3D12PipelineLayout& GetLayout() const { return m_Layout; }
private:
	D3D12PipelineLayout m_Layout;
	TDXPtr<ID3D12PipelineState> m_Pipeline;
	D3D_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
};

class D3D12ComputePipelineState: public RHIComputePipelineState {
public:
	D3D12ComputePipelineState(const RHIComputePipelineStateDesc& desc, D3D12Device* device);
	~D3D12ComputePipelineState() override = default;
	ID3D12RootSignature* GetRootSignature() { return m_Layout.GetRootSignature(); }
	ID3D12PipelineState* GetPipelineState() { return m_Pipeline.Get(); }
	const D3D12PipelineLayout& GetLayout()  { return m_Layout; }
private:
	D3D12PipelineLayout m_Layout;
	TDXPtr<ID3D12PipelineState> m_Pipeline;
};

class D3D12PipelineDescriptorCache {
public:
	NON_COPYABLE(D3D12PipelineDescriptorCache);
	D3D12PipelineDescriptorCache(D3D12Device* device) :m_Device(device), m_PipelineData(nullptr), m_PipelineType(EPipelineType::None) {}
	~D3D12PipelineDescriptorCache() = default;
	void BindGraphicsPipelineState(D3D12GraphicsPipelineState* pipeline);
	void BindComputePipelineState(D3D12ComputePipelineState* pipeline);
	void Reset();
	void SetShaderParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& param);
	D3D12GraphicsPipelineState* GetGraphicsPipelineState();
	D3D12ComputePipelineState* GetComputePipelineState();
	void PreDraw(ID3D12GraphicsCommandList* cmd);
private:
	enum class EPipelineType {
		Graphics,
		Compute,
		None
	};
	struct DescriptorCache {
		TArray<RHIShaderParam> Params;
		DynamicDescriptorHandle DynamicDescriptor;
	};
	TStaticArray<DescriptorCache, EnumCast(EDynamicDescriptorType::Count)> m_DescriptorCaches{};
	TStaticArray<bool, EnumCast(EDynamicDescriptorType::Count)> m_DirtyDescriptorTables{};
	D3D12Device* m_Device;
	union {
		D3D12GraphicsPipelineState* m_GraphicsPipelineState;
		D3D12ComputePipelineState* m_ComputePipelineState;
		void* m_PipelineData;
	};
	EPipelineType m_PipelineType;
	const D3D12PipelineLayout* GetLayout();
	void ResetDescriptorCaches();
};