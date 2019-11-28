#pragma once
#include "Model.h"
#include <memory>
#include <map>

struct PMDHeader {
	char magic[3];		//"PMD"
	float version;		//バージョン
	char model_name[20];//モデル名
	char comment[256];	//コメント
};

struct PMDVertex {
	DirectX::XMFLOAT3 pos;			// x, y, z //座標
	DirectX::XMFLOAT3 normal;		// nx, ny, nz // 法線ベクトル
	DirectX::XMFLOAT2 uv;			// u, v // UV 座標 // MMD は頂点 UV
	unsigned short bone_num[2];		// ボーン番号 1 、番号 2 // モデル変形 頂点移動 時に影響
	unsigned char bone_weight;		// ボーン 1 に与える影響度 // min:0 max:100 // ボーン 2 への影響度は、 (100-bone_weig ht)
	unsigned char edge_flag;		// 0: 通常、 1: エッジ無効 // エッジ 輪郭 が有効の場合
};

struct PMDMaterial {
	DirectX::XMFLOAT4 diffuse;	//拡散反射
	float power;				//スペキュラ乗数
	DirectX::XMFLOAT3 specular;	//鏡面反射
	DirectX::XMFLOAT3 ambient;	//環境光
	unsigned char toon_index;	//toon?
	unsigned char edge_flag;	//輪郭、影
	unsigned long face_vert_cnt;//面頂点数
	char texture_file_name[20];	//テクスチャファイル名、スフィアファイル名
};

struct PMDBone {
	char boneName[20];					// ボーン名
	unsigned short parentBoneIndex;		// 親ボーン番号(ない場合は0xFFFF)
	unsigned short tailBoneIndex;		// tail位置のボーン番号(チェーン末端の場合は0xFFFF 0 →補足2) // 親：子は1：多なので、主に位置決め用
	unsigned char boneType;				// ボーンの種類
	unsigned short ikParentBoneIndex;	// IKボーン番号(影響IKボーン。ない場合は0)
	DirectX::XMFLOAT3 boneHeadPos;		// x, y, z // ボーンのヘッドの位置
};

struct PMDModelData {
	std::string path;
	PMDHeader header;
	unsigned long vertCnt;
	std::vector<PMDVertex> vertices;
	unsigned long idxCnt;
	std::vector<unsigned short> indices;
	unsigned long matCnt;
	std::vector<PMDMaterial> materials;
	unsigned short boneCnt;
	std::vector<PMDBone> bones;
};

class PMDModel :
	public Model
{
private:
	int _frame = 0;

	PMDModelData _model;
	
	void LoadModel(const char* path);
	bool VertexBufferInit(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	bool IndexBufferInit(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	bool MaterialInit(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	bool BoneInit(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	bool InitPipeLine(Microsoft::WRL::ComPtr<ID3D12Device> dev);
public:
	PMDModel(Microsoft::WRL::ComPtr<ID3D12Device> dev, const char* path);
	~PMDModel();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> MaterialHeap()const;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> BoneHeap()const;

	void Update();
	void ShadowDraw(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, D3D12_VIEWPORT& view, D3D12_RECT& rect, ID3D12DescriptorHeap* wvp);
	void Draw(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, D3D12_VIEWPORT& view, D3D12_RECT& rect, ID3D12DescriptorHeap* wvp, ID3D12DescriptorHeap* shadow);
};

