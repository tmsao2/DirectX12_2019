#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"Effekseer.lib")
#pragma comment(lib,"EffekseerRendererDX12.lib")
#pragma comment(lib,"LLGI.lib")
#include "Dx12Wrapper.h"
#include "Application.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <algorithm>
#include "PMDModel.h"
#include "PMXModel.h"
#include "Plane.h"
#include "Input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"

Dx12Wrapper::Dx12Wrapper(HWND hwnd):_hwnd(hwnd)
{
}

bool Dx12Wrapper::DeviceInit()
{
	//�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1;
	HRESULT result = S_OK;
	for (auto lv : levels)
	{
		result = D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(_dev.GetAddressOf()));
		if (SUCCEEDED(result))
		{
			level = lv;
			break;
		}
	}
	if (_dev == nullptr)
	{
		return false;
	}
	return true;
}

bool Dx12Wrapper::CommandInit()
{
	//�R�}���h�L���[�̍쐬
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	auto result = _dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(_cmdQue.GetAddressOf()));
	
	//�R�}���h�A���P�[�^�[�̍쐬
	_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.GetAddressOf()));
	//�R�}���h���X�g�̍쐬
	_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.GetAddressOf()));
	_cmdList->Close();

	return true;
}

bool Dx12Wrapper::SwapChainInit()
{
	//DXGI�̏�����
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgi.GetAddressOf()));

	auto wsize = Application::Instance().GetWindowSize();
	//�X���b�v�`�F�[���̐ݒ�
	DXGI_SWAP_CHAIN_DESC1 swDesc = {};
	swDesc.Width = wsize.w;
	swDesc.Height = wsize.h;
	swDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swDesc.Stereo = false;
	swDesc.SampleDesc.Count = 1;
	swDesc.SampleDesc.Quality = 0;
	swDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swDesc.BufferCount = 2;
	swDesc.Scaling = DXGI_SCALING_STRETCH;
	swDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swDesc.Flags = 0;

	//�X���b�v�`�F�[���̏�����
	result = _dxgi->CreateSwapChainForHwnd(_cmdQue.Get(), _hwnd, &swDesc,
		nullptr, nullptr, (IDXGISwapChain1**)(_swapchain.GetAddressOf()));
	
	return true;
}

bool Dx12Wrapper::RTInit()
{
	//�f�X�N���v�^�q�[�v�̐ݒ�(�����_�[�^�[�Q�b�g�p)
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NumDescriptors = 2;
	heapDesc.NodeMask = 0;
	//�f�X�N���v�^�q�[�v�̏�����
	auto result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeap.GetAddressOf()));

	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();

	DXGI_SWAP_CHAIN_DESC swDesc = {};
	_swapchain->GetDesc(&swDesc);
	int renderTargetNum = swDesc.BufferCount;

	_renderTargets.resize(renderTargetNum);

	auto heapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�����_�[�^�[�Q�b�g�r���[�̏�����
	for (int i = 0; i < renderTargetNum; ++i)
	{
		result = _swapchain->GetBuffer(i, IID_PPV_ARGS(_renderTargets[i].GetAddressOf()));
		_dev->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, descriptorHandle);
		descriptorHandle.ptr += heapSize;
	}
	return true;
}

bool Dx12Wrapper::CreateResourceAndView()
{
	HRESULT result;
	auto heapDesc = _rtvHeap->GetDesc();

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Width = Application::Instance().GetWindowSize().w;
	desc.Height = Application::Instance().GetWindowSize().h;
	desc.MipLevels = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_CUSTOM;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	prop.VisibleNodeMask = 1;
	prop.CreationNodeMask = 1;

	XMStoreFloat3(&color,_status.color);
	D3D12_CLEAR_VALUE clear = 
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
		{ color.x,color.y,color.z,0.0f}
	};

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = desc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NumDescriptors = 3;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_peraRtvHeap.GetAddressOf()));

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_peraSrvHeap.GetAddressOf()));

	auto rtvH = _peraRtvHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvH = _peraSrvHeap->GetCPUDescriptorHandleForHeapStart();

	for (auto& resource : _resources)
	{
		result = _dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE,
			&desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clear, IID_PPV_ARGS(resource.GetAddressOf()));

		_dev->CreateRenderTargetView(resource.Get(), nullptr, rtvH);
		rtvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		_dev->CreateShaderResourceView(resource.Get(), &srvDesc, srvH);
		srvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	
	return true;
}

