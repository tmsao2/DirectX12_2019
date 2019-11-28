#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <memory>
#include <map>
#include <wrl.h>


class VMDLoader;
class TextureResource;

struct BoneNode {
	int boneIdx;
	DirectX::XMFLOAT3 startPos;
	DirectX::XMFLOAT3 endPos;
	std::vector<BoneNode*> children;
};

class Model
{
protected:
	//頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	//インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> _shadowSignature;

	//パイプラインステート
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _shadowPipeline;

	Microsoft::WRL::ComPtr<ID3DBlob> _vsShader;
	Microsoft::WRL::ComPtr<ID3DBlob> _psShader;
	Microsoft::WRL::ComPtr<ID3DBlob> _shadowVS;
	Microsoft::WRL::ComPtr<ID3DBlob> _shadowPS;

	std::shared_ptr<TextureResource> _tex;
	std::shared_ptr<VMDLoader> _vmd;
	std::vector<DirectX::XMMATRIX> _boneMats;
	DirectX::XMMATRIX* _mapedBone;
	std::map<std::string, BoneNode> _boneMap;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _materialHeap;	//マテリアル用
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _boneHeap;		//ボーン用

	Microsoft::WRL::ComPtr<ID3D12Resource> _vertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _boneBuffer = nullptr;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> materialBuffer;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> texResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> toonResources;

	Microsoft::WRL::ComPtr<ID3D12Resource> whiteBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> blackBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> gradBuffer;

	float GetYFromXOnBezier(float x, DirectX::XMFLOAT2 p1, DirectX::XMFLOAT2 p2, int cnt);
	void RecursiveMatrixMultiply(BoneNode & node, const DirectX::XMMATRIX & inMat);
	void RotateBone(std::string boneName, DirectX::XMVECTOR rot);
	void UpdateMotion(int frame);
	bool CreatePipeLine(Microsoft::WRL::ComPtr<ID3D12Device> dev, std::vector<D3D12_INPUT_ELEMENT_DESC> layout);
	bool CreateRootSignature(Microsoft::WRL::ComPtr<ID3D12Device> dev);
public:
	Model(Microsoft::WRL::ComPtr<ID3D12Device> dev, const char* path);
	~Model();
};

