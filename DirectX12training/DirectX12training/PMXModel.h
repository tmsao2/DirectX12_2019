#pragma once
#include "Model.h"

struct PMXHeader
{
	unsigned char magic[4];	//PMX
	float version;			//�o�[�W����
	unsigned char byteSize;	//�㑱����f�[�^��̃o�C�g�T�C�Y  PMX2.0�� 8 �ŌŒ�
	unsigned char data[8];	//�o�C�g��
};

struct PMXVertex
{
	DirectX::XMFLOAT3 pos;		//�ʒu
	DirectX::XMFLOAT3 normal;	//�@��
	DirectX::XMFLOAT2 uv;		//UV
	std::vector<DirectX::XMFLOAT4> uva;	//�ǉ�UV
	int bone[4];		//�{�[���̎Q�ƃC���f�b�N�X
	float weight[4];	//�{�[���̃E�G�C�g
	DirectX::XMFLOAT3 sdef_c;	//SDEF-C �l
	DirectX::XMFLOAT3 sdef_r0;	//SDEF-R0�l
	DirectX::XMFLOAT3 sdef_r1;	//SDEF-R1�l
	float edge;					//�G�b�W�{��
};

struct PMXMaterial
{
	std::string materialName;		//�}�e���A����
	DirectX::XMFLOAT4 diffuse;		//�g�U�F
	DirectX::XMFLOAT3 specular;		//���ːF
	float power;					//spcular�W��
	DirectX::XMFLOAT3 ambient;		//���F
	unsigned char bitflag;			//�`��t���O(8bit) - �ebit 0:OFF 1:ON�@0x01:���ʕ`��, 0x02:�n�ʉe, 0x04:�Z���t�V���h�E�}�b�v�ւ̕`��, 0x08:�Z���t�V���h�E�̕`��, 0x10:�G�b�W�`��
	DirectX::XMFLOAT4 edgeColor;	//�G�b�W�F
	float edgeSize;					//�G�b�W�T�C�Y
	int tex;						//�ʏ�e�N�X�`��
	int sp_tex;						//�X�t�B�A�e�N�X�`��
	unsigned char sphere;			//�X�t�B�A���[�h
	unsigned char toonflag;			//���L�g�D�[���t���O 0:�p���l�͌�Toon 1:�p���l�͋��LToon
	int toonTex;
	std::string memo;				//���� : ���R���^�X�N���v�g�L�q�^�G�t�F�N�g�ւ̃p�����[�^�z�u�Ȃ�
	int vertexNum;					//�}�e���A���ɑΉ������(���_)��
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
	//���_�o�b�t�@�r���[
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	//�C���f�b�N�X�o�b�t�@�r���[
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