bool Dx12Wrapper::CreatePeraVertex()
{
	Vertex vertex[] = { {{-1.0f, 1.0f,0.1f},{0.0f,0.0f}},
						{{ 1.0f, 1.0f,0.1f},{1.0f,0.0f}},
						{{-1.0f,-1.0f,0.1f},{0.0f,1.0f}},
						{{ 1.0f,-1.0f,0.1f},{1.0f,1.0f}}};
	
	//���_�o�b�t�@�쐬
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertex)), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(_vertexBuffer.GetAddressOf()));

	Vertex* vertMap = nullptr;
	result = _vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertex), std::end(vertex), vertMap);
	_vertexBuffer->Unmap(0, nullptr);

	//���_�o�b�t�@�r���[�̍쐬
	_vb.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vb.StrideInBytes = sizeof(Vertex);
	_vb.SizeInBytes = sizeof(vertex);
	return false;
}

bool Dx12Wrapper::CreatePeraPipeline()
{
	//�V�F�[�_�[�̃R���p�C��
	auto result = D3DCompileFromFile(L"Pera.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_peraVsShader, nullptr);
	result = D3DCompileFromFile(L"Pera.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "ps", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_peraPsShader, nullptr);

	//���C�A�E�g�쐬
	D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _peraSignature.Get();
	gpsDesc.InputLayout.pInputElementDescs = inputLayoutDesc;
	gpsDesc.InputLayout.NumElements = _countof(inputLayoutDesc);
	//�V�F�[�_�[�n
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_peraVsShader.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_peraPsShader.Get());
	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;

	//���X�^���C�U
	//���X�^���C�U�̐ݒ�
	D3D12_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rsDesc.CullMode = D3D12_CULL_MODE_NONE;
	rsDesc.FrontCounterClockwise = false;
	rsDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rsDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rsDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rsDesc.DepthClipEnable = true;
	rsDesc.MultisampleEnable = false;
	rsDesc.AntialiasedLineEnable = false;
	rsDesc.ForcedSampleCount = 0;
	rsDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	gpsDesc.RasterizerState = rsDesc;

	//���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;

	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_peraPipeline.GetAddressOf()));
	if (FAILED(result))
	{
		return false;
	}

	result = D3DCompileFromFile(L"Pera.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BloomPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, _peraPsShader.GetAddressOf(), nullptr);

	gpsDesc.NumRenderTargets = 2;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_peraPsShader.Get());

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_shrinkPipeline.GetAddressOf()));

	return true;
}

