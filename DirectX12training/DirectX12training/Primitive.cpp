#include "Primitive.h"
#include "d3dx12.h"

Primitive::Primitive(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
}

Primitive::~Primitive()
{
}

bool Primitive::CreatePipeLine(Microsoft::WRL::ComPtr<ID3D12Device> dev, std::vector<D3D12_INPUT_ELEMENT_DESC> layout)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = _rootSignature.Get();
	gpsDesc.InputLayout.pInputElementDescs = layout.data();
	gpsDesc.InputLayout.NumElements = layout.size();
	//シェーダー系
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_vsShader.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_psShader.Get());
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

	auto result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_pipelineState.GetAddressOf()));
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool Primitive::CreateRootSignature(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	//サンプラの設定
	D3D12_STATIC_SAMPLER_DESC sampleDesc[1] = {};
	sampleDesc[0].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;//補間しない
	sampleDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//横繰り返ししない
	sampleDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//縦繰り返ししない
	sampleDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//奥行繰り返ししない
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
	D3D12_DESCRIPTOR_RANGE range[2] = {};
	//カメラ
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[1].BaseShaderRegister = 0;
	range[1].NumDescriptors = 1;
	range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//ルートパラメーターの設定
	D3D12_ROOT_PARAMETER rootParam[2] = {};
	//カメラ
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].DescriptorTable.pDescriptorRanges = &range[0];
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[1].DescriptorTable.pDescriptorRanges = &range[1];
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	//ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = _countof(rootParam);
	rsd.pParameters = rootParam;
	rsd.NumStaticSamplers = _countof(sampleDesc);
	rsd.pStaticSamplers = sampleDesc;

	//シグネチャ、エラーの初期化
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	//ルートシグネチャの生成
	result = dev->CreateRootSignature(0, signature->GetBufferPointer(),
		signature->GetBufferSize(), IID_PPV_ARGS(_rootSignature.GetAddressOf()));

	if (FAILED(result))
	{
		return false;
	}
	return false;
}

