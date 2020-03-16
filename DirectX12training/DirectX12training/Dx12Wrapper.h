#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <map>
#include <memory>
#include <array>
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
	XMFLOAT3 pos;//���W
	XMFLOAT2 uv;//UV
};

struct WVPMatrix {
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
	XMMATRIX lvp;
	XMVECTOR eye;
	XMVECTOR light;
};

struct GUIStatus
{
	unsigned int debug;
	unsigned int cameraOutLine;
	unsigned int normalOutLine;
	unsigned int bloomFlag;
	unsigned int dofFlag;
	unsigned int rayMarch;
	float time;
	XMFLOAT3 color;
};


class Dx12Wrapper
{
private:
	HWND _hwnd;
	//�f�o�C�X
	ComPtr<ID3D12Device> _dev;
	//�R�}���h�֘A
	ComPtr<ID3D12CommandAllocator> _cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> _cmdList;
	ComPtr<ID3D12CommandQueue> _cmdQue;
	//DXGI�֘A
	ComPtr<IDXGIFactory6> _dxgi;
	ComPtr<IDXGISwapChain4> _swapchain;
	//�҂��̂��߂̃t�F���X
	ComPtr<ID3D12Fence> _fence;
	UINT64 _fenceValue = 0;
	//�o�b�N�o�b�t�@�ƃt�����g�o�b�t�@
	std::vector<ComPtr<ID3D12Resource>> _renderTargets;
	//�f�X�N���v�^���i�[����̈�
	ComPtr<ID3D12DescriptorHeap> _rtvHeap;		//�����_�[�^�[�Q�b�g�r���[�p
	ComPtr<ID3D12DescriptorHeap> _dsvHeap;		//�[�x�X�e���V���r���[�p
	ComPtr<ID3D12DescriptorHeap> _depthSrvHeap;	//�J�����[�x�`��p
	ComPtr<ID3D12DescriptorHeap> _wvpHeap;		//�J�����p
	ComPtr<ID3D12DescriptorHeap> _boneHeap;		//�{�[���p
	ComPtr<ID3D12DescriptorHeap> _imguiHeap;	//IMGUI�p
	ComPtr<ID3D12DescriptorHeap> _statusHeap;	//GUI�X�e�[�^�X�p

	//�}���`�p�X�p
	ComPtr<ID3D12DescriptorHeap>	_peraRtvHeap;
	ComPtr<ID3D12DescriptorHeap>	_peraSrvHeap;
	std::array<ComPtr<ID3D12Resource>,3>	_resources;
	ComPtr<ID3D12RootSignature>		_peraSignature;
	ComPtr<ID3D12PipelineState>		_peraPipeline;
	D3D12_VERTEX_BUFFER_VIEW		_vb;
	ComPtr<ID3D12Resource> _vertexBuffer = nullptr;
	//�[�x�o�b�t�@�[�쐬
	ComPtr<ID3D12Resource> _depthBuffer = nullptr;

	//�e
	ComPtr<ID3D12DescriptorHeap>	_shadowDsvHeap;
	ComPtr<ID3D12DescriptorHeap>	_shadowSrvHeap;
	ComPtr<ID3D12Resource>			_shadowBuffer;
	//�u���[��,��ʊE�[�x
	ComPtr<ID3D12DescriptorHeap>	_shrinkRtvHeap;
	ComPtr<ID3D12DescriptorHeap>	_shrinkSrvHeap;
	std::array<ComPtr<ID3D12Resource>, 2>	_shrinkBuffer;
	ComPtr<ID3D12PipelineState>		_shrinkPipeline;
	
	//�V�F�[�_�[
	ComPtr<ID3DBlob> _peraVsShader = nullptr;
	ComPtr<ID3DBlob> _peraPsShader = nullptr;

	//�r���[�|�[�g
	D3D12_VIEWPORT _viewPort;
	//�V�U�[��`
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

	GUIStatus _status;
	GUIStatus* _mapStatus;
	float _angle = 0;
	float _oldangle = 0;
	int _instanceNum = 1;
	float col[3] = { 0,0,0 };
	XMFLOAT3 target;

	bool DeviceInit();
	bool CommandInit();
	bool SwapChainInit();
	bool RTInit();
	bool DepthInit();
	bool CameraInit();
	bool StatusInit();
	bool CreateResourceAndView();
	bool CreatePeraVertex();
	bool CreatePeraPipeline();
	bool CreatePeraSignature();
	bool CreateDepthTex();
	bool CreateShadow();
	bool CreateBloom();
	bool CreateEffect();

	void ExecuteCommand();
	void WaitFence();
	void CameraMove();
	void ChangeColor();
	void LightMove();
	void DrawShrink();
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();
	bool Init();
	void InitImgui(HWND hwnd);
	void Update();
};

