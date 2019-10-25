#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <map>
#include <memory>

using namespace DirectX;

class Model;
class PMXModel;
class PMDModel;

struct Vertex {
	XMFLOAT3 pos;//座標
	XMFLOAT3 normal;//法線
	XMFLOAT2 uv;//UV
};

struct WVPMatrix {
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
	XMFLOAT3 eye;
};

struct Material {
	XMFLOAT4 diffuse;
	float power;
	XMFLOAT3 specular;
	XMFLOAT3 ambient;
};

struct BoneNode {
	int boneIdx;
	XMFLOAT3 startPos;
	XMFLOAT3 endPos;
	std::vector<BoneNode*> children;
};

class Dx12Wrapper
{
private:
	HWND _hwnd;
	//デバイス
	ID3D12Device* _dev;
	//コマンド関連
	ID3D12CommandAllocator* _cmdAllocator;
	ID3D12GraphicsCommandList* _cmdList;
	ID3D12CommandQueue* _cmdQue;
	//DXGI関連
	IDXGIFactory6* _dxgi;
	IDXGISwapChain4* _swapchain;
	//待ちのためのフェンス
	ID3D12Fence* _fence;
	UINT64 _fenceValue = 0;
	//バックバッファとフロントバッファ
	std::vector<ID3D12Resource*> _renderTargets;
	//デスクリプタを格納する領域
	ID3D12DescriptorHeap* _rtvDescHeap;		//レンダーターゲットビュー用
	ID3D12DescriptorHeap* _dsvDescHeap;		//深度ステンシルビュー用
	ID3D12DescriptorHeap* _cbvDescHeap;		//定数バッファビュー用
	ID3D12DescriptorHeap* _materialHeap;	//マテリアル用
	ID3D12DescriptorHeap* _boneHeap;		//ボーン用

	//ルートシグネチャ
	ID3D12RootSignature* _rootSignature = nullptr;
	//パイプラインステート
	ID3D12PipelineState* _pipelineState = nullptr;
	//シェーダー
	ID3DBlob* _vsShader = nullptr;
	ID3DBlob* _psShader = nullptr;
	//ビューポート
	D3D12_VIEWPORT _viewPort;
	//シザー矩形
	D3D12_RECT _scissorRect;

	WVPMatrix _wvp;
	WVPMatrix* _mapWvp;
	Material mat;
	std::map<std::string, ID3D12Resource*> _resTbl;

	std::vector<XMMATRIX> _boneMats;
	XMMATRIX* _mapedBone;
	std::map<std::string, BoneNode> _boneMap;


	std::shared_ptr<PMDModel> _pmd;
	std::shared_ptr<PMXModel> _pmx;

	float _angle = 0;

	std::string GetTexPathFromModelAndTexPath(
				const std::string& modelPath,
				const char*			texPath);

	std::wstring GetWideStringFromString(const std::string str);

	std::pair<std::string,std::string> SplitFileName(const std::string& path,
													const char splitter = '*');
	std::string GetExtension(const std::string& path);

	ID3D12Resource* LoadTexture(std::string& texPath);
	ID3D12Resource* CreateWhiteTex();
	ID3D12Resource* CreateBlackTex();
	ID3D12Resource* CreateGradationTex();


	bool DeviceInit();
	bool CommandInit();
	bool SwapChainInit();
	bool RTInit();
	bool SignatureInit();
	bool PipelineStateInit();
	bool DepthInit();
	bool ConstantInit();
	bool MaterialInit();
	bool PMXMaterialInit();
	bool BoneInit();

	void ExecuteCommand();
	void WaitFence();
	void RecursiveMatrixMultiply(BoneNode& node, XMMATRIX& inMat);
	void RotateBone(std::string boneName, float angle);

public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();
	bool Init();
	void Update();
};

