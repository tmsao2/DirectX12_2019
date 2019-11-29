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

Dx12Wrapper::Dx12Wrapper(HWND hwnd):_hwnd(hwnd)
{
}

bool Dx12Wrapper::DeviceInit()
{
	//デバイスの初期化
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
	//コマンドキューの作成
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	auto result = _dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(_cmdQue.GetAddressOf()));
	
	//コマンドアロケーターの作成
	_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.GetAddressOf()));
	//コマンドリストの作成
	_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.GetAddressOf()));
	_cmdList->Close();

	return true;
}

bool Dx12Wrapper::SwapChainInit()
{
	//DXGIの初期化
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgi.GetAddressOf()));

	auto wsize = Application::Instance().GetWindowSize();
	//スワップチェーンの設定
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

	//スワップチェーンの初期化
	result = _dxgi->CreateSwapChainForHwnd(_cmdQue.Get(), _hwnd, &swDesc,
		nullptr, nullptr, (IDXGISwapChain1**)(_swapchain.GetAddressOf()));
	
	return true;
}

bool Dx12Wrapper::RTInit()
{
	//デスクリプタヒープの設定(レンダーターゲット用)
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NumDescriptors = 2;
	heapDesc.NodeMask = 0;
	//デスクリプタヒープの初期化
	auto result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeap.GetAddressOf()));

	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();

	DXGI_SWAP_CHAIN_DESC swDesc = {};
	_swapchain->GetDesc(&swDesc);
	int renderTargetNum = swDesc.BufferCount;

	_renderTargets.resize(renderTargetNum);

	auto heapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//レンダーターゲットビューの初期化
	for (int i = 0; i < renderTargetNum; ++i)
	{
		result = _swapchain->GetBuffer(i, IID_PPV_ARGS(_renderTargets[i].GetAddressOf()));
		_dev->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, descriptorHandle);
		descriptorHandle.ptr += heapSize;
	}
	return true;
}

bool Dx12Wrapper::Create1ResourceAndView()
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

	D3D12_CLEAR_VALUE clear = 
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
		{ 0.5f ,0.5f ,0.5f ,1.0f }
	};

	result=_dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear, IID_PPV_ARGS(_resource.GetAddressOf()));

	heapDesc.NumDescriptors = 1;

	result=_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_peraRtvHeap.GetAddressOf()));
	_dev->CreateRenderTargetView(_resource.Get(), nullptr, _peraRtvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format=desc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	result=_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_peraSrvHeap.GetAddressOf()));
	_dev->CreateShaderResourceView(_resource.Get(), &srvDesc, _peraSrvHeap->GetCPUDescriptorHandleForHeapStart());
	return true;
}

bool Dx12Wrapper::Create2ResourceAndView()
{

	return false;
}

bool Dx12Wrapper::CreatePeraVertex()
{
	Vertex vertex[] = { {{-1.0f, 1.0f,0.1f},{0.0f,0.0f}},
						{{ 1.0f, 1.0f,0.1f},{1.0f,0.0f}},
						{{-1.0f,-1.0f,0.1f},{0.0f,1.0f}},
						{{ 1.0f,-1.0f,0.1f},{1.0f,1.0f}}};
	
	//頂点バッファ作成
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertex)), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(_vertexBuffer.GetAddressOf()));

	Vertex* vertMap = nullptr;
	result = _vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertex), std::end(vertex), vertMap);
	_vertexBuffer->Unmap(0, nullptr);

	//頂点バッファビューの作成
	_vb.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vb.StrideInBytes = sizeof(Vertex);
	_vb.SizeInBytes = sizeof(vertex);
	return false;
}

