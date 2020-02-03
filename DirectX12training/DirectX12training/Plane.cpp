#include "Plane.h"
#include <d3dx12.h>
#include <d3dcompiler.h>


Plane::Plane(Microsoft::WRL::ComPtr<ID3D12Device> dev) :Primitive(dev)
{
	InitVertex(dev);
	CreateRootSignature(dev);
	InitPipeLine(dev);
}


bool Plane::InitPipeLine(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	HRESULT result;
	result = D3DCompileFromFile(L"PrimitiveShader.hlsl", nullptr, nullptr, "PrimitiveVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_vsShader, nullptr);
	result = D3DCompileFromFile(L"PrimitiveShader.hlsl", nullptr, nullptr, "PrimitivePS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_psShader, nullptr);

	//レイアウト作成
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDesc = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	CreatePipeLine(dev, inputLayoutDesc);

	return false;
}

bool Plane::InitVertex(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	PriVertex vertex[] = {{{-25.0f, 0.0f,-25.0f},{0.0f,1.0f,0.0f},{0.0f,0.0f}},
						  {{-25.0f, 0.0f, 25.0f},{0.0f,1.0f,0.0f},{0.0f,1.0f}},
						  {{ 25.0f, 0.0f,-25.0f},{0.0f,1.0f,0.0f},{1.0f,0.0f}},
						  {{ 25.0f, 0.0f, 25.0f},{0.0f,1.0f,0.0f},{1.0f,1.0f}}};

	//頂点バッファ作成
	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertex)), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(_vertexBuffer.GetAddressOf()));

	PriVertex* vertMap = nullptr;
	result = _vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertex), std::end(vertex), vertMap);
	_vertexBuffer->Unmap(0, nullptr);

	//頂点バッファビューの作成
	_vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vbView.StrideInBytes = sizeof(PriVertex);
	_vbView.SizeInBytes = sizeof(vertex);
	return false;
}

Plane::~Plane()
{
}

void Plane::Draw(ID3D12Device * dev, ID3D12GraphicsCommandList * cmd, D3D12_VIEWPORT & view, D3D12_RECT & rect, ID3D12DescriptorHeap * wvp, ID3D12DescriptorHeap * shadow)
{
	//パイプラインステートの設定
	cmd->SetPipelineState(_pipelineState.Get());
	//ルートシグネチャの設定
	cmd->SetGraphicsRootSignature(_rootSignature.Get());
	//カメラ
	cmd->SetDescriptorHeaps(1, &wvp);
	auto h = wvp->GetGPUDescriptorHandleForHeapStart();
	cmd->SetGraphicsRootDescriptorTable(0, h);
	//ビューポートの設定
	cmd->RSSetViewports(1, &view);
	//シザー矩形の設定
	cmd->RSSetScissorRects(1, &rect);
	//頂点バッファビューの設定
	cmd->IASetVertexBuffers(0, 1, &_vbView);
	//影
	cmd->SetDescriptorHeaps(1, &shadow);
	cmd->SetGraphicsRootDescriptorTable(1, shadow->GetGPUDescriptorHandleForHeapStart());
	//トポロジの設定
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmd->DrawInstanced(4, 1, 0, 0);
}
