#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#include "Dx12Wrapper.h"
#include "Application.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <algorithm>
#include "PMDModel.h"
#include "PMXModel.h"
#include "VMDLoader.h"
#include "TextureResource.h"

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
		result = D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev));
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

	auto result = _dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&_cmdQue));
	
	//�R�}���h�A���P�[�^�[�̍쐬
	_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	//�R�}���h���X�g�̍쐬
	_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	_cmdList->Close();

	return true;
}

bool Dx12Wrapper::SwapChainInit()
{
	//DXGI�̏�����
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgi));

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
	result = _dxgi->CreateSwapChainForHwnd(_cmdQue, _hwnd, &swDesc,
		nullptr, nullptr, (IDXGISwapChain1**)(&_swapchain));
	
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
	auto result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_rtvDescHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();

	DXGI_SWAP_CHAIN_DESC swDesc = {};
	_swapchain->GetDesc(&swDesc);
	int renderTargetNum = swDesc.BufferCount;

	_renderTargets.resize(renderTargetNum);

	auto heapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�����_�[�^�[�Q�b�g�r���[�̏�����
	for (int i = 0; i < renderTargetNum; ++i)
	{
		result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i]));
		_dev->CreateRenderTargetView(_renderTargets[i], nullptr, descriptorHandle);
		descriptorHandle.ptr += heapSize;
	}
	return true;
}

bool Dx12Wrapper::Create1ResourceAndView()
{
	auto heapDesc = _rtvDescHeap->GetDesc();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	heapDesc.NumDescriptors = 1;
	_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_firstRtvHeap));
	_dev->CreateRenderTargetView(_firstResource, nullptr, _firstRtvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format=rtvDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_firstSrvHeap));
	_dev->CreateShaderResourceView(_firstResource, &srvDesc, _firstSrvHeap->GetCPUDescriptorHandleForHeapStart());
	return true;
}

bool Dx12Wrapper::Create2ResourceAndView()
{
	return false;
}

bool Dx12Wrapper::CreatePeraVertex()
{
	Vertex vertex[] = { {{-1, 1,0.1},{1,1}},
						{{-1,-1,0.1},{0,1}},
						{{ 1, 1,0.1},{1,0}},
						{{ 1,-1,0.1},{0,0}} };
	
	//���_�o�b�t�@�쐬
	ID3D12Resource* vertexBuffer = nullptr;
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertex)), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&vertexBuffer));

	Vertex* vertMap = nullptr;
	result = vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertex, vertex, vertMap);
	vertexBuffer->Unmap(0, nullptr);

	//���_�o�b�t�@�r���[�̍쐬
	_vb.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	_vb.StrideInBytes = sizeof(Vertex);
	_vb.SizeInBytes = sizeof(vertex);
	return false;
}

bool Dx12Wrapper::CreatePeraPipeline()
{
	//���C�A�E�g�쐬
	D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = inputLayoutDesc;
	gpsDesc.InputLayout.NumElements = _countof(inputLayoutDesc);
	//�V�F�[�_�[�n
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_vsShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_psShader);
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

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_firstPipeline));
	if (FAILED(result))
	{
		return false;
	}

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
	D3D12_DESCRIPTOR_RANGE range[1] = {};
	//�V�F�[�_�[���\�[�X�r���[
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	
	//���[�g�p�����[�^�[�̐ݒ�
	D3D12_ROOT_PARAMETER rootParam[1] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].DescriptorTable.pDescriptorRanges = &range[0];
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	//���[�g�V�O�l�`���̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = 1;
	rsd.pParameters = rootParam;
	rsd.NumStaticSamplers = 1;
	rsd.pStaticSamplers = sampleDesc;

	//�V�O�l�`���A�G���[�̏�����
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	//���[�g�V�O�l�`���̐���
	result = _dev->CreateRootSignature(0, signature->GetBufferPointer(),
		signature->GetBufferSize(), IID_PPV_ARGS(&_firstSignature));

	if (FAILED(result))
	{
		return false;
	}
	return true;
}


