#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

struct Material {
	DirectX::XMFLOAT4 diffuse;
	float power;
	DirectX::XMFLOAT3 specular;
	DirectX::XMFLOAT3 ambient;
};

class Model
{
protected:
	//���_�o�b�t�@�r���[
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	//�C���f�b�N�X�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	virtual void LoadModel(const char* path)=0;
	virtual bool VertexBufferInit(ID3D12Device& dev)=0;
	virtual bool IndexBufferInit(ID3D12Device& dev)=0;
public:
	Model(ID3D12Device& dev, const char* path);
	~Model();
	D3D12_VERTEX_BUFFER_VIEW GetVBV();
	D3D12_INDEX_BUFFER_VIEW GetIBV();
};

