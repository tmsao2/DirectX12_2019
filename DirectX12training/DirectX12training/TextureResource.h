#pragma once
#include <d3d12.h>
#include <vector>
#include <map>

class TextureResource
{
private:
	ID3D12Device* _dev;
	std::map<std::string, ID3D12Resource*> _resTbl;
public:
	TextureResource(ID3D12Device& dev);
	~TextureResource();
	std::string GetTexPathFromModelAndTexPath(const std::string & modelPath, const char * texPath);
	std::wstring GetWideStringFromString(const std::string str);
	std::pair<std::string, std::string> SplitFileName(const std::string & path, const char splitter);
	std::string GetExtension(const std::string & path);
	ID3D12Resource* LoadTexture(std::string& texPath);
	ID3D12Resource* CreateWhiteTex();
	ID3D12Resource* CreateBlackTex();
	ID3D12Resource* CreateGradationTex();
};

