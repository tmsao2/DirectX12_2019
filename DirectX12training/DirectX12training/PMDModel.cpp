#include "PMDModel.h"
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <iostream>
#include "TextureResource.h"
#include "VMDLoader.h"

PMDModel::PMDModel(Microsoft::WRL::ComPtr<ID3D12Device> dev, const char* path) :Model(dev, path)
{
	LoadModel(path);
	VertexBufferInit(dev);
	IndexBufferInit(dev);
	MaterialInit(dev);
	BoneInit(dev);
	CreateRootSignature(dev);
	InitPipeLine(dev);
}

PMDModel::~PMDModel()
{
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> PMDModel::MaterialHeap()const
{
	return _materialHeap;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> PMDModel::BoneHeap() const
{
	return _boneHeap;
}

void PMDModel::LoadModel(const char * path)
{
	FILE* fp;
	_model.path = path;
	fopen_s(&fp, _model.path.c_str(), "rb");
	//ヘッダー
	fread(&_model.header.magic, sizeof(_model.header.magic[0]), _countof(_model.header.magic), fp);
	fread(&_model.header.version, sizeof(_model.header.version), 1, fp);
	fread(&_model.header.model_name, sizeof(_model.header.model_name[0]), _countof(_model.header.model_name), fp);
	fread(&_model.header.comment, sizeof(_model.header.comment[0]), _countof(_model.header.comment), fp);

	//頂点
	fread(&_model.vertCnt, sizeof(_model.vertCnt), 1, fp);

	_model.vertices.resize(_model.vertCnt);
	for (auto& vertex : _model.vertices)
	{
		fread(&vertex.pos, sizeof(vertex.pos), 1, fp);
		fread(&vertex.normal, sizeof(vertex.normal), 1, fp);
		fread(&vertex.uv, sizeof(vertex.uv), 1, fp);
		fread(&vertex.bone_num, sizeof(vertex.bone_num), 1, fp);
		fread(&vertex.bone_weight, sizeof(vertex.bone_weight), 1, fp);
		fread(&vertex.edge_flag, sizeof(vertex.edge_flag), 1, fp);
	}
	//インデックス
	fread(&_model.idxCnt, sizeof(_model.idxCnt), 1, fp);
	_model.indices.resize(_model.idxCnt);
	for (int i = 0; i < _model.idxCnt; ++i)
	{
		fread(&_model.indices[i], sizeof(_model.indices[i]), 1, fp);
	}
	//マテリアル
	fread(&_model.matCnt, sizeof(_model.matCnt), 1, fp);
	_model.materials.resize(_model.matCnt);
	for (auto& mat : _model.materials)
	{
		fread(&mat.diffuse, sizeof(mat.diffuse), 1, fp);
		fread(&mat.power, sizeof(mat.power), 1, fp);
		fread(&mat.specular, sizeof(mat.specular), 1, fp);
		fread(&mat.ambient, sizeof(mat.ambient), 1, fp);
		fread(&mat.toon_index, sizeof(mat.toon_index), 1, fp);
		fread(&mat.edge_flag, sizeof(mat.edge_flag), 1, fp);
		fread(&mat.face_vert_cnt, sizeof(mat.face_vert_cnt), 1, fp);
		fread(&mat.texture_file_name, sizeof(mat.texture_file_name[0]), _countof(mat.texture_file_name), fp);
	}
	//ボーン
	fread(&_model.boneCnt, sizeof(_model.boneCnt), 1, fp);
	_model.bones.resize(_model.boneCnt);
	for (auto& bone : _model.bones)
	{
		fread(&bone.boneName, sizeof(bone.boneName[0]), _countof(bone.boneName), fp);
		fread(&bone.parentBoneIndex, sizeof(bone.parentBoneIndex), 1, fp);
		fread(&bone.tailBoneIndex, sizeof(bone.tailBoneIndex), 1, fp);
		fread(&bone.boneType, sizeof(bone.boneType), 1, fp);
		fread(&bone.ikParentBoneIndex, sizeof(bone.ikParentBoneIndex), 1, fp);
		fread(&bone.boneHeadPos, sizeof(bone.boneHeadPos), 1, fp);
	}

	fclose(fp);
}

bool PMDModel::VertexBufferInit(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	//ヒープ設定
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	//リソース設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeof(PMDVertex)*_model.vertCnt;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//頂点バッファ作成
	auto result = dev->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(_vertexBuffer.GetAddressOf()));

	PMDVertex* vertMap = nullptr;
	result = _vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, &_model.vertices[0], sizeof(PMDVertex)*_model.vertCnt);
	_vertexBuffer->Unmap(0, nullptr);

	//頂点バッファビューの作成
	_vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vbView.StrideInBytes = sizeof(PMDVertex);
	_vbView.SizeInBytes = sizeof(PMDVertex)*_model.vertCnt;

	return true;
}

bool PMDModel::IndexBufferInit(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(_model.indices.size() * sizeof(_model.indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(_indexBuffer.GetAddressOf()));

	//インデックスデータ転送
	unsigned short* idxMap = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxMap);

	std::copy(std::begin(_model.indices), std::end(_model.indices), idxMap);
	_indexBuffer->Unmap(0, nullptr);

	_ibView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_ibView.Format = DXGI_FORMAT_R16_UINT;
	_ibView.SizeInBytes = _model.indices.size() * sizeof(_model.indices[0]);

	return true;
}

