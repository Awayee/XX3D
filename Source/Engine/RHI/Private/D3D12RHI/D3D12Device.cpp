#include "D3D12Device.h"
#include "D3D12Descriptor.h"
#include "D3D12Command.h"

D3D12Device::D3D12Device(IDXGIAdapter* adapter) {
	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_Device.Address()));
	if (FAILED(hardwareResult)) {
		LOG_ERROR("Falied to create D3D12 device!");
	}
	m_DescriptorMgr.Reset(new D3D12DescriptorMgr(m_Device));
	m_Uploader.Reset(new D3D12Uploader(m_Device));
	m_CommandMgr.Reset(new D3D12CommandMgr(this));
}

D3D12Device::~D3D12Device() {
}

D3D12DescriptorMgr* D3D12Device::GetDescriptorMgr() {
	return m_DescriptorMgr.Get();
}

D3D12CommandMgr* D3D12Device::GetCommandMgr() {
	return m_CommandMgr.Get();
}

D3D12Uploader* D3D12Device::GetUploader() {
	return m_Uploader.Get();
}

ID3D12Device* D3D12Device::GetDevice() {
	return m_Device.Get();
}