bool Dx12Wrapper::CreatePeraSignature()
{
	//�T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC sampleDesc[1] = {};
	sampleDesc[0].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;//��Ԃ��Ȃ�
	sampleDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���J��Ԃ�
	sampleDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//�c�J��Ԃ�
	sampleDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���s�J��Ԃ�
	sampleDesc[0].MaxLOD = D3D12_FLOAT32_MAX;//�~�b�v�}�b�v�ő�l
	sampleDesc[0].MinLOD = 0.0f;//�~�b�v�}�b�v�ŏ��l
	sampleDesc[0].MipLODBias = 0.0f;
	sampleDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampleDesc[0].ShaderRegister = 0;
	sampleDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	sampleDesc[0].RegisterSpace = 0;
	sampleDesc[0].MaxAnisotropy = 0;
	sampleDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	//�����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE range[5] = {};
	//�V�F�[�_�[���\�[�X�r���[
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 4;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//�J�����[�x
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[1].BaseShaderRegister = 4;
	range[1].NumDescriptors = 1;
	range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//���C�g�[�x
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[2].BaseShaderRegister = 5;
	range[2].NumDescriptors = 1;
	range[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//�k���o�b�t�@
	range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[3].BaseShaderRegister = 6;
	range[3].NumDescriptors = 2;
	range[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//GUI�̃X�e�[�^�X
	range[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[4].BaseShaderRegister = 0;
	range[4].NumDescriptors = 1;
	range[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	
	//���[�g�p�����[�^�[�̐ݒ�
	D3D12_ROOT_PARAMETER rootParam[5] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].DescriptorTable.pDescriptorRanges = &range[0];
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[1].DescriptorTable.pDescriptorRanges = &range[1];
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[2].DescriptorTable.pDescriptorRanges = &range[2];
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[3].DescriptorTable.pDescriptorRanges = &range[3];
	rootParam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParam[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[4].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[4].DescriptorTable.pDescriptorRanges = &range[4];
	rootParam[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	//���[�g�V�O�l�`���̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = _countof(rootParam);
	rsd.pParameters = rootParam;
	rsd.NumStaticSamplers = _countof(sampleDesc);
	rsd.pStaticSamplers = sampleDesc;

	//�V�O�l�`���A�G���[�̏�����
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	//���[�g�V�O�l�`���̐���
	result = _dev->CreateRootSignature(0, signature->GetBufferPointer(),
		signature->GetBufferSize(), IID_PPV_ARGS(&_peraSignature));

	if (FAILED(result))
	{
		return false;
	}
	return true;
}

bool Dx12Wrapper::CreateDepthTex()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	auto result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_depthSrvHeap.GetAddressOf()));

	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_R32_FLOAT;
	desc.Texture2D.MipLevels = 1;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	_dev->CreateShaderResourceView(_depthBuffer.Get(), &desc, _depthSrvHeap->GetCPUDescriptorHandleForHeapStart());

	return false;
}

bool Dx12Wrapper::CreateShadow()
{
	HRESULT result;
	auto wsize = Application::Instance().GetWindowSize();
	auto size = max(wsize.w, wsize.h);

	D3D12_RESOURCE_DESC desc = {};
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.DepthOrArraySize = 1;
	desc.Width = wsize.w;
	desc.Height = wsize.h;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Alignment = 0;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.VisibleNodeMask = 1;
	prop.CreationNodeMask = 1;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;

	result = _dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue, IID_PPV_ARGS(_shadowBuffer.GetAddressOf()));

	//DSV�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_shadowDsvHeap.GetAddressOf()));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvdesc = {};
	dsvdesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvdesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView(_shadowBuffer.Get(), &dsvdesc, _shadowDsvHeap->GetCPUDescriptorHandleForHeapStart());

	//SRV�q�[�v�쐬
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_shadowSrvHeap.GetAddressOf()));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	_dev->CreateShaderResourceView(_shadowBuffer.Get(), &srvDesc, _shadowSrvHeap->GetCPUDescriptorHandleForHeapStart());
	return false;
}

bool Dx12Wrapper::CreateBloom()
{
	HRESULT result;
	auto wsize = Application::Instance().GetWindowSize();

	//RTV�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_shrinkRtvHeap.GetAddressOf()));

	//SRV�q�[�v�쐬
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_shrinkSrvHeap.GetAddressOf()));

	D3D12_RENDER_TARGET_VIEW_DESC rtvdesc = {};
	rtvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_RESOURCE_DESC desc = {};
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.DepthOrArraySize = 1;
	desc.Width = wsize.w / 2;
	desc.Height = wsize.h;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Alignment = 0;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.VisibleNodeMask = 1;
	prop.CreationNodeMask = 1;

	D3D12_CLEAR_VALUE clearValue =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
		{ 0.0f,0.0f,0.0f,1.0f }
	};

	auto rtvH = _shrinkRtvHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvH = _shrinkSrvHeap->GetCPUDescriptorHandleForHeapStart();

	for (auto& buffer : _shrinkBuffer)
	{
		result = _dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE,
			&desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue, IID_PPV_ARGS(buffer.GetAddressOf()));

		_dev->CreateRenderTargetView(buffer.Get(), nullptr, rtvH);
		rtvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		_dev->CreateShaderResourceView(buffer.Get(), &srvDesc, srvH);
		srvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	return false;
}

