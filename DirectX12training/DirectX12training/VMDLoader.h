#pragma once
#include <DirectXMath.h>
#include <vector>
#include <map>

struct VMDMotion
{
	char boneName[15];					// �{�[����
	int frameNo;						// �t���[���ԍ�(�Ǎ����͌��݂̃t���[���ʒu��0�Ƃ������Έʒu)
	DirectX::XMFLOAT3 location;			// �ʒu
	DirectX::XMFLOAT4 rotation;			// Quaternion // ��]
	unsigned char interpolation[64];	// [4][4][4] // �⊮
};

struct VMDSkin
{
	char skinName[15];		// �\�
	int frameNo;			// �t���[���ԍ�
	float weight;			// �\��̐ݒ�l(�\��X���C�_�[�̒l)
};

struct VMDCamera
{
	int frameNo;						// �t���[���ԍ�
	float length;						// -(����)
	DirectX::XMFLOAT3 location;			// �ʒu
	DirectX::XMFLOAT3 rotation;			// �I�C���[�p // X���͕��������]���Ă���̂Œ��� // ��]
	unsigned short interpolation[24];	// �����炭[6][4](������) // �⊮
	unsigned long viewingAngle;			// ���E�p
	unsigned short perspective;			// 0:on 1:off // �p�[�X�y�N�e�B�u
};

struct VMDLight
{
	int frameNo;				// �t���[���ԍ�
	DirectX::XMFLOAT3 color;	// RGB�e�l/256 // �ԁA�΁A��
	DirectX::XMFLOAT3 location; // X, Y, Z
};

struct VMDSelfShadow
{
	int frameNo;			// �t���[���ԍ�
	unsigned short mode;	// 00-02 // ���[�h
	float distance;			// 0.1 - (dist * 0.00001) // ����
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
	DirectX::XMFLOAT3 pos;
	KeyFrame() :frameNo(0), quaternion(0, 0, 0, 0),pos(0,0,0) {}
	KeyFrame(int f, DirectX::XMFLOAT4 q, DirectX::XMFLOAT3 p) :frameNo(f), quaternion(q),pos(p) {}
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

