#include "PMXModel.h"
#include <iostream>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include "TextureResource.h"
#include "VMDLoader.h"

PMXModel::PMXModel(Microsoft::WRL::ComPtr<ID3D12Device> dev, const char* path) :Model(dev, path)
{
	_tex.reset(new TextureResource(dev));
	LoadModel(path);
	VertexBufferInit(dev);
	IndexBufferInit(dev);
	MaterialInit(dev);
	BoneInit(dev);
	CreateRootSignature(dev);
	InitPipeLine(dev);
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
	int cnt = 0;
	for (auto& vertex : _model.vertices)
	{
		cnt++;
		fread(&vertex.pos, sizeof(vertex.pos), 1, fp);
		fread(&vertex.normal, sizeof(vertex.normal), 1, fp);
		fread(&vertex.uv, sizeof(vertex.uv), 1, fp);
		vertex.uva.resize(header.data[1]);
		for (auto& add : vertex.uva)
		{
			fread(&add, sizeof(add), 1, fp);
		}
		fread(&vertex.wdm, sizeof(vertex.wdm), 1, fp);
		switch (vertex.wdm)
		{
		case 0:
			fread(&vertex.bone, header.data[5], 1, fp);
			break;
		case 1:
			for (int i = 0; i < 2; ++i)
			{
				fread(&vertex.bone[i], header.data[5], 1, fp);
			}
			fread(&vertex.weight, sizeof(vertex.weight[0]), 1, fp);
			break;
		case 2:
			for (int i = 0; i < _countof(vertex.bone); ++i)
			{
				fread(&vertex.bone[i], header.data[5], 1, fp);
			}
			fread(&vertex.weight, sizeof(vertex.weight[0]), 4, fp);
			break;
		case 3:
			for (int i = 0; i < 2; ++i)
			{
				fread(&vertex.bone[i], header.data[5], 1, fp);
			}
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
		std::wstring wstr;
		for (int i = 0; i < length / 2; ++i)
		{
			wchar_t wc;
			fread(&wc, sizeof(wc), 1, fp);
			wstr += wc;
		}
		h = WideToMultiByte(wstr);
	}

	//マテリアル
	int matNum;
	fread(&matNum, sizeof(matNum), 1, fp);
	_model.materials.resize(matNum);
	for (auto& mat : _model.materials)
	{
		fread(&length, sizeof(length), 1, fp);
		std::wstring wstr;
		for (int i = 0; i < length / 2; ++i)
		{
			wchar_t wc;
			fread(&wc, sizeof(wc), 1, fp);
			wstr += wc;
		}
		mat.materialName = WideToMultiByte(wstr);
		fread(&length, sizeof(length), 1, fp);
		fseek(fp, length, SEEK_CUR);

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

	//ボーン
	int boneNum;
	fread(&boneNum, sizeof(boneNum), 1, fp);
	_model.bones.resize(boneNum);
	for (auto& bone : _model.bones)
	{
		fread(&length, sizeof(length), 1, fp);
		std::wstring wstr;
		for (int i = 0; i < length / 2; ++i)
		{
			wchar_t wc;
			fread(&wc, sizeof(wc), 1, fp);
			wstr += wc;
		}
		bone.boneName = WideToMultiByte(wstr);
		fread(&length, sizeof(length), 1, fp);
		fseek(fp, length, SEEK_CUR);
		
		fread(&bone.pos, sizeof(bone.pos), 1, fp);
		fread(&bone.index, header.data[5], 1, fp);
		fread(&bone.transLevel, sizeof(bone.transLevel), 1, fp);
		unsigned char bitFlag[2];

		fread(&bitFlag, sizeof(unsigned char), _countof(bitFlag), fp);
		
		bone.linkPoint = bitFlag[0] & 0x01;
		bone.rotate = bitFlag[0] & 0x02;
		bone.move = bitFlag[0] & 0x04;
		bone.display = bitFlag[0] & 0x08;
		bone.operation = bitFlag[0] & 0x10;
		bone.ik = bitFlag[0] & 0x20;
		bone.localGrant = bitFlag[0] & 0x80;
		bone.rotateGrant = bitFlag[1] & 0x01;
		bone.moveGrant = bitFlag[1] & 0x02;
		bone.axisFixed = bitFlag[1] & 0x04;
		bone.localAxis = bitFlag[1] & 0x08;
		bone.physicsTrans = bitFlag[1] & 0x10;
		bone.parentTrans = bitFlag[1] & 0x20;

		if (bone.linkPoint)
		{
			fread(&bone.linkPoint_Index, header.data[5], 1, fp);
		}
		else
		{
			fread(&bone.linkPoint_Offset, sizeof(bone.linkPoint_Offset), 1, fp);
		}
		if (bone.rotateGrant)
		{
			fread(&bone.rotate_Grant_Index, header.data[5], 1, fp);
			fread(&bone.rotate_Grant_Rate, sizeof(bone.rotate_Grant_Rate), 1, fp);
		}
		if (bone.moveGrant)
		{
			fread(&bone.move_Grant_Index, header.data[5], 1, fp);
			fread(&bone.move_Grant_Rate, sizeof(bone.move_Grant_Rate), 1, fp);
		}
		if (bone.axisFixed)
		{
			fread(&bone.axisVec, sizeof(bone.axisVec), 1, fp);
		}
		if (bone.localAxis)
		{
			fread(&bone.localAxisVec_Z, sizeof(bone.localAxisVec_Z), 1, fp);
			fread(&bone.localAxisVec_Z, sizeof(bone.localAxisVec_Z), 1, fp);
		}
		if (bone.parentTrans)
		{
			fread(&bone.key, sizeof(bone.key), 1, fp);
		}
		if (bone.ik)
		{
			fread(&bone.ik_info.target_Index, header.data[5], 1, fp);
			fread(&bone.ik_info.loop, sizeof(bone.ik_info.loop), 1, fp);
			fread(&bone.ik_info.rad, sizeof(bone.ik_info.rad), 1, fp);
			int cnt;
			fread(&cnt, sizeof(cnt), 1, fp);
			bone.ik_info.links.resize(cnt);
			for (auto& link : bone.ik_info.links)
			{
				fread(&link.link_Index, header.data[5], 1, fp);
				fread(&link.angleLimit, sizeof(link.angleLimit), 1, fp);
				if (link.angleLimit)
				{
					fread(&link.rad_Min, sizeof(link.rad_Min), 1, fp);
					fread(&link.rad_Max, sizeof(link.rad_Max), 1, fp);
				}
			}
		}
	}
	fclose(fp);
}

bool PMXModel::VertexBufferInit(Microsoft::WRL::ComPtr<ID3D12Device> dev)
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
	auto result = dev->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(_vertexBuffer.GetAddressOf()));

	PMXVertex* vertMap = nullptr;
	result = _vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(_model.vertices.begin(),_model.vertices.end(),vertMap);
	_vertexBuffer->Unmap(0, nullptr);

	//頂点バッファビューの作成
	_vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vbView.StrideInBytes = sizeof(PMXVertex);
	_vbView.SizeInBytes = sizeof(PMXVertex)*_model.vertices.size();

	return true;
}

