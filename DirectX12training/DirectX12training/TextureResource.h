#pragma once
#include <d3d12.h>
#include <vector>
#include <map>
#include <wrl.h>

class TextureResource
{
private:
	Microsoft::WRL::ComPtr<ID3D12Device> _dev;
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> _resTbl;
	//テクスチャバッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> whiteBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> blackBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> gradBuff = nullptr;

public:
	TextureResource(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	~TextureResource();
	std::string GetTexPathFromModelAndTexPath(const std::string & modelPath, const char * texPath);
	std::wstring GetWideStringFromString(const std::string str);
	std::pair<std::string, std::string> SplitFileName(const std::string & path, const char splitter);
	std::string GetExtension(const std::string & path);
	Microsoft::WRL::ComPtr<ID3D12Resource> LoadTexture(std::string& texPath);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateWhiteTex();
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBlackTex();
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateGradationTex();
};

