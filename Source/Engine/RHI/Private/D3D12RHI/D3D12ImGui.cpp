#include "D3D12ImGui.h"
#include "D3D12Util.h"
#include "D3D12RHI.h"
#include "D3D12Device.h"
#include "D3D12Resources.h"
#include "D3D12Command.h"
#include "D3D12Descriptor.h"
#include "Window/Public/EngineWindow.h"
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

D3D12ImGui::D3D12ImGui(void (* configInitializer)()) {
	IMGUI_CHECKVERSION();
	m_Context = ImGui::CreateContext();
	WindowHandle windowHandle = Engine::EngineWindow::Instance()->GetWindowHandle();
	D3D12RHI* d3d12RHI = (D3D12RHI*)RHI::Instance();
	ImGui_ImplWin32_Init((HWND)windowHandle);
	m_Device = d3d12RHI->GetDevice()->GetDevice();
	m_DescriptorAllocator.Reset(new StaticDescriptorAllocator(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 64));

	// allocate font descriptor
	m_FontDescriptor = m_DescriptorAllocator->AllocateDescriptorSlot();
	ID3D12DescriptorHeap* fontHeap = m_DescriptorAllocator->GetHeap(m_FontDescriptor.HeapIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE fontCpuHandle = m_DescriptorAllocator->GetCPUHandle(m_FontDescriptor);
	D3D12_GPU_DESCRIPTOR_HANDLE fontGpuHandle = m_DescriptorAllocator->GetGPUHandle(m_FontDescriptor);
	ImGui_ImplDX12_Init(m_Device, RHI_FRAME_IN_FLIGHT_MAX, DXGI_FORMAT_R8G8B8A8_UNORM, fontHeap, fontCpuHandle, fontGpuHandle);
	if (configInitializer) {
		configInitializer();
	}
}

D3D12ImGui::~D3D12ImGui() {
	if (m_Context) {
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext(m_Context);
		m_Context = nullptr;
	}
}

void D3D12ImGui::FrameBegin() {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void D3D12ImGui::FrameEnd() {
	ImGui::EndFrame();
	ImGui::UpdatePlatformWindows();
}

void D3D12ImGui::RenderDrawData(RHICommandBuffer* cmd) {
	ImGui::Render();
	ID3D12GraphicsCommandList* d3d12Cmd = ((D3D12CommandList*)cmd)->GetD3D12Ptr();
	if(!m_ImTextureDescriptorMap.empty()) {
		// collect heaps
		TSet<uint32> heapIndicesSet;
		heapIndicesSet.insert(m_FontDescriptor.HeapIndex);
		for (auto& [texID, handle] : m_ImTextureDescriptorMap) {
			heapIndicesSet.insert(handle.HeapIndex);
		}
		TArray<ID3D12DescriptorHeap*> heaps;
		for (const uint32 heapIndex : heapIndicesSet) {
			heaps.PushBack(m_DescriptorAllocator->GetHeap(heapIndex));
		}
		d3d12Cmd->SetDescriptorHeaps(heaps.Size(), heaps.Data());
	}
	else {
		ID3D12DescriptorHeap* heap = m_DescriptorAllocator->GetHeap(m_FontDescriptor.HeapIndex);
		d3d12Cmd->SetDescriptorHeaps(1, &heap);
	}
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d12Cmd);
}

ImTextureID D3D12ImGui::RegisterImGuiTexture(RHITexture* texture, RHISampler* sampler) {
	D3D12Texture* d3d12Texture = (D3D12Texture*)texture;
	StaticDescriptorHandle descriptor = m_DescriptorAllocator->AllocateDescriptorSlot();
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_DescriptorAllocator->GetCPUHandle(descriptor);
	m_Device->CopyDescriptorsSimple(1, cpuHandle, d3d12Texture->GetDescriptor(ETexDescriptorType::SRV), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_DescriptorAllocator->GetGPUHandle(descriptor);
	ImTextureID imID = (ImTextureID)gpuHandle.ptr;
	m_ImTextureDescriptorMap[imID] = descriptor;
	return imID;
}

void D3D12ImGui::RemoveImGuiTexture(ImTextureID textureID) {
	if(auto iter = m_ImTextureDescriptorMap.find(textureID); iter != m_ImTextureDescriptorMap.end()) {
		m_DescriptorAllocator->FreeDescriptorSlot(iter->second);
		m_ImTextureDescriptorMap.erase(iter);
	}
}