bool Dx12Wrapper::SignatureInit()
{
	//�T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC sampleDesc[2] = {};
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
	sampleDesc[1] = sampleDesc[0];
	sampleDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampleDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampleDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampleDesc[1].ShaderRegister = 1;

	//�����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE range[4] = {};
	//�R���X�^���g�o�b�t�@�r���[
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//�}�e���A��
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[1].BaseShaderRegister = 1;
	range[1].NumDescriptors = 1;
	range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//�V�F�[�_�[���\�[�X�r���[
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[2].BaseShaderRegister = 0;
	range[2].NumDescriptors = 4;
	range[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//�{�[��
	range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[3].BaseShaderRegister = 2;
	range[3].NumDescriptors = 1;
	range[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//���[�g�p�����[�^�[�̐ݒ�
	D3D12_ROOT_PARAMETER rootParam[3] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].DescriptorTable.pDescriptorRanges = &range[0];
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2;
	rootParam[1].DescriptorTable.pDescriptorRanges = &range[1];
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	
	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[2].DescriptorTable.pDescriptorRanges = &range[3];
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	//���[�g�V�O�l�`���̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = 3;
	rsd.pParameters = rootParam;
	rsd.NumStaticSamplers = 2;
	rsd.pStaticSamplers = sampleDesc;

	//�V�O�l�`���A�G���[�̏�����
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	//���[�g�V�O�l�`���̐���
	result = _dev->CreateRootSignature(0, signature->GetBufferPointer(),
		signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));

	if (FAILED(result)) 
	{
		return false;
	}

	return true;
}

bool Dx12Wrapper::PipelineStateInit()
{
	//���C�A�E�g�쐬
	D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BONENO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = inputLayoutDesc;
	gpsDesc.InputLayout.NumElements = _countof(inputLayoutDesc);
	//�V�F�[�_�[�n
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_vsShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_psShader);
	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

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

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pipelineState));
	if (FAILED(result))
	{
		return false;
	}

	return true;
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
	resDesc.Format = DXGI_FORMAT_D32_FLOAT;
	resDesc.SampleDesc.Count = 1;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;

	//�[�x�o�b�t�@�[�쐬
	ID3D12Resource* depthBuffer = nullptr;

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue, IID_PPV_ARGS(&depthBuffer));

	//�f�X�N���v�^�q�[�v�ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	//�q�[�v�쐬
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_dsvDescHeap));
	//�[�x�X�e���V���r���[�ݒ�
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	auto h = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateDepthStencilView(depthBuffer, &dsvDesc, h);

	return true;
}

bool Dx12Wrapper::ConstantInit()
{
	auto wsize = Application::Instance().GetWindowSize();

	XMFLOAT3 eye(0, 20, -30);	//���_
	XMFLOAT3 target(0, 10, 0);  //�����_
	XMFLOAT3 up(0, 1, 0);		//��x�N�g��

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
		300
	);

	//���[���h�s��
	_wvp.world = XMMatrixIdentity();

	_wvp.eye = eye;

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
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_cbvDescHeap));

	auto h = _cbvDescHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateConstantBufferView(&cbvDesc, h);

	return true;
}

bool Dx12Wrapper::BoneInit()
{
	//�s��̏�����
	auto model = _pmd->GetModel();
	_boneMats.resize(model.boneCnt);
	std::fill(_boneMats.begin(), _boneMats.end(), XMMatrixIdentity());
	//�}�b�v���̍\�z
	for (int idx = 0; idx < model.boneCnt; ++idx)
	{
		auto& b = model.bones[idx];
		auto& boneNode = _boneMap[b.boneName];
		boneNode.boneIdx = idx;
		boneNode.startPos = b.boneHeadPos;
		boneNode.endPos = model.bones[b.tailBoneIndex].boneHeadPos;
	}
	//�e�ɒǉ�
	for (auto& b : _boneMap)
	{
		if (model.bones[b.second.boneIdx].parentBoneIndex >= model.boneCnt)continue;
		auto parentName = model.bones[model.bones[b.second.boneIdx].parentBoneIndex].boneName;
		_boneMap[parentName].children.push_back(&b.second);
	}

	size_t size = sizeof(XMMATRIX)* model.boneCnt;
	size = (size + 0xff) & ~0xff;

	ID3D12Resource* _boneBuffer = nullptr;

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_boneBuffer));


	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_boneHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = _boneBuffer->GetGPUVirtualAddress();
	desc.SizeInBytes = size;

	auto h = _boneHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateConstantBufferView(&desc, h);

	result = _boneBuffer->Map(0, nullptr, (void**)&_mapedBone);
	std::copy(_boneMats.begin(), _boneMats.end(), _mapedBone);

	return false;
}