bool PMXModel::IndexBufferInit(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{

	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(_model.indices.size() * sizeof(_model.indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(_indexBuffer.GetAddressOf()));

	//インデックスデータ転送
	unsigned int* idxMap = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxMap);

	std::copy(std::begin(_model.indices), std::end(_model.indices), idxMap);
	_indexBuffer->Unmap(0, nullptr);

	_ibView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_ibView.Format = DXGI_FORMAT_R32_UINT;
	_ibView.SizeInBytes = _model.indices.size() * sizeof(_model.indices[0]);

	return true;
}

bool PMXModel::MaterialInit(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	HRESULT result;

	texResources.resize(_model.materials.size());
	sphResources.resize(_model.materials.size());
	spaResources.resize(_model.materials.size());
	toonResources.resize(_model.materials.size());

	whiteBuffer = _tex->CreateWhiteTex();
	blackBuffer = _tex->CreateBlackTex();

	for (int i = 0; i < _model.materials.size(); ++i)
	{
		texResources[i] = whiteBuffer;
		sphResources[i] = whiteBuffer;
		spaResources[i] = blackBuffer;
		toonResources[i] = whiteBuffer;
		std::string toonFilePath = "toon/";
		char toonFileName[16];
		if (_model.materials[i].toonflag !=0)
		{
			sprintf_s(toonFileName, "toon%02d.bmp", _model.materials[i].toonTex + 1);
			toonFilePath += toonFileName;
			toonResources[i] = _tex->LoadTexture(toonFilePath);
		}
		else if (_model.materials[i].toonTex < 0xff)
		{
			toonFilePath = _tex->GetTexPathFromModelAndTexPath(_model.path, _model.handle[_model.materials[i].toonTex].c_str());
			toonResources[i] = _tex->LoadTexture(toonFilePath);
		}
		std::string texFileName;

		if (_model.materials[i].tex < 0xff)
		{
			texFileName = _model.handle[_model.materials[i].tex];
		}
		std::string filePath;
		if (strlen(texFileName.c_str()) != 0)
		{
			filePath = _tex->GetTexPathFromModelAndTexPath(_model.path, texFileName.c_str());
			texResources[i] = _tex->LoadTexture(filePath);
		}
		if (_model.materials[i].sp_tex < 0xff)
		{
			if (strlen(_model.handle[_model.materials[i].sp_tex].c_str()))
			{
				if (_model.materials[i].sphere == 1)//乗算スフィア
				{
					filePath = _tex->GetTexPathFromModelAndTexPath(_model.path, _model.handle[_model.materials[i].sp_tex].c_str());
					sphResources[i] = _tex->LoadTexture(filePath);
				}
				else if (_model.materials[i].sphere == 2)//加算スフィア
				{
					filePath = _tex->GetTexPathFromModelAndTexPath(_model.path, _model.handle[_model.materials[i].sp_tex].c_str());
					spaResources[i] = _tex->LoadTexture(filePath);
				}
				else//サブテクスチャ(追加UV1のx,yをUV参照して通常テクスチャ描画を行う)
				{

				}
			}
		}
	}

	auto mats = _model.materials;

	size_t size = sizeof(PMXMaterial);
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

		PMXMaterial* matMap = nullptr;
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

bool PMXModel::BoneInit(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	//行列の初期化
	_boneMats.resize(_model.bones.size());
	std::fill(_boneMats.begin(), _boneMats.end(), DirectX::XMMatrixIdentity());
	//マップ情報の構築
	for (int idx = 0; idx < _model.bones.size(); ++idx)
	{
		auto& b = _model.bones[idx];
		auto& boneNode = _boneMap[b.boneName];
		boneNode.boneIdx = idx;
		boneNode.startPos = b.pos;
		if (b.linkPoint && b.linkPoint_Index < 65535)
		{
			boneNode.endPos = _model.bones[b.linkPoint_Index].pos;
		}
		else
		{
			boneNode.endPos = b.linkPoint_Offset;
		}
	}
	//親に追加
	for (auto& b : _boneMap)
	{
		if (_model.bones[b.second.boneIdx].index >= _model.bones.size())continue;
		auto parentName = _model.bones[_model.bones[b.second.boneIdx].index].boneName;
		_boneMap[parentName].children.push_back(&b.second);
	}

	size_t size = sizeof(DirectX::XMMATRIX)* _model.bones.size();
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

	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_boneHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = _boneBuffer->GetGPUVirtualAddress();
	desc.SizeInBytes = size;

	auto h = _boneHeap->GetCPUDescriptorHandleForHeapStart();
	dev->CreateConstantBufferView(&desc, h);

	result = _boneBuffer->Map(0, nullptr, (void**)&_mapedBone);
	std::copy(_boneMats.begin(), _boneMats.end(), _mapedBone);

	return false;
}

bool PMXModel::InitPipeLine(Microsoft::WRL::ComPtr<ID3D12Device> dev)
{
	HRESULT result;
	result = D3DCompileFromFile(L"PMXShader.hlsl", nullptr, nullptr, "PmxVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_vsShader, nullptr);
	result = D3DCompileFromFile(L"PMXShader.hlsl", nullptr, nullptr, "PmxPS", "ps_5_0",
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
		{"ADDUV1",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"ADDUV2",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"ADDUV3",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"ADDUV4",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BONESTATE",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BONENO",0,DXGI_FORMAT_R32G32B32A32_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"WEIGHT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"SV_InstanceID",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	CreatePipeLine(dev, inputLayoutDesc);
	return false;
}

std::string PMXModel::WideToMultiByte(const std::wstring wstr)
{
	auto num1 = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string str;
	str.resize(num1);
	WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wstr.c_str(), -1, &str[0], num1, nullptr, nullptr);
	return str;
}

void PMXModel::Update()
{
	static auto lastTime = GetTickCount();
	if (GetTickCount() - lastTime > _vmd->GetDuration()*33.33333f)
	{
		lastTime = GetTickCount();
	}
	UpdateMotion(static_cast<float>(GetTickCount() - lastTime) / 33.33333f);
}

void PMXModel::ShadowDraw(ID3D12Device * dev, ID3D12GraphicsCommandList * cmd, D3D12_VIEWPORT & view, D3D12_RECT & rect, ID3D12DescriptorHeap * wvp, int instNum)
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

	cmd->DrawIndexedInstanced(_model.indices.size(), instNum, 0, 0, 0);
}

void PMXModel::Draw(ID3D12Device * dev, ID3D12GraphicsCommandList * cmd, D3D12_VIEWPORT & view, D3D12_RECT & rect,
	ID3D12DescriptorHeap * wvp, ID3D12DescriptorHeap * shadow, int instNum)
{
	_model.materials;
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
		cmd->DrawIndexedInstanced(m.vertexNum, instNum, offset, 0, 0);
		matH.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		offset += m.vertexNum;
	}
}