bool Dx12Wrapper::CreateEffect()
{
	DXGI_FORMAT bbFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	_efkRenderer = EffekseerRendererDX12::Create
	(_dev.Get(),	//�f�o�C�X
		_cmdQue.Get(),	//�R�}���h�L���[
		2,				//�X���b�v�`�F�[����(�o�b�N�o�b�t�@�̐�)
		&bbFormat,		//�o�b�t�@�̃t�H�[�}�b�g
		1,				//�����_�[�^�[�Q�b�g��
		false,			//�f�v�X�L���t���O
		false,			//���]�f�v�X�L���t���O
		2000);			//�p�[�e�B�N����

	_efkManager = Effekseer::Manager::Create(10000);
	_efkManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

	_efkManager->SetSpriteRenderer(_efkRenderer->CreateSpriteRenderer());
	_efkManager->SetRibbonRenderer(_efkRenderer->CreateRibbonRenderer());
	_efkManager->SetRingRenderer(_efkRenderer->CreateRingRenderer());
	_efkManager->SetTrackRenderer(_efkRenderer->CreateTrackRenderer());
	_efkManager->SetModelRenderer(_efkRenderer->CreateModelRenderer());

	_efkManager->SetTextureLoader(_efkRenderer->CreateTextureLoader());
	_efkManager->SetModelLoader(_efkRenderer->CreateModelLoader());
 
	_efkMemoryPool = EffekseerRendererDX12::CreateSingleFrameMemoryPool(_efkRenderer.Get());
	_efkCommandList = EffekseerRendererDX12::CreateCommandList(_efkRenderer.Get(), _efkMemoryPool.Get());

	_efkRenderer->SetCommandList(_efkCommandList.Get());

	auto size = Application::Instance().GetWindowSize();

	_efkRenderer->SetProjectionMatrix(Effekseer::Matrix44().PerspectiveFovLH(30.0f / 180.0f * 3.14f, size.w / size.h, 1.0f, 100.0f));

	_efkRenderer->SetCameraMatrix(
		Effekseer::Matrix44().LookAtLH(Effekseer::Vector3D(0.0f, 20.0f, -30.0f),
										Effekseer::Vector3D(target.x, target.y, target.z),
										Effekseer::Vector3D(0.0f, 1.0f, 0.0f)));

	_effect = Effekseer::Effect::Create(_efkManager.Get(), (const EFK_CHAR*)L"effect/test.efk");

	return false;
}

void Dx12Wrapper::InitImgui(HWND hwnd)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_imguiHeap.GetAddressOf()));

	ImGui::CreateContext();
	auto imguiresult = ImGui_ImplWin32_Init(hwnd);
	assert(imguiresult);
	imguiresult = ImGui_ImplDX12_Init(_dev.Get(), 2, DXGI_FORMAT_R8G8B8A8_UNORM,
		_imguiHeap.Get(),
		_imguiHeap->GetCPUDescriptorHandleForHeapStart(),
		_imguiHeap->GetGPUDescriptorHandleForHeapStart());
	assert(imguiresult);
}

bool Dx12Wrapper::DepthInit()
{
	auto wsize = Application::Instance().GetWindowSize();

	//���\�[�X�ݒ�
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = wsize.w;
	resDesc.Height = wsize.h;
	resDesc.DepthOrArraySize = 1;
	resDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	resDesc.SampleDesc.Count = 1;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue, IID_PPV_ARGS(_depthBuffer.GetAddressOf()));

	//�f�X�N���v�^�q�[�v�ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	//�q�[�v�쐬
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_dsvHeap.GetAddressOf()));
	//�[�x�X�e���V���r���[�ݒ�
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	auto h = _dsvHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateDepthStencilView(_depthBuffer.Get(), &dsvDesc, h);
	return true;
}

bool Dx12Wrapper::CameraInit()
{
	auto wsize = Application::Instance().GetWindowSize();

	XMFLOAT3 eye(0, 20, -30);	//���_
	XMFLOAT3 up(0, 1, 0);		//��x�N�g��
	target = XMFLOAT3(0, 10, 0);  //�����_

	//����n�A�r���[�s��
	_wvp.view = XMMatrixLookAtLH(
		XMLoadFloat3(&eye),
		XMLoadFloat3(&target),
		XMLoadFloat3(&up));

	//�v���W�F�N�V�����s��
	//����p�A�A�X�y�N�g��A�j�A�A�t�@�[
	_wvp.projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(wsize.w) / static_cast<float>(wsize.h),
		0.1f,
		1000
	);

	XMFLOAT3 light(50, 50, 50);
	XMFLOAT3 tolight(1, 1, 1);
	auto lightview=XMMatrixLookAtLH(XMLoadFloat3(&light), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto lightproj= XMMatrixOrthographicLH(40, 40, 0.1f, 1000);
	_wvp.lvp = lightview * lightproj;
	_wvp.light = XMLoadFloat3(&tolight);

	//���[���h�s��
	_wvp.world = XMMatrixIdentity();

	_wvp.eye = XMLoadFloat3(&eye);

	size_t size = sizeof(_wvp);
	size = (size + 0xff) & ~0xff;

	ID3D12Resource* _cbBuffer = nullptr;

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_cbBuffer));

	result = _cbBuffer->Map(0, nullptr, (void**)&_mapWvp);
	*_mapWvp = _wvp;
	_cbBuffer->Unmap(0, nullptr);

	//�R���X�^���g�o�b�t�@�r���[�̐ݒ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _cbBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;

	//�f�X�N���v�^�q�[�v�̐ݒ�(�e�N�X�`���p)
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NumDescriptors = 1;
	heapDesc.NodeMask = 0;

	//�f�X�N���v�^�q�[�v�̏�����
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_wvpHeap));

	auto h = _wvpHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateConstantBufferView(&cbvDesc, h);

	return true;
}