bool PMDModel::MaterialInit(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	HRESULT result;

	texResources.resize(_model.matCnt);
	sphResources.resize(_model.matCnt);
	spaResources.resize(_model.matCnt);
	toonResources.resize(_model.matCnt);

	whiteBuffer = _tex->CreateWhiteTex();
	blackBuffer = _tex->CreateBlackTex();
	gradBuffer = _tex->CreateGradationTex();

	for (int i = 0; i < _model.matCnt; ++i)
	{
		texResources[i] = whiteBuffer;
		sphResources[i] = whiteBuffer;
		spaResources[i] = blackBuffer;
		toonResources[i] = gradBuffer;
		std::string toonFilePath = "toon/";
		char toonFileName[16];
		if (_model.materials[i].toon_index < 0xff)
		{
			sprintf_s(toonFileName, "toon%02d.bmp", _model.materials[i].toon_index + 1);
			toonFilePath += toonFileName;
			toonResources[i] = _tex->LoadTexture(toonFilePath);
		}

		std::string texFileName = _model.materials[i].texture_file_name;
		if (strlen(texFileName.c_str()) != 0)
		{
			int spCnt = std::count(texFileName.begin(), texFileName.end(), '*');
			if (spCnt > 0)
			{
				auto namePair = _tex->SplitFileName(texFileName, '*');
				texFileName = (_tex->GetExtension(namePair.first) == "sph" || _tex->GetExtension(namePair.first) == "spa") ? namePair.second : namePair.first;
			}
			auto filePath = _tex->GetTexPathFromModelAndTexPath(_model.path, texFileName.c_str());
			if (_tex->GetExtension(texFileName.c_str()) == "sph")
			{
				sphResources[i] = _tex->LoadTexture(filePath);
			}
			else if (_tex->GetExtension(texFileName.c_str()) == "spa")
			{
				spaResources[i] = _tex->LoadTexture(filePath);
			}
			else
			{
				texResources[i] = _tex->LoadTexture(filePath);
			}
		}
	}

	auto mats = _model.materials;

	size_t size = sizeof(PMDMaterial);
	size = (size + 0xff)&~0xff;

	int midx = 0;
	materialBuffer.resize(mats.size());

	for (auto& matBuff : materialBuffer)
	{
		result = dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(matBuff.GetAddressOf())
		);

		PMDMaterial* matMap = nullptr;
		result = matBuff->Map(0, nullptr, (void**)&matMap);
		*matMap = mats[midx];
		++midx;
	}

	//デスクリプタヒープ設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = mats.size() * 5;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_materialHeap.GetAddressOf()));

	//シェーダーリソースビューの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto h = _materialHeap->GetCPUDescriptorHandleForHeapStart();
	auto inc = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc = {};
	cbDesc.SizeInBytes = size;
	for (int i = 0; i < mats.size(); i++)
	{
		//マテリアル
		cbDesc.BufferLocation = materialBuffer[i]->GetGPUVirtualAddress();
		dev->CreateConstantBufferView(&cbDesc, h);
		h.ptr += inc;
		//テクスチャ
		srvDesc.Format = texResources[i]->GetDesc().Format;
		dev->CreateShaderResourceView(texResources[i].Get(), &srvDesc, h);
		h.ptr += inc;
		//スフィア(乗算)
		srvDesc.Format = sphResources[i]->GetDesc().Format;
		dev->CreateShaderResourceView(sphResources[i].Get(), &srvDesc, h);
		h.ptr += inc;
		//スフィア(加算)
		srvDesc.Format = spaResources[i]->GetDesc().Format;
		dev->CreateShaderResourceView(spaResources[i].Get(), &srvDesc, h);
		h.ptr += inc;
		//トゥーン
		srvDesc.Format = toonResources[i]->GetDesc().Format;
		dev->CreateShaderResourceView(toonResources[i].Get(), &srvDesc, h);
		h.ptr += inc;
	}
	return false;
}

bool PMDModel::BoneInit(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	//行列の初期化
	_boneMats.resize(_model.boneCnt);
	std::fill(_boneMats.begin(), _boneMats.end(), DirectX::XMMatrixIdentity());
	//マップ情報の構築
	for (int idx = 0; idx < _model.boneCnt; ++idx)
	{
		auto& b = _model.bones[idx];
		auto& boneNode = _boneMap[b.boneName];
		boneNode.boneIdx = idx;
		boneNode.startPos = b.boneHeadPos;
		boneNode.endPos = _model.bones[b.tailBoneIndex].boneHeadPos;
	}
	//親に追加
	for (auto& b : _boneMap)
	{
		if (_model.bones[b.second.boneIdx].parentBoneIndex >= _model.boneCnt)continue;
		auto parentName = _model.bones[_model.bones[b.second.boneIdx].parentBoneIndex].boneName;
		_boneMap[parentName].children.push_back(&b.second);
	}

	size_t size = sizeof(DirectX::XMMATRIX)* _model.boneCnt;
	size = (size + 0xff) & ~0xff;


	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_boneBuffer.GetAddressOf()));


	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(_boneHeap.GetAddressOf()));

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = _boneBuffer->GetGPUVirtualAddress();
	desc.SizeInBytes = size;

	auto h = _boneHeap->GetCPUDescriptorHandleForHeapStart();
	dev->CreateConstantBufferView(&desc, h);

	result = _boneBuffer->Map(0, nullptr, (void**)&_mapedBone);
	std::copy(_boneMats.begin(), _boneMats.end(), _mapedBone);

	return false;
}

