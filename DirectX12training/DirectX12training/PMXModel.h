#pragma once
#include "Model.h"
#include <memory>

class TextureResource;

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
	unsigned char wdm;
	int bone[4];		//�{�[���̎Q�ƃC���f�b�N�X
	float weight[4];	//�{�[���̃E�G�C�g
	DirectX::XMFLOAT3 sdef_c;	//SDEF-C �l
	DirectX::XMFLOAT3 sdef_r0;	//SDEF-R0�l
	DirectX::XMFLOAT3 sdef_r1;	//SDEF-R1�l
	float edge;					//�G�b�W�{��
};

struct PMXMaterial
{
	DirectX::XMFLOAT4 diffuse;		//�g�U�F
	float power;					//spcular�W��
	DirectX::XMFLOAT3 specular;		//���ːF
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
	std::string materialName;		//�}�e���A����
};

struct IK_Link
{
	int link_Index;					//IK�c1 �����N�{�[����index
	bool angleLimit;					//IK�c1 �p�x���� 0:OFF 1:ON
	DirectX::XMFLOAT3 rad_Min;		//IK�c1 ���� (x,y,z) -> ���W�A���p
	DirectX::XMFLOAT3 rad_Max;		//IK�c1 ��� (x,y,z) -> ���W�A���p
};

struct IK_Info
{
	int target_Index;				//IK�c1 IK�^�[�Q�b�g�{�[����index
	int loop;						//IK�c1 IK���[�v��
	float rad;						//IK�c1 IK���[�v�v�Z���̐����p�x(���W�A���p)
	std::vector<IK_Link> links;
};


struct PMXBone
{
	std::string boneName;				//�{�[����
	DirectX::XMFLOAT3 pos;				//�ʒu
	int index;							//�e�̃{�[��index
	int transLevel;						//�ό`�K�w
	bool linkPoint;						//�ڑ���
	bool rotate;						//��]�\
	bool move;							//�ړ��\
	bool display;						//�\��
	bool operation;						//�����
	bool ik;							//IK
	bool localGrant;					//���[�J���t�^
	bool rotateGrant;					//��]�t�^
	bool moveGrant;						//�ړ��t�^
	bool axisFixed;						//���Œ�
	bool localAxis;						//���[�J����
	bool physicsTrans;					//������ό`
	bool parentTrans;					//�O���e�ό`
	DirectX::XMFLOAT3 linkPoint_Offset;	//�ڑ���c0 ���W�I�t�Z�b�g�A�{�[���ʒu����̑��Ε�
	int linkPoint_Index;				//�ڑ���c1 �ڑ���̃{�[��index
	int rotate_Grant_Index;				//��]�t�^�c1 �t�^�e�̃{�[��index
	float rotate_Grant_Rate;			//��]�t�^�c1 �t�^��
	int move_Grant_Index;				//�ړ��t�^�c1 �t�^�e�̃{�[��index
	float move_Grant_Rate;				//�ړ��t�^�c1 �t�^��
	DirectX::XMFLOAT3 axisVec;			//���Œ�c1 ���̕����x�N�g��
	DirectX::XMFLOAT3 localAxisVec_X;	//���[�J�����c1 X���̕����x�N�g��
	DirectX::XMFLOAT3 localAxisVec_Z;	//���[�J�����c1 Z���̕����x�N�g��
	int key;							//�O���e�ό`�c1 Key�l

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
	bool InitPipeLine(Microsoft::WRL::ComPtr<ID3D12Device> dev);

	std::string WideToMultiByte(const std::wstring wstr);
public:
	PMXModel(Microsoft::WRL::ComPtr<ID3D12Device> dev,const char* path);
	~PMXModel();

	void Update();
	void ShadowDraw(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, D3D12_VIEWPORT& view, D3D12_RECT& rect, ID3D12DescriptorHeap* wvp, int instNum);
	void Draw(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, D3D12_VIEWPORT& view, D3D12_RECT& rect,
		ID3D12DescriptorHeap* wvp, ID3D12DescriptorHeap* shadow,int instNum);
};

