#pragma once
#include "Model.h"
#include <memory>

class TextureResource;


struct PMDHeader {
	char magic[3];		//"PMD"
	float version;		//�o�[�W����
	char model_name[20];//���f����
	char comment[256];	//�R�����g
};

struct PMDVertex {
	DirectX::XMFLOAT3 pos;			// x, y, z //���W
	DirectX::XMFLOAT3 normal;		// nx, ny, nz // �@���x�N�g��
	DirectX::XMFLOAT2 uv;			// u, v // UV ���W // MMD �͒��_ UV
	unsigned short bone_num[2];		// �{�[���ԍ� 1 �A�ԍ� 2 // ���f���ό` ���_�ړ� ���ɉe��
	unsigned char bone_weight;		// �{�[�� 1 �ɗ^����e���x // min:0 max:100 // �{�[�� 2 �ւ̉e���x�́A (100-bone_weig ht)
	unsigned char edge_flag;		// 0: �ʏ�A 1: �G�b�W���� // �G�b�W �֊s ���L���̏ꍇ
};

struct PMDMaterial {
	DirectX::XMFLOAT4 diffuse;	//�g�U����
	float power;				//�X�y�L�����搔
	DirectX::XMFLOAT3 specular;	//���ʔ���
	DirectX::XMFLOAT3 ambient;	//����
	unsigned char toon_index;	//toon?
	unsigned char edge_flag;	//�֊s�A�e
	unsigned long face_vert_cnt;//�ʒ��_��
	char texture_file_name[20];	//�e�N�X�`���t�@�C�����A�X�t�B�A�t�@�C����
};

struct PMDBone {
	char boneName[20];					// �{�[����
	unsigned short parentBoneIndex;		// �e�{�[���ԍ�(�Ȃ��ꍇ��0xFFFF)
	unsigned short tailBoneIndex;		// tail�ʒu�̃{�[���ԍ�(�`�F�[�����[�̏ꍇ��0xFFFF 0 ���⑫2) // �e�F�q��1�F���Ȃ̂ŁA��Ɉʒu���ߗp
	unsigned char boneType;				// �{�[���̎��
	unsigned short ikParentBoneIndex;	// IK�{�[���ԍ�(�e��IK�{�[���B�Ȃ��ꍇ��0)
	DirectX::XMFLOAT3 boneHeadPos;		// x, y, z // �{�[���̃w�b�h�̈ʒu
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
	PMDModelData _model;
	std::shared_ptr<TextureResource> _tex;
	Material mat;
	ID3D12DescriptorHeap* _materialHeap;	//�}�e���A���p


	void LoadModel(const char* path);
	bool VertexBufferInit(ID3D12Device& dev);
	bool IndexBufferInit(ID3D12Device& dev);
	bool MaterialInit(ID3D12Device & dev);
public:
	PMDModel(ID3D12Device& dev, const char* path);
	~PMDModel();
	ID3D12DescriptorHeap* MaterialHeap()const;
	PMDModelData GetModel();
	float GetYFromXOnBezier(float x, DirectX::XMFLOAT2 p1, DirectX::XMFLOAT2 p2,int cnt);
};