bool Dx12Wrapper::CreatePeraPipeline()
{
	//レイアウト作成
	D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = _peraSignature.Get();
	gpsDesc.InputLayout.pInputElementDescs = inputLayoutDesc;
	gpsDesc.InputLayout.NumElements = _countof(inputLayoutDesc);
	//シェーダー系
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_peraVsShader.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_peraPsShader.Get());
	//レンダーターゲット
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;

	//ラスタライザ
	//ラスタライザの設定
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

	//その他
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;

	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_peraPipeline.GetAddressOf()));
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool Dx12Wrapper::CreatePeraSignature()
{
	//サンプラの設定
	D3D12_STATIC_SAMPLER_DESC sampleDesc[1] = {};
	sampleDesc[0].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;//補間しない
	sampleDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横繰り返し
	sampleDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦繰り返し
	sampleDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行繰り返し
	sampleDesc[0].MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
	sampleDesc[0].MinLOD = 0.0f;//ミップマップ最小値
	sampleDesc[0].MipLODBias = 0.0f;
	sampleDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampleDesc[0].ShaderRegister = 0;
	sampleDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	sampleDesc[0].RegisterSpace = 0;
	sampleDesc[0].MaxAnisotropy = 0;
	sampleDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	//レンジの設定
	D3D12_DESCRIPTOR_RANGE range[3] = {};
	//シェーダーリソースビュー
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//深度
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[1].BaseShaderRegister = 1;
	range[1].NumDescriptors = 1;
	range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//ライトビュー
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[2].BaseShaderRegister = 2;
	range[2].NumDescriptors = 1;
	range[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	
	//ルートパラメーターの設定
	D3D12_ROOT_PARAMETER rootParam[3] = {};
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

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	//ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = 3;
	rsd.pParameters = rootParam;
	rsd.NumStaticSamplers = 1;
	rsd.pStaticSamplers = sampleDesc;

	//シグネチャ、エラーの初期化
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	//ルートシグネチャの生成
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

	//DSVヒープ作成
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

	//SRVヒープ作成
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

bool Dx12Wrapper::CreateEffect()
{
	DXGI_FORMAT bbFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	_efkRenderer = EffekseerRendererDX12::Create
	(_dev.Get(),	//デバイス
		_cmdQue.Get(),	//コマンドキュー
		2,				//スワップチェーン数(バックバッファの数)
		&bbFormat,		//バッファのフォーマット
		1,				//レンダーターゲット数
		false,			//デプス有効フラグ
		false,			//反転デプス有効フラグ
		2000);			//パーティクル数

	_efkManager = Effekseer::Manager::Create(2000);
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

bool Dx12Wrapper::DepthInit()
{
	auto wsize = Application::Instance().GetWindowSize();

	//リソース設定
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

	//デスクリプタヒープ設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	//ヒープ作成
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_dsvHeap.GetAddressOf()));
	//深度ステンシルビュー設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	auto h = _dsvHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateDepthStencilView(_depthBuffer.Get(), &dsvDesc, h);
	return true;
}

bool Dx12Wrapper::ConstantInit()
{
	auto wsize = Application::Instance().GetWindowSize();

	XMFLOAT3 eye(0, 20, -30);	//視点
	XMFLOAT3 up(0, 1, 0);		//上ベクトル
	target = XMFLOAT3(0, 10, 0);  //注視点

	//左手系、ビュー行列
	_wvp.view = XMMatrixLookAtLH(
		XMLoadFloat3(&eye),
		XMLoadFloat3(&target),
		XMLoadFloat3(&up));

	//プロジェクション行列
	//視野角、アスペクト比、ニア、ファー
	_wvp.projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(wsize.w) / static_cast<float>(wsize.h),
		0.1f,
		300
	);

	XMFLOAT3 light(-50, 50, -50);
	auto lightview=XMMatrixLookAtLH(XMLoadFloat3(&light), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto lightproj= XMMatrixOrthographicLH(40, 40, 0.1f, 1000);
	_wvp.lvp = lightview * lightproj;

	//ワールド行列
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

	//コンスタントバッファビューの設定
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _cbBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;

	//デスクリプタヒープの設定(テクスチャ用)
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NumDescriptors = 1;
	heapDesc.NodeMask = 0;

	//デスクリプタヒープの初期化
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_wvpHeap));

	auto h = _wvpHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateConstantBufferView(&cbvDesc, h);

	return true;
}

Dx12Wrapper::~Dx12Wrapper()
{
}

