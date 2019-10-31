#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")
#include "Dx12Wrapper.h"
#include "Application.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include "Model.h"
#include "PMDModel.h"
#include "PMXModel.h"
#include "VMDLoader.h"

Dx12Wrapper::Dx12Wrapper(HWND hwnd):_hwnd(hwnd)
{
}


std::string Dx12Wrapper::GetTexPathFromModelAndTexPath(const std::string & modelPath, const char * texPath)
{
	int pathIdx1 = modelPath.rfind("/");
	int pathIdx2 = modelPath.rfind("\\");
	auto pathIdx = max(pathIdx1, pathIdx2);
	auto folderPath = modelPath.substr(0, pathIdx);
	return folderPath +"/"+ texPath;
}

std::wstring Dx12Wrapper::GetWideStringFromString(const std::string str)
{
	auto num1 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
									str.c_str(), -1, nullptr, 0);
	std::wstring wstr;
	wstr.resize(num1);
	auto num2= MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
									str.c_str(), -1, &wstr[0], num1);
	return wstr;
}

std::pair<std::string, std::string> Dx12Wrapper::SplitFileName(const std::string & path, const char splitter)
{
	int idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	return ret;
}

std::string Dx12Wrapper::GetExtension(const std::string & path)
{
	int idx = path.rfind(".");
	return path.substr(idx + 1, path.length() - idx - 1);
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
}

bool Dx12Wrapper::CommandInit()
{
	//コマンドキューの作成
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	auto result = _dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&_cmdQue));
	
	//コマンドアロケーターの作成
	_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	//コマンドリストの作成
	_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	_cmdList->Close();

	return true;
}

bool Dx12Wrapper::SwapChainInit()
{
	//DXGIの初期化
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgi));

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
	result = _dxgi->CreateSwapChainForHwnd(_cmdQue, _hwnd, &swDesc,
		nullptr, nullptr, (IDXGISwapChain1**)(&_swapchain));
	
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
	auto result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_rtvDescHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();

	DXGI_SWAP_CHAIN_DESC swDesc = {};
	_swapchain->GetDesc(&swDesc);
	int renderTargetNum = swDesc.BufferCount;

	_renderTargets.resize(renderTargetNum);

	auto heapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//レンダーターゲットビューの初期化
	for (int i = 0; i < renderTargetNum; ++i)
	{
		result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i]));
		_dev->CreateRenderTargetView(_renderTargets[i], nullptr, descriptorHandle);
		descriptorHandle.ptr += heapSize;
	}
	return true;
}

bool Dx12Wrapper::SignatureInit()
{
	//サンプラの設定
	D3D12_STATIC_SAMPLER_DESC sampleDesc[2] = {};
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
	sampleDesc[1] = sampleDesc[0];
	sampleDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampleDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampleDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampleDesc[1].ShaderRegister = 1;

	//レンジの設定
	D3D12_DESCRIPTOR_RANGE range[4] = {};
	//コンスタントバッファビュー
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//マテリアル
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[1].BaseShaderRegister = 1;
	range[1].NumDescriptors = 1;
	range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//シェーダーリソースビュー
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[2].BaseShaderRegister = 0;
	range[2].NumDescriptors = 4;
	range[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//ボーン
	range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[3].BaseShaderRegister = 2;
	range[3].NumDescriptors = 1;
	range[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//ルートパラメーターの設定
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

	//ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = 3;
	rsd.pParameters = rootParam;
	rsd.NumStaticSamplers = 2;
	rsd.pStaticSamplers = sampleDesc;

	//シグネチャ、エラーの初期化
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	//ルートシグネチャの生成
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
	//レイアウト作成
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
	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = _rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = inputLayoutDesc;
	gpsDesc.InputLayout.NumElements = _countof(inputLayoutDesc);
	//シェーダー系
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_vsShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_psShader);
	//レンダーターゲット
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

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

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pipelineState));
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

