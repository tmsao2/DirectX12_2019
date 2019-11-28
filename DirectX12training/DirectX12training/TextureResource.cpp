#include "TextureResource.h"
#include <DirectXTex.h>
#pragma comment(lib,"DirectXTex.lib")


TextureResource::TextureResource(Microsoft::WRL::ComPtr<ID3D12Device> dev):_dev(dev)
{
}


TextureResource::~TextureResource()
{
}

std::string TextureResource::GetTexPathFromModelAndTexPath(const std::string & modelPath, const char * texPath)
{
	int pathIdx1 = modelPath.rfind("/");
	int pathIdx2 = modelPath.rfind("\\");
	auto pathIdx = max(pathIdx1, pathIdx2);
	auto folderPath = modelPath.substr(0, pathIdx);
	return folderPath + "/" + texPath;
}

std::wstring TextureResource::GetWideStringFromString(const std::string str)
{
	auto num1 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, nullptr, 0);
	std::wstring wstr;
	wstr.resize(num1);
	auto num2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, &wstr[0], num1);
	return wstr;
}

std::pair<std::string, std::string> TextureResource::SplitFileName(const std::string & path, const char splitter)
{
	int idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	return ret;
}

std::string TextureResource::GetExtension(const std::string & path)
{
	int idx = path.rfind(".");
	return path.substr(idx + 1, path.length() - idx - 1);
}


Microsoft::WRL::ComPtr<ID3D12Resource> TextureResource::LoadTexture(std::string & texPath)
{
	auto result = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	//テクスチャ読み込み
	DirectX::TexMetadata meta;
	DirectX::ScratchImage img;
	auto it = _resTbl.find(texPath);
	if (it != _resTbl.end()) {
		return it->second;
	}
	auto wstr = GetWideStringFromString(texPath);
	auto ext = GetExtension(texPath);
	if (ext == "png" || ext == "bmp" || ext == "jpg" || ext == "sph" || ext == "spa")
	{
		result = LoadFromWICFile(wstr.c_str(), DirectX::WIC_FLAGS_NONE, &meta, img);
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

	

	//テクスチャオブジェクト生成
	result = _dev->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&buffer));

	if (FAILED(result))
	{
		return nullptr;
	}

	result = buffer->WriteToSubresource(0,
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



Microsoft::WRL::ComPtr<ID3D12Resource> TextureResource::CreateWhiteTex()
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 4;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
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

Microsoft::WRL::ComPtr<ID3D12Resource> TextureResource::CreateBlackTex()
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 4;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

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

Microsoft::WRL::ComPtr<ID3D12Resource> TextureResource::CreateGradationTex()
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 4;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

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