bool PMDModel::InitPipeLine(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	HRESULT result;
	result = D3DCompileFromFile(L"PMDShader.hlsl", nullptr, nullptr, "PmdVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_vsShader, nullptr);
	result = D3DCompileFromFile(L"PMDShader.hlsl", nullptr, nullptr, "PmdPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_psShader, nullptr);
	result = D3DCompileFromFile(L"Shadow.hlsl", nullptr, nullptr, "ShadowVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_shadowVS, nullptr);
	result = D3DCompileFromFile(L"Shadow.hlsl", nullptr, nullptr, "ShadowPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_shadowPS, nullptr);
	//レイアウト作成
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDesc = {
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

	CreatePipeLine(dev, inputLayoutDesc);
	return false;
}

void PMDModel::Update()
{
	static auto lastTime = GetTickCount();
	if (GetTickCount() - lastTime > _vmd->GetDuration()*33.33333f) 
	{
		lastTime = GetTickCount(); 
	}
	UpdateMotion(static_cast<float>(GetTickCount() - lastTime) / 33.33333f);
}

void PMDModel::ShadowDraw(ID3D12Device * dev, ID3D12GraphicsCommandList * cmd, D3D12_VIEWPORT & view, D3D12_RECT & rect, ID3D12DescriptorHeap * wvp)
{
	//パイプラインステートの設定
	cmd->SetPipelineState(_shadowPipeline.Get());
	//ルートシグネチャの設定
	cmd->SetGraphicsRootSignature(_shadowSignature.Get());
	//カメラ
	cmd->SetDescriptorHeaps(1, &wvp);
	auto h = wvp->GetGPUDescriptorHandleForHeapStart();
	cmd->SetGraphicsRootDescriptorTable(0, h);
	//ビューポートの設定
	cmd->RSSetViewports(1, &view);
	//シザー矩形の設定
	cmd->RSSetScissorRects(1, &rect);
	//頂点バッファビューの設定
	cmd->IASetVertexBuffers(0, 1, &_vbView);
	//インデックスバッファビューの設定
	cmd->IASetIndexBuffer(&_ibView);
	//トポロジの設定
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//ボーン
	auto boneH = _boneHeap->GetGPUDescriptorHandleForHeapStart();
	cmd->SetDescriptorHeaps(1, _boneHeap.GetAddressOf());
	cmd->SetGraphicsRootDescriptorTable(2, boneH);
	
	cmd->DrawIndexedInstanced(_model.indices.size(), 1, 0, 0, 0);
	
}

void PMDModel::Draw(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, D3D12_VIEWPORT& view, D3D12_RECT& rect, ID3D12DescriptorHeap* wvp, ID3D12DescriptorHeap* shadow)
{
	//パイプラインステートの設定
	cmd->SetPipelineState(_pipelineState.Get());
	//ルートシグネチャの設定
	cmd->SetGraphicsRootSignature(_rootSignature.Get());
	//カメラ
	cmd->SetDescriptorHeaps(1, &wvp);
	auto h = wvp->GetGPUDescriptorHandleForHeapStart();
	cmd->SetGraphicsRootDescriptorTable(0, h);
	//ビューポートの設定
	cmd->RSSetViewports(1, &view);
	//シザー矩形の設定
	cmd->RSSetScissorRects(1, &rect);
	//頂点バッファビューの設定
	cmd->IASetVertexBuffers(0, 1, &_vbView);
	//インデックスバッファビューの設定
	cmd->IASetIndexBuffer(&_ibView);
	//トポロジの設定
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//ボーン
	auto boneH = _boneHeap->GetGPUDescriptorHandleForHeapStart();
	cmd->SetDescriptorHeaps(1, _boneHeap.GetAddressOf());
	cmd->SetGraphicsRootDescriptorTable(2, boneH);
	//影
	cmd->SetDescriptorHeaps(1, &shadow);
	cmd->SetGraphicsRootDescriptorTable(3, shadow->GetGPUDescriptorHandleForHeapStart());

	//マテリアル
	auto matH = _materialHeap->GetGPUDescriptorHandleForHeapStart();
	cmd->SetDescriptorHeaps(1, _materialHeap.GetAddressOf());

	unsigned int offset = 0;
	for (auto& m : _model.materials)
	{
		cmd->SetGraphicsRootDescriptorTable(1, matH);
		cmd->DrawIndexedInstanced(m.face_vert_cnt, 1, offset, 0, 0);
		matH.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		offset += m.face_vert_cnt;
	}
}