ID3D12Resource * Dx12Wrapper::LoadTexture(std::string & texPath)
{
	auto result = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	//テクスチャ読み込み
	TexMetadata meta;
	ScratchImage img;
	auto it = _resTbl.find(texPath);
	if (it != _resTbl.end()) {
		return it->second;
	}
	auto wstr = GetWideStringFromString(texPath);
	auto ext = GetExtension(texPath);
	if (ext == "png" || ext == "bmp" || ext == "jpg" || ext == "sph" || ext == "spa")
	{
		result = LoadFromWICFile(wstr.c_str(), WIC_FLAGS_NONE, &meta, img);
	}
	else if (ext == "tga")
	{
		result = LoadFromTGAFile(wstr.c_str(), &meta, img);
	}
	else if (ext == "dds")
	{
		result = LoadFromDDSFile(wstr.c_str(), 0, &meta, img);
	}
	
	if (FAILED(result))
	{
		return nullptr;
	}

	//ヒープ設定
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	//リソース設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(meta.dimension);
	resDesc.Alignment = 0;
	resDesc.Width = meta.width;
	resDesc.Height = meta.height;
	resDesc.DepthOrArraySize = meta.arraySize;
	resDesc.MipLevels = meta.mipLevels;
	resDesc.Format = meta.format;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//テクスチャバッファ
	ID3D12Resource* buffer = nullptr;

	//テクスチャオブジェクト生成
	result = _dev->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&buffer));

	if (FAILED(result))
	{
		return nullptr;
	}

	result=buffer->WriteToSubresource(0,
		nullptr,
		img.GetPixels(),
		img.GetImages()->rowPitch,
		img.GetPixelsSize()
	);

	if (FAILED(result))
	{
		return nullptr;
	}
	_resTbl[texPath] = buffer;

	return buffer;
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
	resDesc.Format = DXGI_FORMAT_D32_FLOAT;
	resDesc.SampleDesc.Count = 1;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;

	//深度バッファー作成
	ID3D12Resource* depthBuffer = nullptr;

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue, IID_PPV_ARGS(&depthBuffer));

	//デスクリプタヒープ設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	//ヒープ作成
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_dsvDescHeap));
	//深度ステンシルビュー設定
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

	XMFLOAT3 eye(0, 20, -30);	//視点
	XMFLOAT3 target(0, 10, 0);  //注視点
	XMFLOAT3 up(0, 1, 0);		//上ベクトル

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
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_cbvDescHeap));

	auto h = _cbvDescHeap->GetCPUDescriptorHandleForHeapStart();

	//h.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	_dev->CreateConstantBufferView(&cbvDesc, h);

	return true;
}

ID3D12Resource* Dx12Wrapper::CreateWhiteTex()
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;// 
	texHeapProp.CreationNodeMask = 0;//
	texHeapProp.VisibleNodeMask = 0;//

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 4;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//
	ID3D12Resource* whiteBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteBuff)
	);
	if (FAILED(result)) {
		return nullptr;
	}
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);//全部255で埋める
	//データ転送
	result = whiteBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	return whiteBuff;
}

ID3D12Resource * Dx12Wrapper::CreateBlackTex()
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;// 
	texHeapProp.CreationNodeMask = 0;//
	texHeapProp.VisibleNodeMask = 0;//

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 4;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//
	ID3D12Resource* blackBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&blackBuff)
	);
	if (FAILED(result)) {
		return nullptr;
	}
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);//全部0で埋める
	//データ転送
	result = blackBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	return blackBuff;
}

ID3D12Resource * Dx12Wrapper::CreateGradationTex()
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;// 
	texHeapProp.CreationNodeMask = 0;//
	texHeapProp.VisibleNodeMask = 0;//

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 4;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//

	ID3D12Resource* gradBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&gradBuff)
	);
	if (FAILED(result)) {
		return nullptr;
	}
	//上が白くて下が黒いテクスチャデータを作成
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4) {
		auto col = (c << 0xff) | (c << 16) | (c << 8) | c;
		std::fill(it, it + 4, col);
		--c;
	}
	result = gradBuff->WriteToSubresource(0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int)*data.size());

	return gradBuff;
}