Dx12Wrapper::~Dx12Wrapper()
{
}

bool Dx12Wrapper::Init()
{
	//�f�o�b�O���C���[�̋N��
	ID3D12Debug* debugLayer;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
	{
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}

	//�V�F�[�_�[�̃R���p�C��
	D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_vsShader, nullptr);
	D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_psShader, nullptr);
	
	DeviceInit();
	CommandInit();
	SwapChainInit();
	RTInit();
	
	_vmd.reset(new VMDLoader());
	_pmd.reset(new PMDModel(*_dev, "model/�����~�N.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/�����~�Nmetal.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/��������.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/��������.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/�������J.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/�特���C�R.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/�㉹�n�N.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/���k�l��.pmd"));
	//_pmx.reset(new PMXModel(*_dev,"model/�h�[��/�h�[��.pmx"));
	//_pmx.reset(new PMXModel(*_dev,"model/�V�X�^�[�N���A/�V�X�^�[�N���A.pmx"));

	_dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	DepthInit();
	ConstantInit();
	BoneInit();
	SignatureInit();
	PipelineStateInit();

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
	//auto pmx = _pmx->GetModel();
	auto pmd = _pmd->GetModel();

	_cmdAllocator->Reset();//�A���P�[�^���Z�b�g
	_cmdList->Reset(_cmdAllocator, nullptr);//�R�}���h���X�g���Z�b�g

	//�p�C�v���C���X�e�[�g�̐ݒ�
	_cmdList->SetPipelineState(_pipelineState);
	//���[�g�V�O�l�`���̐ݒ�
	_cmdList->SetGraphicsRootSignature(_rootSignature);

	//�萔
	_cmdList->SetDescriptorHeaps(1, &_cbvDescHeap);
	auto h = _cbvDescHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(0, h);

	//�r���[�|�[�g�̐ݒ�
	_cmdList->RSSetViewports(1, &_viewPort);
	//�V�U�[��`�̐ݒ�
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	auto bbIndex = _swapchain->GetCurrentBackBufferIndex();
	auto heapStart = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto heapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	heapStart.ptr = heapStart.ptr + bbIndex * heapSize;

	//�o���A�̐ݒ�
	D3D12_RESOURCE_BARRIER barrierDesc{};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.Transition.pResource = _renderTargets[bbIndex];
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	_cmdList->ResourceBarrier(1, &barrierDesc);

	auto dsvheap = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
	float clearColor[] = { 0.6f,0.3f,0.3f,1.0f };
	_cmdList->OMSetRenderTargets(1, &heapStart, false, &dsvheap);//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);//�N���A
	_cmdList->ClearDepthStencilView(dsvheap, D3D12_CLEAR_FLAG_DEPTH, 1.0f,0,0,nullptr);

	//���_�o�b�t�@�r���[�̐ݒ�
	_cmdList->IASetVertexBuffers(0, 1, &_pmd->GetVBV());

	//�C���f�b�N�X�o�b�t�@�r���[�̐ݒ�
	_cmdList->IASetIndexBuffer(&_pmd->GetIBV());

	//�g�|���W�̐ݒ�
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//�{�[��
	_cmdList->SetDescriptorHeaps(1, &_boneHeap);
	auto boneH = _boneHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(2, boneH);

	//���f��
	unsigned int offset = 0;
	auto material = _pmd->MaterialHeap();
	auto matH = material->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &material);

	for (auto& m : pmd.materials)
	{
		_cmdList->SetGraphicsRootDescriptorTable(1, matH);
		_cmdList->DrawIndexedInstanced(m.face_vert_cnt, 1, offset, 0, 0);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)*5;
		offset += m.face_vert_cnt;
	}

	/*for (auto& m : pmx.materials)
	{
		_cmdList->SetGraphicsRootDescriptorTable(1, matH);
		_cmdList->DrawIndexedInstanced(m.vertexNum, 1, offset, 0, 0);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		offset += m.vertexNum;
	}*/
	
	//���\�[�X�o���A
	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[bbIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	
	_cmdList->Close();//�R�}���h�̃N���[�Y

	ExecuteCommand();
	WaitFence();

	_swapchain->Present(0,0);
	auto duration = _vmd->GetDuration();
	UpdateMotion(++_frame / 14 % duration);
	std::copy(_boneMats.begin(), _boneMats.end(), _mapedBone);

	_wvp.world = XMMatrixRotationY(_angle);
	*_mapWvp = _wvp;
}