bool Dx12Wrapper::Init()
{
	HRESULT result = S_OK;

	//デバッグレイヤーの起動
	ID3D12Debug* debugLayer;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
	{
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}

	//シェーダーのコンパイル
	result = D3DCompileFromFile(L"Pera.hlsl", nullptr, nullptr, "vs", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_peraVsShader, nullptr);
	result = D3DCompileFromFile(L"Pera.hlsl", nullptr, nullptr, "ps", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_peraPsShader, nullptr);
	
	DeviceInit();
	CommandInit();
	SwapChainInit();
	RTInit();
	Create1ResourceAndView();
	CreatePeraVertex();
	CreatePeraSignature();
	CreatePeraPipeline();
	DepthInit();
	CreateDepthTex();
	CreateShadow();
	ConstantInit();
	CreateEffect();

	_dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.GetAddressOf()));

	//_pmd1.reset(new PMDModel(_dev.Get(), "model/初音ミク.pmd"));
	//_pmd1.reset(new PMDModel(_dev.Get(), "model/初音ミクmetal.pmd"));
	//_pmd2.reset(new PMDModel(_dev.Get(), "model/鏡音リン.pmd"));
	//_pmd.reset(new PMDModel(_dev.Get(), "model/鏡音レン.pmd"));
	//_pmd.reset(new PMDModel(_dev.Get(), "model/巡音ルカ.pmd"));
	//_pmd.reset(new PMDModel(_dev.Get(), "model/咲音メイコ.pmd"));
	//_pmd.reset(new PMDModel(_dev.Get(), "model/弱音ハク.pmd"));
	//_pmd.reset(new PMDModel(_dev.Get(), "model/亞北ネル.pmd"));
	_pmx.reset(new PMXModel(_dev.Get(),"model/ドーラ/ドーラ.pmx"));
	//_pmx.reset(new PMXModel(_dev.Get(),"model/シスタークレア/シスタークレア.pmx"));
	_plane.reset(new Plane(_dev.Get()));

	_input.reset(new Input());

	auto wsize = Application::Instance().GetWindowSize();
	//ビューポート設定
	_viewPort.TopLeftX = 0;
	_viewPort.TopLeftY = 0;
	_viewPort.Width = wsize.w;
	_viewPort.Height = wsize.h;
	_viewPort.MaxDepth = 1.0f;
	_viewPort.MinDepth = 0.0f;
	//シザー矩形設定
	_scissorRect.left = 0;
	_scissorRect.top = 0;
	_scissorRect.right = wsize.w;
	_scissorRect.bottom = wsize.h;

	return true;
}