bool Dx12Wrapper::MaterialInit()
{
	HRESULT result;
	auto pmd = _pmd->GetModel();
	
	std::vector<ID3D12Resource*> texResources(pmd.matCnt);
	std::vector<ID3D12Resource*> sphResources(pmd.matCnt);
	std::vector<ID3D12Resource*> spaResources(pmd.matCnt);
	std::vector<ID3D12Resource*> toonResources(pmd.matCnt);

	ID3D12Resource* whiteBuffer = CreateWhiteTex();
	ID3D12Resource* blackBuffer = CreateBlackTex();
	ID3D12Resource* gradBuffer = CreateGradationTex();

	for (int i = 0; i < pmd.matCnt; ++i)
	{
		texResources[i] = whiteBuffer;
		sphResources[i] = whiteBuffer;
		spaResources[i] = blackBuffer;
		toonResources[i] = gradBuffer;
		std::string toonFilePath = "toon/";
		char toonFileName[16];
		if (pmd.materials[i].toon_index < 0xff)
		{
			sprintf_s(toonFileName, "toon%02d.bmp", pmd.materials[i].toon_index + 1);
			toonFilePath += toonFileName;
			toonResources[i] = LoadTexture(toonFilePath);
		}

		std::string texFileName = pmd.materials[i].texture_file_name;
		if (strlen(texFileName.c_str()) != 0)
		{
			int spCnt = std::count(texFileName.begin(), texFileName.end(), '*');
			if (spCnt > 0)
			{
				auto namePair = SplitFileName(texFileName);
				texFileName = (GetExtension(namePair.first) == "sph" || GetExtension(namePair.first) == "spa") ? namePair.second : namePair.first;
			}
			auto filePath = GetTexPathFromModelAndTexPath(pmd.path,texFileName.c_str());
			if (GetExtension(texFileName.c_str()) == "sph")
			{
				sphResources[i] = LoadTexture(filePath);
			}
			else if (GetExtension(texFileName.c_str()) == "spa")
			{
				spaResources[i] = LoadTexture(filePath);
			}
			else
			{
				texResources[i] = LoadTexture(filePath);
			}
		}
	}

	std::vector<ID3D12Resource*> materialBuffer;
	auto mats = pmd.materials;

	size_t size = sizeof(Material);
	size = (size + 0xff)&~0xff;

	int midx = 0;
	materialBuffer.resize(mats.size());

	for (auto& matBuff : materialBuffer)
	{
		result = _dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&matBuff)
		);

		mat.diffuse = mats[midx].diffuse;
		mat.power = mats[midx].power;
		mat.specular = mats[midx].specular;
		mat.ambient = mats[midx].ambient;

		Material* matMap = nullptr;
		result = matBuff->Map(0, nullptr, (void**)&matMap);
		*matMap = mat;
		++midx;
	}

	//デスクリプタヒープ設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = mats.size() * 5;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_materialHeap));

	//シェーダーリソースビューの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto h = _materialHeap->GetCPUDescriptorHandleForHeapStart();
	auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc = {};
	cbDesc.SizeInBytes = size;
	for (int i=0;i<mats.size();i++)
	{
		//マテリアル
		cbDesc.BufferLocation = materialBuffer[i]->GetGPUVirtualAddress();
		_dev->CreateConstantBufferView(&cbDesc, h);
		h.ptr += inc;
		//テクスチャ
		srvDesc.Format = texResources[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(texResources[i], &srvDesc, h);
		h.ptr += inc;
		//スフィア(乗算)
		srvDesc.Format = sphResources[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(sphResources[i], &srvDesc, h);
		h.ptr += inc;
		//スフィア(加算)
		srvDesc.Format = spaResources[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(spaResources[i], &srvDesc, h);
		h.ptr += inc;
		//トゥーン
		srvDesc.Format = toonResources[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(toonResources[i], &srvDesc, h);
		h.ptr += inc;
	}
	return false;
}

bool Dx12Wrapper::PMXMaterialInit()
{
	HRESULT result;
	auto pmx = _pmx->GetModel();

	std::vector<ID3D12Resource*> texResources(pmx.materials.size());
	std::vector<ID3D12Resource*> sphResources(pmx.materials.size());
	std::vector<ID3D12Resource*> spaResources(pmx.materials.size());
	std::vector<ID3D12Resource*> toonResources(pmx.materials.size());

	ID3D12Resource* whiteBuffer = CreateWhiteTex();
	ID3D12Resource* blackBuffer = CreateBlackTex();

	for (int i = 0; i < pmx.materials.size(); ++i)
	{
		texResources[i] = whiteBuffer;
		sphResources[i] = whiteBuffer;
		spaResources[i] = blackBuffer;
		toonResources[i] = whiteBuffer;
		std::string toonFilePath = "toon/";
		char toonFileName[16];
		if (pmx.materials[i].toonflag !=0)
		{
			sprintf_s(toonFileName, "toon%02d.bmp", pmx.materials[i].toonTex + 1);
			toonFilePath += toonFileName;
			toonResources[i] = LoadTexture(toonFilePath);
		}
		else if (pmx.materials[i].toonTex < 0xff)
		{
			toonFilePath = GetTexPathFromModelAndTexPath(pmx.path, pmx.handle[pmx.materials[i].toonTex].c_str());
			toonResources[i] = LoadTexture(toonFilePath);
		}
		std::string texFileName;

		if (pmx.materials[i].tex < 0xff)
		{
			texFileName = pmx.handle[pmx.materials[i].tex];
		}
		std::string filePath;
		if (strlen(texFileName.c_str()) != 0)
		{
			filePath = GetTexPathFromModelAndTexPath(pmx.path, texFileName.c_str());
			texResources[i] = LoadTexture(filePath);
		}
		if (pmx.materials[i].sp_tex < 0xff)
		{
			if (strlen(pmx.handle[pmx.materials[i].sp_tex].c_str()))
			{
				if (pmx.materials[i].sphere == 1)//乗算スフィア
				{
					filePath = GetTexPathFromModelAndTexPath(pmx.path, pmx.handle[pmx.materials[i].sp_tex].c_str());
					sphResources[i] = LoadTexture(filePath);
				}
				else if (pmx.materials[i].sphere == 2)//加算スフィア
				{
					filePath = GetTexPathFromModelAndTexPath(pmx.path, pmx.handle[pmx.materials[i].sp_tex].c_str());
					spaResources[i] = LoadTexture(filePath);
				}
				else//サブテクスチャ(追加UV1のx,yをUV参照して通常テクスチャ描画を行う)
				{

				}
			}
		}
	}

	std::vector<ID3D12Resource*> materialBuffer;
	auto mats = pmx.materials;

	size_t size = sizeof(Material);
	size = (size + 0xff)&~0xff;

	int midx = 0;
	materialBuffer.resize(mats.size());

	for (auto& matBuff : materialBuffer)
	{
		result = _dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&matBuff)
		);

		mat.diffuse = mats[midx].diffuse;
		mat.power = mats[midx].power;
		mat.specular = mats[midx].specular;
		mat.ambient = mats[midx].ambient;

		Material* matMap = nullptr;
		result = matBuff->Map(0, nullptr, (void**)&matMap);
		*matMap = mat;
		++midx;
	}
	//デスクリプタヒープ設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = mats.size() * 5;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_materialHeap));

	//シェーダーリソースビューの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto h = _materialHeap->GetCPUDescriptorHandleForHeapStart();
	auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc = {};
	cbDesc.SizeInBytes = size;
	for (int i = 0; i < mats.size(); i++)
	{
		//マテリアル
		cbDesc.BufferLocation = materialBuffer[i]->GetGPUVirtualAddress();
		_dev->CreateConstantBufferView(&cbDesc, h);
		h.ptr += inc;
		//テクスチャ
		srvDesc.Format = texResources[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(texResources[i], &srvDesc, h);
		h.ptr += inc;
		//スフィア(乗算)
		srvDesc.Format = sphResources[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(sphResources[i], &srvDesc, h);
		h.ptr += inc;
		//スフィア(加算)
		srvDesc.Format = spaResources[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(spaResources[i], &srvDesc, h);
		h.ptr += inc;
		//トゥーン
		srvDesc.Format = toonResources[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(toonResources[i], &srvDesc, h);
		h.ptr += inc;
	}
	return false;
}

bool Dx12Wrapper::BoneInit()
{
	//行列の初期化
	auto model = _pmd->GetModel();
	_boneMats.resize(model.boneCnt);
	std::fill(_boneMats.begin(), _boneMats.end(), XMMatrixIdentity());
	//マップ情報の構築
	for (int idx = 0; idx < model.boneCnt; ++idx)
	{
		auto& b = model.bones[idx];
		auto& boneNode = _boneMap[b.boneName];
		boneNode.boneIdx = idx;
		boneNode.startPos = b.boneHeadPos;
		boneNode.endPos = model.bones[b.tailBoneIndex].boneHeadPos;
	}
	//親に追加
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
	//デバッグレイヤーの起動
	ID3D12Debug* debugLayer;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
	{
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}

	//シェーダーのコンパイル
	D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_vsShader, nullptr);
	D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_psShader, nullptr);
	
	DeviceInit();
	CommandInit();
	SwapChainInit();
	RTInit();
	
	_vmd.reset(new VMDLoader());
	_pmd.reset(new PMDModel(*_dev, "model/初音ミク.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/初音ミクmetal.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/鏡音リン.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/鏡音レン.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/巡音ルカ.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/咲音メイコ.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/弱音ハク.pmd"));
	//_pmd.reset(new PMDModel(*_dev, "model/亞北ネル.pmd"));
	//_pmx.reset(new PMXModel(*_dev,"model/ドーラ/ドーラ.pmx"));
	//_pmx.reset(new PMXModel(*_dev,"model/シスタークレア/シスタークレア.pmx"));

	_dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	DepthInit();
	ConstantInit();
	MaterialInit();
	//PMXMaterialInit();
	BoneInit();
	SignatureInit();
	PipelineStateInit();

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
	//auto pmx = _pmx->GetModel();
	auto pmd = _pmd->GetModel();

	_cmdAllocator->Reset();//アロケータリセット
	_cmdList->Reset(_cmdAllocator, nullptr);//コマンドリストリセット

	//パイプラインステートの設定
	_cmdList->SetPipelineState(_pipelineState);
	//ルートシグネチャの設定
	_cmdList->SetGraphicsRootSignature(_rootSignature);

	//定数
	_cmdList->SetDescriptorHeaps(1, &_cbvDescHeap);
	auto h = _cbvDescHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(0, h);

	//ビューポートの設定
	_cmdList->RSSetViewports(1, &_viewPort);
	//シザー矩形の設定
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	auto bbIndex = _swapchain->GetCurrentBackBufferIndex();
	auto heapStart = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto heapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	heapStart.ptr = heapStart.ptr + bbIndex * heapSize;

	//バリアの設定
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
	_cmdList->OMSetRenderTargets(1, &heapStart, false, &dsvheap);//レンダーターゲット設定
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);//クリア
	_cmdList->ClearDepthStencilView(dsvheap, D3D12_CLEAR_FLAG_DEPTH, 1.0f,0,0,nullptr);

	//頂点バッファビューの設定
	_cmdList->IASetVertexBuffers(0, 1, &_pmd->GetVBV());

	//インデックスバッファビューの設定
	_cmdList->IASetIndexBuffer(&_pmd->GetIBV());

	//トポロジの設定
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//ボーン
	_cmdList->SetDescriptorHeaps(1, &_boneHeap);
	auto boneH = _boneHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(2, boneH);

	//モデル
	unsigned int offset = 0;
	auto matH = _materialHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &_materialHeap);

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
	
	//リソースバリア
	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[bbIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	
	_cmdList->Close();//コマンドのクローズ

	ExecuteCommand();
	WaitFence();

	_swapchain->Present(0,0);
	auto duration = _vmd->GetDuration();
	std::fill(_boneMats.begin(), _boneMats.end(), XMMatrixIdentity());
	UpdateMotion(++_frame / 8 % duration);
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

void Dx12Wrapper::RecursiveMatrixMultiply(BoneNode & node, XMMATRIX & inMat)
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
	for (auto& boneanim : _vmd->GetAnim())
	{
		auto& keyframe = boneanim.second;
		auto rit = std::find_if(keyframe.rbegin(), keyframe.rend(), 
			[frame](const KeyFrame& k) {return k.frameNo <= frame; });
		if (rit == keyframe.rend())continue;
		auto it = rit.base();
		auto q = XMLoadFloat4(&rit->quaternion);
		XMMATRIX mat;
		if (it != keyframe.end())
		{
			auto q2= XMLoadFloat4(&it->quaternion);
			auto t = static_cast<float>(frame - rit->frameNo) /
				static_cast<float>(it->frameNo - rit->frameNo);
			q = XMQuaternionSlerp(q, q2, t);
			RotateBone(boneanim.first, q);
		}
		else
		{
			RotateBone(boneanim.first, q);
		}
	}

	auto rootmat = XMMatrixIdentity();
	RecursiveMatrixMultiply(_boneMap["センター"], rootmat);
}