bool Dx12Wrapper::StatusInit()
{
	_status.debug = 0;
	_status.cameraOutLine = 0;
	_status.normalOutLine = 0;
	_status.bloomFlag = 0;
	_status.dofFlag = 0;
	_status.time = 0;
	_status.rayMarch = 0;
	ChangeColor();

	size_t size = sizeof(_status);
	size = (size + 0xff) & ~0xff;

	ID3D12Resource* _cbBuffer = nullptr;

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_cbBuffer));

	result = _cbBuffer->Map(0, nullptr, (void**)&_mapStatus);
	*_mapStatus = _status;

	//�R���X�^���g�o�b�t�@�r���[�̐ݒ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _cbBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;

	//�f�X�N���v�^�q�[�v�̐ݒ�(�e�N�X�`���p)
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NumDescriptors = 1;
	heapDesc.NodeMask = 0;

	//�f�X�N���v�^�q�[�v�̏�����
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_statusHeap.GetAddressOf()));

	auto h = _statusHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateConstantBufferView(&cbvDesc, h);

	return true;
}

Dx12Wrapper::~Dx12Wrapper()
{
}

bool Dx12Wrapper::Init()
{
	HRESULT result = S_OK;

	//�f�o�b�O���C���[�̋N��
	ID3D12Debug* debugLayer;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
	{
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}

	DeviceInit();
	CommandInit();
	SwapChainInit();
	RTInit();
	CreateResourceAndView();
	CreatePeraVertex();
	CreatePeraSignature();
	CreatePeraPipeline();
	DepthInit();
	CreateDepthTex();
	CreateShadow();
	CreateBloom();
	CameraInit();
	StatusInit();
	CreateEffect();

	_dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.GetAddressOf()));

	//_pmd1.reset(new PMDModel(_dev.Get(), "model/�����~�N.pmd"));
	_pmd1.reset(new PMDModel(_dev.Get(), "model/�����~�Nmetal.pmd"));
	//_pmd2.reset(new PMDModel(_dev.Get(), "model/��������.pmd"));
	//_pmd.reset(new PMDModel(_dev.Get(), "model/��������.pmd"));
	//_pmd.reset(new PMDModel(_dev.Get(), "model/�������J.pmd"));
	//_pmd.reset(new PMDModel(_dev.Get(), "model/�特���C�R.pmd"));
	//_pmd.reset(new PMDModel(_dev.Get(), "model/�㉹�n�N.pmd"));
	//_pmd.reset(new PMDModel(_dev.Get(), "model/���k�l��.pmd"));
	//_pmx.reset(new PMXModel(_dev.Get(),"model/�h�[��/�h�[��.pmx"));
	//_pmx.reset(new PMXModel(_dev.Get(), "model/���_�����/���_�����.pmx"));
	//_pmx.reset(new PMXModel(_dev.Get(), "model/�L��������/�L��������.pmx"));
	//_pmx.reset(new PMXModel(_dev.Get(),"model/�V�X�^�[�N���A/�V�X�^�[�N���A.pmx"));
	_plane.reset(new Plane(_dev.Get()));

	_input.reset(new Input());

	auto wsize = Application::Instance().GetWindowSize();
	//�r���[�|�[�g�ݒ�
	_viewPort.TopLeftX = 0;
	_viewPort.TopLeftY = 0;
	_viewPort.Width = wsize.w;
	_viewPort.Height = wsize.h;
	_viewPort.MaxDepth = 1.0f;
	_viewPort.MinDepth = 0.0f;
	//�V�U�[��`�ݒ�
	_scissorRect.left = 0;
	_scissorRect.top = 0;
	_scissorRect.right = wsize.w;
	_scissorRect.bottom = wsize.h;

	return true;
}

