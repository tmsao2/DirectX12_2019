#include "Model.h"
#include <iostream>
#include <d3dx12.h>
#include <algorithm>
#include "VMDLoader.h"
#include "TextureResource.h"

Model::Model(Microsoft::WRL::ComPtr<ID3D12Device> dev, const char* path)
{
	_vmd.reset(new VMDLoader());
	_tex.reset(new TextureResource(dev));
}

Model::~Model()
{
}

bool Model::CreatePipeLine(Microsoft::WRL::ComPtr<ID3D12Device> dev, std::vector<D3D12_INPUT_ELEMENT_DESC> layout)
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

	D3D12_RENDER_TARGET_BLEND_DESC renderBlendDesc = {};
	renderBlendDesc.BlendEnable = true;
	renderBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	renderBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	renderBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	renderBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	renderBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	renderBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	renderBlendDesc.LogicOp = D3D12_LOGIC_OP_CLEAR;
	renderBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//その他
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	for (auto i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		blendDesc.RenderTarget[i] = renderBlendDesc;
	}
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.BlendState = blendDesc;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_pipelineState.GetAddressOf()));
	if (FAILED(result))
	{
		return false;
	}
	gpsDesc.pRootSignature = _shadowSignature.Get();
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_shadowVS.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_shadowPS.Get());

	result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_shadowPipeline.GetAddressOf()));

	return true;
}

bool Model::CreateRootSignature(Microsoft::WRL::ComPtr<ID3D12Device> dev)
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
	D3D12_DESCRIPTOR_RANGE range[5] = {};
	//カメラ
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//マテリアル
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[1].BaseShaderRegister = 1;
	range[1].NumDescriptors = 1;
	range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//テクスチャ
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[2].BaseShaderRegister = 0;
	range[2].NumDescriptors = 4;
	range[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//ボーン
	range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[3].BaseShaderRegister = 2;
	range[3].NumDescriptors = 1;
	range[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//影
	range[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[4].BaseShaderRegister = 4;
	range[4].NumDescriptors = 1;
	range[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//ルートパラメーターの設定
	D3D12_ROOT_PARAMETER rootParam[4] = {};
	//カメラ
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].DescriptorTable.pDescriptorRanges = &range[0];
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	//マテリアル・テクスチャ
	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2;
	rootParam[1].DescriptorTable.pDescriptorRanges = &range[1];
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//ボーン
	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[2].DescriptorTable.pDescriptorRanges = &range[3];
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	//影
	rootParam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[3].DescriptorTable.pDescriptorRanges = &range[4];
	rootParam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

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

	//ルートパラメーターの設定
	D3D12_ROOT_PARAMETER shadowParam[3] = {};
	shadowParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	shadowParam[0].DescriptorTable.NumDescriptorRanges = 1;
	shadowParam[0].DescriptorTable.pDescriptorRanges = &range[0];
	shadowParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	shadowParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	shadowParam[1].DescriptorTable.NumDescriptorRanges = 1;
	shadowParam[1].DescriptorTable.pDescriptorRanges = &range[3];
	shadowParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	
	//ルートシグネチャの設定
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = _countof(shadowParam);
	rsd.pParameters = shadowParam;
	rsd.NumStaticSamplers = 0;
	rsd.pStaticSamplers = nullptr;

	//ルートシグネチャの生成
	result = dev->CreateRootSignature(0, signature->GetBufferPointer(),
		signature->GetBufferSize(), IID_PPV_ARGS(_shadowSignature.GetAddressOf()));

	return true;
}

float Model::GetYFromXOnBezier(float x, DirectX::XMFLOAT2 p1, DirectX::XMFLOAT2 p2, int cnt)
{
	if (p1.x == p1.y&&p2.x == p2.y) {
		return x;
	}
	float t = x;
	float k0 = 1 + 3 * p1.x - 3 * p2.x;
	float k1 = 3 * p2.x - 6 * p1.x;
	float k2 = 3 * p1.x;
	const float epsilon = 0.0005f;
	for (int i = 0; i < cnt; ++i) {

		auto ft = k0 * t*t*t + k1 * t*t + k2 * t - x;
		if (ft <= epsilon && ft >= -epsilon)break;
		t -= ft / 2;
	}
	auto r = 1 - t;

	return t * t*t + 3 * t*t*r*p2.y + 3 * t*r*r*p1.y;
}

void Model::RecursiveMatrixMultiply(BoneNode & node, const DirectX::XMMATRIX & inMat)
{
	_boneMats[node.boneIdx] *= inMat;
	for (auto& cnode : node.children)
	{
		RecursiveMatrixMultiply(*cnode, _boneMats[node.boneIdx]);
	}
}

void Model::RotateBone(std::string boneName, DirectX::XMVECTOR rot)
{
	auto& bonenode = _boneMap[boneName];
	auto vec = DirectX::XMLoadFloat3(&bonenode.startPos);
	_boneMats[bonenode.boneIdx] =
		DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(vec, -1))*
		DirectX::XMMatrixRotationQuaternion(rot)*
		DirectX::XMMatrixTranslationFromVector(vec);
}

void Model::UpdateMotion(int frame)
{
	std::fill(_boneMats.begin(), _boneMats.end(), DirectX::XMMatrixIdentity());

	for (auto& boneanim : _vmd->GetAnim())
	{
		if (_boneMap.find(boneanim.first) == _boneMap.end())continue;
		auto& keyframe = boneanim.second;
		auto rit = std::find_if(keyframe.rbegin(), keyframe.rend(),
			[frame](const KeyFrame& k) {return k.frameNo <= frame; });
		if (rit == keyframe.rend())continue;
		auto it = rit.base();
		auto q = DirectX::XMLoadFloat4(&rit->quaternion);
		auto pos = DirectX::XMLoadFloat3(&rit->pos);
		DirectX::XMMATRIX mat;
		if (it != keyframe.end())
		{
			auto q2 = DirectX::XMLoadFloat4(&it->quaternion);
			auto t = static_cast<float>(frame - rit->frameNo) /
				static_cast<float>(it->frameNo - rit->frameNo);
			t = GetYFromXOnBezier(t, it->p1, it->p2, 12);
			q = DirectX::XMQuaternionSlerp(q, q2, t);
			auto pos2 = DirectX::XMLoadFloat3(&it->pos);
			pos = DirectX::XMVectorLerp(pos, pos2, t);
		}
		

		auto& bonenode = _boneMap[boneanim.first];
		auto vec = DirectX::XMLoadFloat3(&bonenode.startPos);

		mat = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(vec, -1))*
			DirectX::XMMatrixRotationQuaternion(q)*
			DirectX::XMMatrixTranslationFromVector(vec);

		mat = mat * DirectX::XMMatrixTranslationFromVector(pos);

		_boneMats[bonenode.boneIdx] = mat;
	}

	auto rootmat = DirectX::XMMatrixIdentity();
	RecursiveMatrixMultiply(_boneMap["センター"], rootmat);

	std::copy(_boneMats.begin(), _boneMats.end(), _mapedBone);
}

