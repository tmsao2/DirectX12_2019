#include "Model.h"
#include <iostream>
#include "d3dx12.h"

Model::Model(ID3D12Device& dev, const char* path)
{
}

Model::~Model()
{
}

D3D12_VERTEX_BUFFER_VIEW Model::GetVBV()
{
	return _vbView;
}

D3D12_INDEX_BUFFER_VIEW Model::GetIBV()
{
	return _ibView;
}