void Dx12Wrapper::ExecuteCommand()
{
	ID3D12CommandList* cmdLists[] = { _cmdList };
	_cmdQue->ExecuteCommandLists(1, cmdLists);
	_cmdQue->Signal(_fence, ++_fenceValue);
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

void Dx12Wrapper::RecursiveMatrixMultiply(BoneNode & node, const XMMATRIX & inMat)
{
	_boneMats[node.boneIdx] *= inMat;
	for (auto& cnode : node.children)
	{
		RecursiveMatrixMultiply(*cnode, _boneMats[node.boneIdx]);
	}
}

void Dx12Wrapper::RotateBone(std::string boneName, XMVECTOR rot)
{
	auto& bonenode = _boneMap[boneName];
	auto vec = XMLoadFloat3(&bonenode.startPos);
	_boneMats[bonenode.boneIdx] =
		XMMatrixTranslationFromVector(XMVectorScale(vec, -1))*
		XMMatrixRotationQuaternion(rot)*
		XMMatrixTranslationFromVector(vec);
}

void Dx12Wrapper::UpdateMotion(int frame)
{
	std::fill(_boneMats.begin(), _boneMats.end(), XMMatrixIdentity());

	for (auto& boneanim : _vmd->GetAnim())
	{
		if (_boneMap.find(boneanim.first) == _boneMap.end())continue;
		auto& keyframe = boneanim.second;
		auto rit = std::find_if(keyframe.rbegin(), keyframe.rend(), 
			[frame](const KeyFrame& k) {return k.frameNo <= frame; });
		if (rit == keyframe.rend())continue;
		auto it = rit.base();
		auto q = XMLoadFloat4(&rit->quaternion);
		auto pos = XMLoadFloat3(&rit->pos);
		XMMATRIX mat;
		if (it != keyframe.end())
		{
			auto q2= XMLoadFloat4(&it->quaternion);
			auto t = static_cast<float>(frame - rit->frameNo) /
				static_cast<float>(it->frameNo - rit->frameNo);
			t = _pmd->GetYFromXOnBezier(t, it->p1, it->p2, 12);
			q = XMQuaternionSlerp(q, q2, t);
			auto pos2= XMLoadFloat3(&it->pos);
			pos= XMVectorLerp(pos, pos2, t);
		}

		auto& bonenode = _boneMap[boneanim.first];
		auto vec = XMLoadFloat3(&bonenode.startPos);

		mat =XMMatrixTranslationFromVector(XMVectorScale(vec, -1))*
			 XMMatrixRotationQuaternion(q)*
			 XMMatrixTranslationFromVector(vec);

		mat = mat * XMMatrixTranslationFromVector(pos);

		_boneMats[bonenode.boneIdx] = mat;
	}

	auto rootmat = XMMatrixIdentity();
	RecursiveMatrixMultiply(_boneMap["�Z���^�["], rootmat);
}
