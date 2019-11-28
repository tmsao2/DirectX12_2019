#pragma once
#include "Primitive.h"
class Plane :
	public Primitive
{
private:
	bool InitPipeLine(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	bool InitVertex(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertexBuffer = nullptr;
	//頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
public:
	Plane(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	~Plane();
	void Draw(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, D3D12_VIEWPORT& view, D3D12_RECT& rect, ID3D12DescriptorHeap* wvp, ID3D12DescriptorHeap* shadow);

};