void Dx12Wrapper::Update()
{
	_input->Update();
	_pmd1->Update();
	//_pmd2->Update();
	//_pmx->Update();
	CameraMove();
	LightMove();
	ChangeColor();
	_oldangle = _angle;
	*_mapStatus = _status;

	if (_input->GetKey()[VK_SPACE] & 0x80)
	{
		if (_efkManager->Exists(_efkHandle))
		{
			_efkManager->StopEffect(_efkHandle);
		}
		_efkHandle = _efkManager->Play(_effect.Get(), Effekseer::Vector3D(0, 10, 0));
		_efkManager->SetScale(_efkHandle, 3, 3, 3);
	}
	
	_cmdAllocator->Reset();//�A���P�[�^���Z�b�g
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);//�R�}���h���X�g���Z�b�g
	float clearColor[] = { color.x,color.y,color.z,0.0f };

	//�o���A�̐ݒ�
	D3D12_RESOURCE_BARRIER barrierDesc{};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.Transition.pResource = _shadowBuffer.Get();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	_cmdList->ResourceBarrier(1, &barrierDesc);
	
	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(0, nullptr, false, &_shadowDsvHeap->GetCPUDescriptorHandleForHeapStart());
	//�f�v�X�X�e���V���r���[�̏�����
	_cmdList->ClearDepthStencilView(_shadowDsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	//�e�̕`��
	_pmd1->ShadowDraw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect, _wvpHeap.Get() ,_instanceNum);
	//_pmx->ShadowDraw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect, _wvpHeap.Get(), _instanceNum);

	//���\�[�X�o���A(�V�F�[�_�[���\�[�X�Ƃ��Ďg��)
	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_shadowBuffer.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	for (auto& buffer : _shrinkBuffer)
	{
		_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}
	
	DrawShrink();

	for (auto& buffer : _shrinkBuffer)
	{
		_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();

	//�o���A�̐ݒ�
	for (auto& resource : _resources)
	{
		_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}
	
	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(3, &_peraRtvHeap->GetCPUDescriptorHandleForHeapStart(), true, &dsvH);
	//�N���A
	auto rtvH = _peraRtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto& resource : _resources)
	{
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		rtvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	//�f�v�X�X�e���V���r���[�̏�����
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	
	//���f���`��
	_pmd1->Draw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect,_wvpHeap.Get(),_shadowSrvHeap.Get(), _instanceNum);
	//_pmd2->Draw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect, _wvpHeap.Get(),_shadowSrvHeap.Get(), _instanceNum);
	//_pmx->Draw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect, _wvpHeap.Get(), _shadowSrvHeap.Get(),_instanceNum);

	//�G�t�F�N�g
	_efkManager->Update();
	_efkMemoryPool->NewFrame();
	EffekseerRendererDX12::BeginCommandList(_efkCommandList.Get(), _cmdList.Get());
	_efkRenderer->BeginRendering();
	_efkManager->Draw();
	_efkRenderer->EndRendering();
	EffekseerRendererDX12::EndCommandList(_efkCommandList.Get());
	//��
	_plane->Draw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect, _wvpHeap.Get(), _shadowSrvHeap.Get());


	//���\�[�X�o���A(�V�F�[�_�[���\�[�X�Ƃ��Ďg��)
	for (auto& resource : _resources)
	{
		_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}
	
	//�p�C�v���C���X�e�[�g�̐ݒ�
	_cmdList->SetPipelineState(_peraPipeline.Get());
	//���[�g�V�O�l�`���̐ݒ�
	_cmdList->SetGraphicsRootSignature(_peraSignature.Get());

	//�r���[�|�[�g�̐ݒ�
	_cmdList->RSSetViewports(1, &_viewPort);
	//�V�U�[��`�̐ݒ�
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	auto bbIndex = _swapchain->GetCurrentBackBufferIndex();
	auto heapStart = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
	auto heapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	heapStart.ptr = heapStart.ptr + bbIndex * heapSize;

	barrierDesc.Transition.pResource = _renderTargets[bbIndex].Get();
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	_cmdList->ResourceBarrier(1, &barrierDesc);

	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);
	//�N���A
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);
	//�J�����[�x
	_cmdList->SetDescriptorHeaps(1, _depthSrvHeap.GetAddressOf());
	auto depthSrvH = _depthSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(1, depthSrvH);
	//���C�g�[�x
	_cmdList->SetDescriptorHeaps(1, _shadowSrvHeap.GetAddressOf());
	auto shadowSrvH = _shadowSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(2, shadowSrvH);
	//�k���o�b�t�@
	_cmdList->SetDescriptorHeaps(1, _shrinkSrvHeap.GetAddressOf());
	auto bloomSrvH = _shrinkSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(3, bloomSrvH);
	//GUI�X�e�[�^�X
	_cmdList->SetDescriptorHeaps(1, _statusHeap.GetAddressOf());
	auto statusH = _statusHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(4, statusH);

	//���_�o�b�t�@�r���[�̐ݒ�
	_cmdList->IASetVertexBuffers(0, 1, &_vb);
	//�g�|���W�̐ݒ�
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//���܂ł̕`�挋�ʂ��y���|����
	_cmdList->SetDescriptorHeaps(1, _peraSrvHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(0, _peraSrvHeap->GetGPUDescriptorHandleForHeapStart());

	
	_cmdList->DrawInstanced(4, 1, 0, 0);

	/////////////////////////////imgui///////////////////////////////////////////////////
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowSize(ImVec2(400, 400));
	ImGui::Begin("GUI");
	ImGui::ColorPicker3("Color1", col);
	ImGui::SliderInt("InstanceNum", &_instanceNum, 1, 25);
	ImGui::SliderAngle("LightAngle", &_angle, -180, 180, "%.02f");
	ImGui::CheckboxFlags("Debug", &_status.debug,1);
	ImGui::CheckboxFlags("CameraOutLine", &_status.cameraOutLine,1);
	ImGui::CheckboxFlags("NormalOutLine", &_status.normalOutLine,1);
	ImGui::CheckboxFlags("Bloom", &_status.bloomFlag,1);
	ImGui::CheckboxFlags("Dof", &_status.dofFlag,1);
	ImGui::CheckboxFlags("RayMarch", &_status.rayMarch,1);
	ImGui::End();
	ImGui::Render();
	_cmdList->SetDescriptorHeaps(1, _imguiHeap.GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _cmdList.Get());

	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[bbIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	_cmdList->Close();//�R�}���h�̃N���[�Y

	ExecuteCommand();
	WaitFence();
	_status.time+=0.01;

	_swapchain->Present(0,0);
}

void Dx12Wrapper::ExecuteCommand()
{
	ID3D12CommandList* cmdLists[] = { _cmdList.Get()};
	_cmdQue->ExecuteCommandLists(1, cmdLists);
	_cmdQue->Signal(_fence.Get(), ++_fenceValue);
}

void Dx12Wrapper::WaitFence()
{
	if(_fence->GetCompletedValue()!=_fenceValue)
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceValue, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
}

void Dx12Wrapper::CameraMove()
{
	XMFLOAT3 up(0, 1, 0);		//��x�N�g��
	float speed = 0.05f;

	if (_input->GetKey()['W'] & 0x80)
	{
		auto mat = XMMatrixTranslation(0, speed, 0);
		_wvp.eye = XMVector3Transform(_wvp.eye, mat);
		target = XMFLOAT3(target.x, target.y + speed, target.z);
	}
	if (_input->GetKey()['A'] & 0x80)
	{
		auto mat = XMMatrixTranslation(-speed, 0, 0);
		_wvp.eye = XMVector3Transform(_wvp.eye, mat);
		target = XMFLOAT3(target.x - speed, target.y, target.z);
	}
	if (_input->GetKey()['S'] & 0x80)
	{
		auto mat = XMMatrixTranslation(0, -speed, 0);
		_wvp.eye = XMVector3Transform(_wvp.eye, mat);
		target = XMFLOAT3(target.x, target.y - speed, target.z);
	}
	if (_input->GetKey()['D'] & 0x80)
	{
		auto mat = XMMatrixTranslation(speed, 0, 0);
		_wvp.eye = XMVector3Transform(_wvp.eye, mat);
		target = XMFLOAT3(target.x + speed, target.y, target.z);
	}
	if (_input->GetKey()[VK_UP] & 0x80)
	{
		auto mat = XMMatrixTranslation(0, 0, speed);
		_wvp.eye = XMVector3Transform(_wvp.eye, mat);
		target = XMFLOAT3(target.x, target.y, target.z + speed);
	}
	if (_input->GetKey()[VK_DOWN] & 0x80)
	{
		auto mat = XMMatrixTranslation(0, 0, -speed);
		_wvp.eye = XMVector3Transform(_wvp.eye, mat);		
		target = XMFLOAT3(target.x, target.y, target.z - speed);
	}
	if (_input->GetKey()[VK_LEFT] & 0x80)
	{
		auto mat = XMMatrixRotationY(speed);
		
		_wvp.eye = XMVector3Transform(_wvp.eye, mat);
	}
	if (_input->GetKey()[VK_RIGHT] & 0x80)
	{
		auto mat = XMMatrixRotationY(-speed);

		_wvp.eye = XMVector3Transform(_wvp.eye, mat);
	}

	_wvp.view = XMMatrixLookAtLH(
		_wvp.eye,
		XMLoadFloat3(&target),
		XMLoadFloat3(&up));
	
	*_mapWvp = _wvp;
}

void Dx12Wrapper::ChangeColor()
{
	color = XMFLOAT3(col[0], col[1], col[2]);
	_status.color = XMLoadFloat3(&color);
}

void Dx12Wrapper::LightMove()
{
	XMFLOAT3 up(0, 1, 0);		//��x�N�g��

	if (_angle != _oldangle)
	{
		XMFLOAT3 light(50, 50, 50);
		auto Vlight = XMLoadFloat3(&light);
		auto mat = XMMatrixRotationY(_angle);
		Vlight = XMVector3Transform(Vlight, mat);
		auto lightview = XMMatrixLookAtLH(Vlight, XMLoadFloat3(&target), XMLoadFloat3(&up));
		auto lightproj = XMMatrixOrthographicLH(40, 40, 0.1f, 1000);
		_wvp.lvp = lightview * lightproj;
		_wvp.light = Vlight / 50;
	}
}

void Dx12Wrapper::DrawShrink()
{
	float clearColor[] = { color.x,color.y,color.z,0.0f };
	_cmdList->SetPipelineState(_shrinkPipeline.Get());
	_cmdList->SetGraphicsRootSignature(_peraSignature.Get());

	//���_�o�b�t�@�r���[�̐ݒ�
	_cmdList->IASetVertexBuffers(0, 1, &_vb);
	//�g�|���W�̐ݒ�
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	auto rtvH = _shrinkRtvHeap->GetCPUDescriptorHandleForHeapStart();
	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(2, &rtvH, true, nullptr);
	for (auto& buffer : _shrinkBuffer)
	{
		//�N���A
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		rtvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	_cmdList->SetDescriptorHeaps(1, _peraSrvHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(0, _peraSrvHeap->GetGPUDescriptorHandleForHeapStart());
	auto desc = _shrinkBuffer[0]->GetDesc();
	D3D12_VIEWPORT vp = {};
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;
	vp.Width = desc.Width;
	vp.Height = desc.Height/2;
	vp.TopLeftX = vp.TopLeftY = 0;
	D3D12_RECT rc = {};
	rc.top = 0;
	rc.left = 0;
	rc.right = vp.Width;
	rc.bottom = vp.Height;

	for (int i = 0; i < 8; ++i)
	{
		//�r���[�|�[�g�̐ݒ�
		_cmdList->RSSetViewports(1, &vp);
		//�V�U�[��`�̐ݒ�
		_cmdList->RSSetScissorRects(1, &rc);
		_cmdList->DrawInstanced(4, 1, 0, 0);

		rc.top += vp.Height;
		vp.TopLeftX = 0;
		vp.TopLeftY = rc.top;
		vp.Width /= 2;
		vp.Height /= 2;
		rc.right = rc.left + vp.Width;
		rc.bottom = rc.top + vp.Height;
	}
}

