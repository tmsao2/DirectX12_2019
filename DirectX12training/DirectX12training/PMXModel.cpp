#include "PMXModel.h"
#include <iostream>
#include "d3dx12.h"


PMXModel::PMXModel(ID3D12Device& dev, const char* path) :Model(dev, path)
{
	LoadModel(path);
	VertexBufferInit(dev);
	IndexBufferInit(dev);
}

PMXModel::~PMXModel()
{
}

void PMXModel::LoadModel(const char * path)
{
	FILE* fp;
	_model.path = path;
	fopen_s(&fp, _model.path.c_str(), "rb");
	PMXHeader header;
	//ヘッダー
	fread(&header.magic, sizeof(header.magic[0]), _countof(header.magic), fp);
	fread(&header.version, sizeof(header.version), 1, fp);
	fread(&header.byteSize, sizeof(header.byteSize), 1, fp);
	fread(&header.data, sizeof(header.data[0]), _countof(header.data), fp);

	//モデル情報
	int length;
	for (int i = 0; i < 4; ++i)
	{
		fread(&length, sizeof(length), 1, fp);
		fseek(fp, length, SEEK_CUR);
	}

	//頂点情報
	int vertexNum;
	fread(&vertexNum, sizeof(vertexNum), 1, fp);
	_model.vertices.resize(vertexNum);
	for (auto& vertex : _model.vertices)
	{
		fread(&vertex.pos, sizeof(vertex.pos), 1, fp);
		fread(&vertex.normal, sizeof(vertex.normal), 1, fp);
		fread(&vertex.uv, sizeof(vertex.uv), 1, fp);
		vertex.uva.resize(header.data[1]);
		for (auto& add : vertex.uva)
		{
			fread(&add, sizeof(add), 1, fp);
		}
		unsigned char wdm;
		fread(&wdm, sizeof(wdm), 1, fp);
		switch (wdm)
		{
		case 0:
			fread(&vertex.bone, header.data[5], 1, fp);
			break;
		case 1:

			fread(&vertex.bone, header.data[5], 2, fp);
			fread(&vertex.weight, sizeof(vertex.weight[0]), 1, fp);
			break;
		case 2:

			fread(&vertex.bone, header.data[5], _countof(vertex.bone), fp);
			fread(&vertex.weight, sizeof(vertex.weight[0]), _countof(vertex.weight), fp);
		case 3:
			fread(&vertex.bone, header.data[5], 2, fp);
			fread(&vertex.weight, sizeof(vertex.weight[0]), 1, fp);
			fread(&vertex.sdef_c, sizeof(vertex.sdef_c), 1, fp);
			fread(&vertex.sdef_r0, sizeof(vertex.sdef_r0), 1, fp);
			fread(&vertex.sdef_r1, sizeof(vertex.sdef_r1), 1, fp);
			break;
		default:
			break;
		}
		fread(&vertex.edge, sizeof(vertex.edge), 1, fp);
	}

	//インデックス情報
	int idxNum;
	fread(&idxNum, sizeof(idxNum), 1, fp);
	_model.indices.resize(idxNum);
	for (auto& idx : _model.indices)
	{
		fread(&idx, header.data[2], 1, fp);
	}

	//テクスチャ
	int texNum;
	fread(&texNum, sizeof(texNum), 1, fp);
	_model.handle.resize(texNum);
	for (auto& h : _model.handle)
	{
		fread(&length, sizeof(length), 1, fp);
		for (int i = 0; i < length / 2; ++i)
		{
			wchar_t str;
			fread(&str, sizeof(str), 1, fp);
			h += str;
		}
	}

	//マテリアル
	int matNum;
	fread(&matNum, sizeof(matNum), 1, fp);
	_model.materials.resize(matNum);
	for (auto& mat : _model.materials)
	{
		fread(&length, sizeof(length), 1, fp);
		for (int i = 0; i < length / 2; ++i)
		{
			wchar_t str;
			fread(&str, sizeof(str), 1, fp);
			mat.materialName += str;
		}
		fread(&length, sizeof(length), 1, fp);
		for (int i = 0; i < length / 2; ++i)
		{
			wchar_t str;
			fread(&str, sizeof(str), 1, fp);
			mat.materialName += str;
		}
		fread(&mat.diffuse, sizeof(mat.diffuse), 1, fp);
		fread(&mat.specular, sizeof(mat.specular), 1, fp);
		fread(&mat.power, sizeof(mat.power), 1, fp);
		fread(&mat.ambient, sizeof(mat.ambient), 1, fp);
		fread(&mat.bitflag, sizeof(mat.bitflag), 1, fp);
		fread(&mat.edgeColor, sizeof(mat.edgeColor), 1, fp);
		fread(&mat.edgeSize, sizeof(mat.edgeSize), 1, fp);
		fread(&mat.tex, header.data[3], 1, fp);
		fread(&mat.sp_tex, header.data[3], 1, fp);
		fread(&mat.sphere, sizeof(mat.sphere), 1, fp);
		fread(&mat.toonflag, sizeof(mat.toonflag), 1, fp);
		if (mat.toonflag == 0)
		{
			fread(&mat.toonTex, header.data[3], 1, fp);
		}
		else
		{
			fread(&mat.toonTex, sizeof(unsigned char), 1, fp);
		}
		fread(&length, sizeof(length), 1, fp);
		for (int i = 0; i < length / 2; ++i)
		{
			wchar_t str;
			fread(&str, sizeof(str), 1, fp);
			mat.memo += str;
		}
		fread(&mat.vertexNum, sizeof(mat.vertexNum), 1, fp);
	}

	fclose(fp);
}

bool PMXModel::VertexBufferInit(ID3D12Device& dev)
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
	resDesc.Width = sizeof(PMXVertex)*_model.vertices.size();
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

	PMXVertex* vertMap = nullptr;
	result = vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, &_model.vertices[0], sizeof(PMXVertex)*_model.vertices.size());
	vertexBuffer->Unmap(0, nullptr);

	//頂点バッファビューの作成
	_vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	_vbView.StrideInBytes = sizeof(PMXVertex);
	_vbView.SizeInBytes = sizeof(PMXVertex)*_model.vertices.size();

	return true;
}

bool PMXModel::IndexBufferInit(ID3D12Device& dev)
{
	ID3D12Resource* indexBuffer = nullptr;

	auto result = dev.CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(_model.indices.size() * sizeof(_model.indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));

	//インデックスデータ転送
	unsigned int* idxMap = nullptr;
	result = indexBuffer->Map(0, nullptr, (void**)&idxMap);

	std::copy(std::begin(_model.indices), std::end(_model.indices), idxMap);
	indexBuffer->Unmap(0, nullptr);

	_ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	_ibView.Format = DXGI_FORMAT_R32_UINT;
	_ibView.SizeInBytes = _model.indices.size() * sizeof(_model.indices[0]);

	return true;
}


PMXModelData PMXModel::GetModel()
{
	return _model;
}

D3D12_VERTEX_BUFFER_VIEW PMXModel::GetVBV()
{
	return _vbView;
}

D3D12_INDEX_BUFFER_VIEW PMXModel::GetIBV()
{
	return _ibView;
}

