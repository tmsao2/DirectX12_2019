#include "PMDModel.h"
#include "d3dx12.h"
#include "TextureResource.h"
#include <iostream>

PMDModel::PMDModel(ID3D12Device& dev, const char* path) :Model(dev, path)
{
	_tex.reset(new TextureResource(dev));
	LoadModel(path);
	VertexBufferInit(dev);
	IndexBufferInit(dev);
	MaterialInit(dev);
}


PMDModel::~PMDModel()
{
}

ID3D12DescriptorHeap * PMDModel::MaterialHeap()const
{
	return _materialHeap;
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

bool PMDModel::VertexBufferInit(ID3D12Device & dev)
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
	ID3D12Resource* vertexBuffer = nullptr;
	auto result = dev.CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&vertexBuffer));

	PMDVertex* vertMap = nullptr;
	result = vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, &_model.vertices[0], sizeof(PMDVertex)*_model.vertCnt);
	vertexBuffer->Unmap(0, nullptr);

	//頂点バッファビューの作成
	_vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	_vbView.StrideInBytes = sizeof(PMDVertex);
	_vbView.SizeInBytes = sizeof(PMDVertex)*_model.vertCnt;

	return true;
}

bool PMDModel::IndexBufferInit(ID3D12Device & dev)
{
	ID3D12Resource* indexBuffer = nullptr;

	auto result = dev.CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(_model.indices.size() * sizeof(_model.indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));

	//インデックスデータ転送
	unsigned short* idxMap = nullptr;
	result = indexBuffer->Map(0, nullptr, (void**)&idxMap);

	std::copy(std::begin(_model.indices), std::end(_model.indices), idxMap);
	indexBuffer->Unmap(0, nullptr);

	_ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	_ibView.Format = DXGI_FORMAT_R16_UINT;
	_ibView.SizeInBytes = _model.indices.size() * sizeof(_model.indices[0]);

	return true;
}

bool PMDModel::MaterialInit(ID3D12Device & dev)
{
	HRESULT result;

	std::vector<ID3D12Resource*> texResources(_model.matCnt);
	std::vector<ID3D12Resource*> sphResources(_model.matCnt);
	std::vector<ID3D12Resource*> spaResources(_model.matCnt);
	std::vector<ID3D12Resource*> toonResources(_model.matCnt);

	ID3D12Resource* whiteBuffer = _tex->CreateWhiteTex();
	ID3D12Resource* blackBuffer = _tex->CreateBlackTex();
	ID3D12Resource* gradBuffer = _tex->CreateGradationTex();

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

	std::vector<ID3D12Resource*> materialBuffer;
	auto mats = _model.materials;

	size_t size = sizeof(Material);
	size = (size + 0xff)&~0xff;

	int midx = 0;
	materialBuffer.resize(mats.size());

	for (auto& matBuff : materialBuffer)
	{
		result = dev.CreateCommittedResource(
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

	result = dev.CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_materialHeap));

	//シェーダーリソースビューの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto h = _materialHeap->GetCPUDescriptorHandleForHeapStart();
	auto inc = dev.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc = {};
	cbDesc.SizeInBytes = size;
	for (int i = 0; i < mats.size(); i++)
	{
		//マテリアル
		cbDesc.BufferLocation = materialBuffer[i]->GetGPUVirtualAddress();
		dev.CreateConstantBufferView(&cbDesc, h);
		h.ptr += inc;
		//テクスチャ
		srvDesc.Format = texResources[i]->GetDesc().Format;
		dev.CreateShaderResourceView(texResources[i], &srvDesc, h);
		h.ptr += inc;
		//スフィア(乗算)
		srvDesc.Format = sphResources[i]->GetDesc().Format;
		dev.CreateShaderResourceView(sphResources[i], &srvDesc, h);
		h.ptr += inc;
		//スフィア(加算)
		srvDesc.Format = spaResources[i]->GetDesc().Format;
		dev.CreateShaderResourceView(spaResources[i], &srvDesc, h);
		h.ptr += inc;
		//トゥーン
		srvDesc.Format = toonResources[i]->GetDesc().Format;
		dev.CreateShaderResourceView(toonResources[i], &srvDesc, h);
		h.ptr += inc;
	}
	return false;
}

PMDModelData PMDModel::GetModel()
{
	return _model;
}
