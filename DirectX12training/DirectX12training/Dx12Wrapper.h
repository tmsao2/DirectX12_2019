#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <map>
#include <memory>
#include <wrl.h>

using namespace DirectX;
using namespace Microsoft::WRL;

class PMXModel;
class PMDModel;
class VMDLoader;

struct Vertex {
	XMFLOAT3 pos;//���W
	XMFLOAT2 uv;//UV
};

struct WVPMatrix {
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
	XMFLOAT3 eye;
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
	//�f�o�C�X
	ID3D12Device* _dev;
	//�R�}���h�֘A
	ID3D12CommandAllocator* _cmdAllocator;
	ID3D12GraphicsCommandList* _cmdList;
	ID3D12CommandQueue* _cmdQue;
	//DXGI�֘A
	IDXGIFactory6* _dxgi;
	IDXGISwapChain4* _swapchain;
	//�҂��̂��߂̃t�F���X
	ID3D12Fence* _fence;
	UINT64 _fenceValue = 0;
	//�o�b�N�o�b�t�@�ƃt�����g�o�b�t�@
	std::vector<ID3D12Resource*> _renderTargets;
	//�f�X�N���v�^���i�[����̈�
	ID3D12DescriptorHeap* _rtvDescHeap;		//�����_�[�^�[�Q�b�g�r���[�p
	ID3D12DescriptorHeap* _dsvDescHeap;		//�[�x�X�e���V���r���[�p
	ID3D12DescriptorHeap* _cbvDescHeap;		//�萔�o�b�t�@�r���[�p
	ID3D12DescriptorHeap* _boneHeap;		//�{�[���p

	//�}���`�p�X�p
	ID3D12DescriptorHeap* _firstRtvHeap;
	ID3D12DescriptorHeap* _firstSrvHeap;
	ID3D12Resource* _firstResource;
	ID3D12RootSignature* _firstSignature;
	ID3D12PipelineState* _firstPipeline;
	D3D12_VERTEX_BUFFER_VIEW _vb;
	


	//���[�g�V�O�l�`��
	ID3D12RootSignature* _rootSignature = nullptr;
	//�p�C�v���C���X�e�[�g
	ID3D12PipelineState* _pipelineState = nullptr;
	//�V�F�[�_�[
	ID3DBlob* _vsShader = nullptr;
	ID3DBlob* _psShader = nullptr;
	//�r���[�|�[�g
	D3D12_VIEWPORT _viewPort;
	//�V�U�[��`
	D3D12_RECT _scissorRect;

	WVPMatrix _wvp;
	WVPMatrix* _mapWvp;
	std::map<std::string, ID3D12Resource*> _resTbl;

	std::vector<XMMATRIX> _boneMats;
	XMMATRIX* _mapedBone;
	std::map<std::string, BoneNode> _boneMap;

	std::shared_ptr<PMDModel> _pmd;
	std::shared_ptr<PMXModel> _pmx;
	std::shared_ptr<VMDLoader> _vmd;

	float _angle = 0;
	int _frame = 0;

	bool DeviceInit();
	bool CommandInit();
	bool SwapChainInit();
	bool RTInit();
	bool SignatureInit();
	bool PipelineStateInit();
	bool DepthInit();
	bool ConstantInit();
	bool BoneInit();
	bool Create1ResourceAndView();
	bool Create2ResourceAndView();
	bool CreatePeraVertex();
	bool CreatePeraPipeline();
	bool CreatePeraSignature();

	void ExecuteCommand();
	void WaitFence();
	void RecursiveMatrixMultiply(BoneNode& node, const XMMATRIX& inMat);
	void RotateBone(std::string boneName, XMVECTOR rot);
	void UpdateMotion(int frame);

public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();
	bool Init();
	void Update();
};

