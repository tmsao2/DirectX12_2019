#pragma once
#include "Model.h"
#include <memory>

class TextureResource;

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

struct IK_Link
{
	int link_Index;					//IK…1 リンクボーンのindex
	bool angleLimit;					//IK…1 角度制限 0:OFF 1:ON
	DirectX::XMFLOAT3 rad_Min;		//IK…1 下限 (x,y,z) -> ラジアン角
	DirectX::XMFLOAT3 rad_Max;		//IK…1 上限 (x,y,z) -> ラジアン角
};

struct IK_Info
{
	int target_Index;				//IK…1 IKターゲットボーンのindex
	int loop;						//IK…1 IKループ回数
	float rad;						//IK…1 IKループ計算時の制限角度(ラジアン角)
	std::vector<IK_Link> links;
};


struct PMXBone
{
	std::wstring boneName;				//ボーン名
	DirectX::XMFLOAT3 pos;				//位置
	int index;							//親のボーンindex
	int transLevel;						//変形階層
	bool linkPoint;						//接続先
	bool rotate;						//回転可能
	bool move;							//移動可能
	bool display;						//表示
	bool operation;						//操作可
	bool ik;							//IK
	bool localGrant;					//ローカル付与
	bool rotateGrant;					//回転付与
	bool moveGrant;						//移動付与
	bool axisFixed;						//軸固定
	bool localAxis;						//ローカル軸
	bool physicsTrans;					//物理後変形
	bool parentTrans;					//外部親変形
	DirectX::XMFLOAT3 linkPoint_Offset;	//接続先…0 座標オフセット、ボーン位置からの相対分
	int linkPoint_Index;				//接続先…1 接続先のボーンindex
	int rotate_Grant_Index;				//回転付与…1 付与親のボーンindex
	float rotate_Grant_Rate;			//回転付与…1 付与率
	int move_Grant_Index;				//移動付与…1 付与親のボーンindex
	float move_Grant_Rate;				//移動付与…1 付与率
	DirectX::XMFLOAT3 axisVec;			//軸固定…1 軸の方向ベクトル
	DirectX::XMFLOAT3 localAxisVec_X;	//ローカル軸…1 X軸の方向ベクトル
	DirectX::XMFLOAT3 localAxisVec_Z;	//ローカル軸…1 Z軸の方向ベクトル
	int key;							//外部親変形…1 Key値

	IK_Info ik_info;
};

struct PMXModelData 
{
	std::string path;
	std::vector<PMXVertex> vertices;
	std::vector<int> indices;
	std::vector<std::string> handle;
	std::vector<PMXMaterial> materials;
	std::vector<PMXBone> bones;
};

class PMXModel:
	public Model
{
private:
	PMXModelData _model;
	std::shared_ptr<TextureResource> _tex;

	void LoadModel(const char* path);
	bool VertexBufferInit(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	bool IndexBufferInit(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	bool MaterialInit(Microsoft::WRL::ComPtr<ID3D12Device> dev);
	bool BoneInit(Microsoft::WRL::ComPtr<ID3D12Device> dev);

	std::string WideToMultiByte(const std::wstring wstr);
public:
	PMXModel(Microsoft::WRL::ComPtr<ID3D12Device> dev,const char* path);
	~PMXModel();
	PMXModelData GetModel();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> MaterialHeap()const;
};

