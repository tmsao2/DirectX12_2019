#pragma once
#include <DirectXMath.h>
#include <vector>
#include <map>

struct VMDMotion
{
	char boneName[15];					// ボーン名
	int frameNo;						// フレーム番号(読込時は現在のフレーム位置を0とした相対位置)
	DirectX::XMFLOAT3 location;			// 位置
	DirectX::XMFLOAT4 rotation;			// Quaternion // 回転
	unsigned char interpolation[64];	// [4][4][4] // 補完
};

struct VMDSkin
{
	char skinName[15];		// 表情名
	int frameNo;			// フレーム番号
	float weight;			// 表情の設定値(表情スライダーの値)
};

struct VMDCamera
{
	int frameNo;						// フレーム番号
	float length;						// -(距離)
	DirectX::XMFLOAT3 location;			// 位置
	DirectX::XMFLOAT3 rotation;			// オイラー角 // X軸は符号が反転しているので注意 // 回転
	unsigned short interpolation[24];	// おそらく[6][4](未検証) // 補完
	unsigned long viewingAngle;			// 視界角
	unsigned short perspective;			// 0:on 1:off // パースペクティブ
};

struct VMDLight
{
	int frameNo;				// フレーム番号
	DirectX::XMFLOAT3 color;	// RGB各値/256 // 赤、緑、青
	DirectX::XMFLOAT3 location; // X, Y, Z
};

struct VMDSelfShadow
{
	int frameNo;			// フレーム番号
	unsigned short mode;	// 00-02 // モード
	float distance;			// 0.1 - (dist * 0.00001) // 距離
};

struct VMDData
{
	std::vector<VMDMotion> motion;
	std::vector<VMDSkin> skin;
	std::vector<VMDCamera> camera;
	std::vector<VMDLight> light;
	std::vector<VMDSelfShadow> shadow;
};

struct KeyFrame
{
	int frameNo;
	DirectX::XMFLOAT4 quaternion;
	KeyFrame() :frameNo(0), quaternion(0, 0, 0, 0) {}
	KeyFrame(int f, DirectX::XMFLOAT4 q) :frameNo(f), quaternion(q) {}
};

class VMDLoader
{
private:
	VMDData _vmd;
	int _duration;
	std::map<std::string, std::vector<KeyFrame>> _animData;
public:
	VMDLoader();
	~VMDLoader();
	const std::map <std::string, std::vector<KeyFrame>>& GetAnim()const;
	const int GetDuration()const;
};

