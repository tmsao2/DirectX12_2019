#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <vector>
#include <DirectXMath.h>
#include <wrl.h>

struct PriVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
};

class Primitive
{
protected:
	
	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;
	//パイプラインステート
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState;

	Microsoft::WRL::ComPtr<ID3DBlob> _vsShader;
	Microsoft::WRL::ComPtr<ID3DBlob> _psShader;

	bool CreatePipeLine(Microsoft::WRL::ComPtr<ID3D12Device> dev, std::vector<D3D12_INPUT_ELEMENT_DESC> layout);
	bool CreateRootSignature(Microsoft::WRL::ComPtr<ID3D12Device> dev);

public:
	Primitive(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	~Primitive();
};