void Dx12Wrapper::Update()
{
	_input->Update();
	//_pmd1->Update();
	//_pmd2->Update();
	CameraMove();
	*_mapWvp = _wvp;

	if (_input->GetKey()[VK_SPACE] & 0x80)
	{
		if (_efkManager->Exists(_efkHandle))
		{
			_efkManager->StopEffect(_efkHandle);
		}
		_efkHandle = _efkManager->Play(_effect.Get(), Effekseer::Vector3D(0, 10, 0));
		_efkManager->SetScale(_efkHandle, 3, 3, 3);
	}
	
	_cmdAllocator->Reset();//アロケータリセット
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);//コマンドリストリセット
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };

	//バリアの設定
	D3D12_RESOURCE_BARRIER barrierDesc{};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.Transition.pResource = _shadowBuffer.Get();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	_cmdList->ResourceBarrier(1, &barrierDesc);
	
	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(0, nullptr, false, &_shadowDsvHeap->GetCPUDescriptorHandleForHeapStart());
	//デプスステンシルビューの初期化
	_cmdList->ClearDepthStencilView(_shadowDsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	//影の描画
	//_pmd1->ShadowDraw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect, _wvpHeap.Get());
	_pmx->ShadowDraw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect, _wvpHeap.Get());

	//リソースバリア(シェーダーリソースとして使う)
	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_shadowBuffer.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	//バリアの設定
	barrierDesc.Transition.pResource = _resource.Get();
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	_cmdList->ResourceBarrier(1, &barrierDesc);

	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(1, &_peraRtvHeap->GetCPUDescriptorHandleForHeapStart(), false, &dsvH);
	//クリア
	_cmdList->ClearRenderTargetView(_peraRtvHeap->GetCPUDescriptorHandleForHeapStart(), clearColor, 0, nullptr);
	//デプスステンシルビューの初期化
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	
	//モデル描画
	//_pmd1->Draw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect,_wvpHeap.Get(),_shadowSrvHeap.Get());
	//_pmd2->Draw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect, _wvpHeap.Get(),_shadowSrvHeap.Get());
	_pmx->Draw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect, _wvpHeap.Get(), _shadowSrvHeap.Get());
	//エフェクト
	_efkManager->Update();
	_efkMemoryPool->NewFrame();
	EffekseerRendererDX12::BeginCommandList(_efkCommandList.Get(), _cmdList.Get());
	_efkRenderer->BeginRendering();
	_efkManager->Draw();
	_efkRenderer->EndRendering();
	EffekseerRendererDX12::EndCommandList(_efkCommandList.Get());
	//床
	_plane->Draw(_dev.Get(), _cmdList.Get(), _viewPort, _scissorRect, _wvpHeap.Get(), _shadowSrvHeap.Get());


	//リソースバリア(シェーダーリソースとして使う)
	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_resource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	
	//パイプラインステートの設定
	_cmdList->SetPipelineState(_peraPipeline.Get());
	//ルートシグネチャの設定
	_cmdList->SetGraphicsRootSignature(_peraSignature.Get());

	//ビューポートの設定
	_cmdList->RSSetViewports(1, &_viewPort);
	//シザー矩形の設定
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	auto bbIndex = _swapchain->GetCurrentBackBufferIndex();
	auto heapStart = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
	auto heapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	heapStart.ptr = heapStart.ptr + bbIndex * heapSize;

	barrierDesc.Transition.pResource = _renderTargets[bbIndex].Get();
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	_cmdList->ResourceBarrier(1, &barrierDesc);

	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);
	float clearColor2[] = { 0.5f,0.5f,0.5f,1.0f };
	//クリア
	_cmdList->ClearRenderTargetView(heapStart, clearColor2, 0, nullptr);

	_cmdList->SetDescriptorHeaps(1, _depthSrvHeap.GetAddressOf());
	auto depthSrvH = _depthSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(1, depthSrvH);

	_cmdList->SetDescriptorHeaps(1, _shadowSrvHeap.GetAddressOf());
	auto shadowSrvH = _shadowSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(2, shadowSrvH);

	//頂点バッファビューの設定
	_cmdList->IASetVertexBuffers(0, 1, &_vb);
	//トポロジの設定
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	_cmdList->SetDescriptorHeaps(1, _peraSrvHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(0, _peraSrvHeap->GetGPUDescriptorHandleForHeapStart());

	
	_cmdList->DrawInstanced(4, 1, 0, 0);

	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[bbIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	_cmdList->Close();//コマンドのクローズ

	ExecuteCommand();
	WaitFence();

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
	XMFLOAT3 up(0, 1, 0);		//上ベクトル
	float speed = 0.05f;

	if (_input->GetKey()['W'] & 0x80)
	{
		_wvp.eye = XMFLOAT3(_wvp.eye.x, _wvp.eye.y += speed, _wvp.eye.z);
		target = XMFLOAT3(target.x, target.y += speed, target.z);
	}
	if (_input->GetKey()['A'] & 0x80)
	{
		_wvp.eye = XMFLOAT3(_wvp.eye.x -= speed, _wvp.eye.y, _wvp.eye.z);
		target = XMFLOAT3(target.x -= speed, target.y, target.z);
	}
	if (_input->GetKey()['S'] & 0x80)
	{
		_wvp.eye = XMFLOAT3(_wvp.eye.x, _wvp.eye.y -= speed, _wvp.eye.z);
		target = XMFLOAT3(target.x, target.y -= speed, target.z);
	}
	if (_input->GetKey()['D'] & 0x80)
	{
		_wvp.eye = XMFLOAT3(_wvp.eye.x += speed, _wvp.eye.y, _wvp.eye.z);
		target = XMFLOAT3(target.x += speed, target.y, target.z);
	}
	if (_input->GetKey()[VK_UP] & 0x80)
	{
		_wvp.eye = XMFLOAT3(_wvp.eye.x, _wvp.eye.y, _wvp.eye.z += speed);
		target = XMFLOAT3(target.x, target.y, target.z += speed);
	}
	if (_input->GetKey()[VK_DOWN] & 0x80)
	{
		_wvp.eye = XMFLOAT3(_wvp.eye.x, _wvp.eye.y, _wvp.eye.z -= speed);
		target = XMFLOAT3(target.x, target.y, target.z -= speed);
	}
	if (_input->GetKey()[VK_LEFT] & 0x80)
	{
		_wvp.world *= XMMatrixRotationY(-0.01f);
	}
	if (_input->GetKey()[VK_RIGHT] & 0x80)
	{
		_wvp.world *= XMMatrixRotationY(0.01f);
	}
	_wvp.view = XMMatrixLookAtLH(
		XMLoadFloat3(&_wvp.eye),
		XMLoadFloat3(&target),
		XMLoadFloat3(&up));
}

