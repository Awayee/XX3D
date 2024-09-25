#pragma once
#include "D3D12Util.h"

class D3D12DescriptorMgr;
class D3D12CommandMgr;
class D3D12Uploader;

class D3D12Device {
public:
	D3D12Device(IDXGIAdapter* adapter);
	~D3D12Device();
	D3D12DescriptorMgr* GetDescriptorMgr();
	D3D12CommandMgr* GetCommandMgr();
	D3D12Uploader* GetUploader();
	ID3D12Device* GetDevice();
private:
	TDXPtr<ID3D12Device> m_Device;
	TUniquePtr<D3D12DescriptorMgr> m_DescriptorMgr;
	TUniquePtr<D3D12Uploader> m_Uploader;
	TUniquePtr<D3D12CommandMgr> m_CommandMgr;
};
