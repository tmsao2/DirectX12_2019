#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <map>
#include <memory>
#include <wrl.h>
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

using namespace DirectX;
using namespace Microsoft::WRL;

class PMXModel;
class PMDModel;
class Plane;
class Input;

struct Vertex {
	XMFLOAT3 pos;//座標
	XMFLOAT2 uv;//UV
};

struct WVPMatrix {
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
	XMMATRIX lvp;
	XMFLOAT3 eye;
};



class Dx12Wrapper
{
private:
	HWND _hwnd;
	//デバイス
	ComPtr<ID3D12Device> _dev;
	//コマンド関連
	ComPtr<ID3D12CommandAllocator> _cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> _cmdList;
	ComPtr<ID3D12CommandQueue> _cmdQue;
	//DXGI関連
	ComPtr<IDXGIFactory6> _dxgi;
	ComPtr<IDXGISwapChain4> _swapchain;
	//待ちのためのフェンス
	ComPtr<ID3D12Fence> _fence;
	UINT64 _fenceValue = 0;
	//バックバッファとフロントバッファ
	std::vector<ComPtr<ID3D12Resource>> _renderTargets;
	//デスクリプタを格納する領域
	ComPtr<ID3D12DescriptorHeap> _rtvHeap;		//レンダーターゲットビュー用
	ComPtr<ID3D12DescriptorHeap> _dsvHeap;		//深度ステンシルビュー用
	ComPtr<ID3D12DescriptorHeap> _depthSrvHeap;
	ComPtr<ID3D12DescriptorHeap> _wvpHeap;		//定数バッファビュー用
	ComPtr<ID3D12DescriptorHeap> _boneHeap;		//ボーン用

	//マルチパス用
	ComPtr<ID3D12DescriptorHeap>	_peraRtvHeap;
	ComPtr<ID3D12DescriptorHeap>	_peraSrvHeap;
	ComPtr<ID3D12Resource>			_resource;
	ComPtr<ID3D12RootSignature>		_peraSignature;
	ComPtr<ID3D12PipelineState>		_peraPipeline;
	D3D12_VERTEX_BUFFER_VIEW		_vb;
	ComPtr<ID3D12Resource> _vertexBuffer = nullptr;
	//深度バッファー作成
	ComPtr<ID3D12Resource> _depthBuffer = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	_shadowDsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	_shadowSrvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource>			_shadowBuffer;
	
	//シェーダー
	ComPtr<ID3DBlob> _peraVsShader = nullptr;
	ComPtr<ID3DBlob> _peraPsShader = nullptr;

	//ビューポート
	D3D12_VIEWPORT _viewPort;
	//シザー矩形
	D3D12_RECT _scissorRect;

	WVPMatrix _wvp;
	WVPMatrix* _mapWvp;

	std::shared_ptr<PMDModel>	_pmd1;
	std::shared_ptr<PMDModel>	_pmd2;
	std::shared_ptr<PMXModel>	_pmx;
	std::shared_ptr<Plane>		_plane; 
	std::shared_ptr<Input>		_input;

	Microsoft::WRL::ComPtr<EffekseerRenderer::Renderer>					_efkRenderer;
	Microsoft::WRL::ComPtr<Effekseer::Manager>							_efkManager;
	Microsoft::WRL::ComPtr<EffekseerRenderer::SingleFrameMemoryPool>	_efkMemoryPool;
	Microsoft::WRL::ComPtr<EffekseerRenderer::CommandList>				_efkCommandList;
	Microsoft::WRL::ComPtr<Effekseer::Effect>							_effect;
	Effekseer::Handle													_efkHandle;

	float _angle = 0;
	XMFLOAT3 target;

	bool DeviceInit();
	bool CommandInit();
	bool SwapChainInit();
	bool RTInit();
	bool DepthInit();
	bool ConstantInit();
	bool Create1ResourceAndView();
	bool Create2ResourceAndView();
	bool CreatePeraVertex();
	bool CreatePeraPipeline();
	bool CreatePeraSignature();
	bool CreateDepthTex();
	bool CreateShadow();
	bool CreateEffect();

	void ExecuteCommand();
	void WaitFence();
	void CameraMove();
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();
	bool Init();
	void Update();
};

