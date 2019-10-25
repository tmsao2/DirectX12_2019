#pragma once
#include "Model.h"

struct PMXHeader
{
	unsigned char magic[4];	//PMX
	float version;			//バージョン
	unsigned char byteSize;	//後続するデータ列のバイトサイズ  PMX2.0は 8 で固定
	unsigned char data[8];	//バイト列
};

struct PMXVertex
{
	DirectX::XMFLOAT3 pos;		//位置
	DirectX::XMFLOAT3 normal;	//法線
	DirectX::XMFLOAT2 uv;		//UV
	std::vector<DirectX::XMFLOAT4> uva;	//追加UV
	int bone[4];		//ボーンの参照インデックス
	float weight[4];	//ボーンのウエイト
	DirectX::XMFLOAT3 sdef_c;	//SDEF-C 値
	DirectX::XMFLOAT3 sdef_r0;	//SDEF-R0値
	DirectX::XMFLOAT3 sdef_r1;	//SDEF-R1値
	float edge;					//エッジ倍率
};

struct PMXMaterial
{
	std::string materialName;		//マテリアル名
	DirectX::XMFLOAT4 diffuse;		//拡散色
	DirectX::XMFLOAT3 specular;		//反射色
	float power;					//spcular係数
	DirectX::XMFLOAT3 ambient;		//環境色
	unsigned char bitflag;			//描画フラグ(8bit) - 各bit 0:OFF 1:ON　0x01:両面描画, 0x02:地面影, 0x04:セルフシャドウマップへの描画, 0x08:セルフシャドウの描画, 0x10:エッジ描画
	DirectX::XMFLOAT4 edgeColor;	//エッジ色
	float edgeSize;					//エッジサイズ
	int tex;						//通常テクスチャ
	int sp_tex;						//スフィアテクスチャ
	unsigned char sphere;			//スフィアモード
	unsigned char toonflag;			//共有トゥーンフラグ 0:継続値は個別Toon 1:継続値は共有Toon
	int toonTex;
	std::string memo;				//メモ : 自由欄／スクリプト記述／エフェクトへのパラメータ配置など
	int vertexNum;					//マテリアルに対応する面(頂点)数
};

struct PMXModelData 
{
	std::string path;
	std::vector<PMXVertex> vertices;
	std::vector<int> indices;
	std::vector<std::string> handle;
	std::vector<PMXMaterial> materials;
};

class PMXModel:
	public Model
{
private:
	//頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	//インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	void LoadModel(const char* path);
	bool VertexBufferInit(ID3D12Device& dev);
	bool IndexBufferInit(ID3D12Device& dev);
	PMXModelData _model;
public:
	PMXModel(ID3D12Device& dev,const char* path);
	~PMXModel();
	PMXModelData GetModel();
	D3D12_VERTEX_BUFFER_VIEW GetVBV();
	D3D12_INDEX_BUFFER_VIEW GetIBV();
};

