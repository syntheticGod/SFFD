#pragma once



//#define TEST_DISPLAY_GC_POS_ERROR
//#define TEST_DISPLAY_CYM_POS_ERROR
//#define TEST_DISPLAY_GC_NORMAL_ERROR
//#define TEST_DISPLAY_CYM_NORMAL_ERROR
//#define TEST_DISPLAY_BOTH_POS_ERROR
//#define TEST_DISPLAY_BOTH_NORMAL_ERROR

//��������Ϣ���ļ�
#define TEST_DISPLAY_CYM_WITHTEX

//�����㱻����
#define MULTI_SamplePoints

#include <fstream>
#include <sstream>
#include <GLEW\glew.h>
#include <vector>
#include "shader.h"
#include <glm/glm.hpp>
#include <map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
struct Vertex;
struct Texture;

using std::string;

typedef glm::vec3 Vector3;

const string FileStr_OriginSamplePoint = "������ԭʼ��Ϣ.txt";
const string FileStr_CYMResult_Normal = "CYM�����÷���.txt";
const string FileStr_CYMResult_Position = "CYM�����ü�������.txt";
const string FileStr_CYMResult_Texcord = "CYM��������������.txt";

const string FileStr_CYMSampleResult_Normal = "CYM������������.txt";
const string FileStr_CYMSampleResult_Position = "CYM����������������.txt";
const string FileStr_MySampleResult_Position = "GC����������������_";
const string FileStr_MySampleResult_Normal = "GC������������_";
const string FileStr_GroundTruth_Position = "��������κ�GroundTruth_��������.txt";
const string FileStr_GroundTruth_Normal = "��������κ�GroundTruth_������.txt";

const string FileStr_CompareFileStr = "�ԱȽ��/�ԱȽ��_";
const string FileStr_ErrorFile_mypos = "GCErrorFile/pos_GC_";
const string FileStr_ErrorFile_cympos = "CYMErrorFile/pos.txt";
const string FileStr_ErrorFile_mynormal = "GCErrorFile/normal_GC_";
const string FileStr_ErrorFile_cymnormal = "CYMErrorFile/normal.txt";
const string FileStr_ErrorFile_bothpos = "BothErrorFile/pos_both_";
const string FileStr_ErrorFile_bothnormal = "BothErrorFile/normal_both_";

const string FileStr_CompareDataList_pos = "�Ա�������_����λ��_";
const string FileStr_CompareDataList_normal = "�Ա�������_������_";
const string FileStr_CompareDataList_difference = "�Ա�������_��ֵ_";
//ϸ������
const int tessNums = 6;

struct CYMSamplePoint {
	int samplePointsListIndex;
	int originTriangleIndex;
	glm::vec3 centroidCoordinates;
	glm::vec3 position;
	glm::vec2 texCord;
	glm::vec3 normal;
	CYMSamplePoint(){}
	CYMSamplePoint(int listIndex,int faceIndex, glm::vec3& cen_pos,glm::vec3& pos, glm::vec2& tex,glm::vec3& nor) {
		originTriangleIndex = faceIndex;
		centroidCoordinates = cen_pos;
		position = pos;
		texCord = tex;
		normal = nor;
		samplePointsListIndex = listIndex;
	}
};


class CYMResult {
public:
	std::vector<CYMSamplePoint> originSamplePoints;
	//��Ϊԭʼ�������ε��±�
	std::map<int, std::vector<CYMSamplePoint>> originSamplePointsMap;

	//��ȡ�ļ���ȡԭʼ��������Ϣ
	void loadFile_OriginSamplePoint();
	//��ȡCYM�Ľ�����ݣ�ֱ�����ڻ��ƣ������㣩
	void loadModel_CYMResult();
	//float cutspace��ѡ�õ��ʷּ��
	void output_groundTruthCompareFile(float cutspace,float smoothSharplimit);
	void draw(const Shader& shader) const;
private:
	std::vector<Vertex> cymResultData;
	std::vector<GLuint> indices;
	std::vector<glm::vec3> position_groundTruth;
	std::vector<glm::vec3> normal_groundTruth;
	std::vector<glm::vec3> position_myResult;
	std::vector<glm::vec3> normal_myResult;
	std::vector<glm::vec3> position_cymResult;
	std::vector<glm::vec3> normal_cymResult;

	std::vector<Texture> textures;
	std::string modelFileDir;
	std::map<std::string, Texture> loadedTextureMap;
	void loadModel(const std::string& filePath);
	void processNode(const aiNode* node, const aiScene* sceneObjPtr);
	void processMesh(const aiMesh* meshPtr, const aiScene* sceneObjPtr);
	bool processMaterial(const aiMaterial* matPtr, const aiScene* sceneObjPtr,
		const aiTextureType textureType, std::vector<Texture>& textures);

	void setupBufffer();
	int BigTriangleNums;
	GLuint VAOId, VBOId, EBOId;
	void loadFile_GroundTruth();
	void loadFile_CYMSampleResult();
	void loadFile_MySampleResult(float cutspace,float smoothSharpLimit);
};

bool Point_Is_In_Triangle(const Vector3& A, const Vector3& B, const Vector3& C, const Vector3& P);

//���
class Error {
public:
	//
	Error(float cut_Space,float smoothSharp_Limit) :hasInited(false), cutSpace(cut_Space), smoothSharpLimit(smoothSharp_Limit){}
	void init();
	void display(const Shader& shader); 
private:
	Error() :hasInited(false) {}
	bool hasInited;
	float cutSpace;
	float smoothSharpLimit;
	GLuint VAOId, VBOId[2], EBOId;
	std::vector<glm::vec3> posVec;
	std::vector<float> errorVec;
	std::vector<GLuint> indices_tessellation;
};

//cutSpaceChangingָ�Ƿ�Ϊ�и����������������Լ�����ֵ������
//beginVar��endVar�����������ʼֵ�����ֵ��stepΪ�����仯���ֵ
//����Ϊ�˱���С��������������ļ����жϵı��������һǧ��
void OutputFile_CompareDataList(bool cutSpaceChanging, float beginVar, float endVar, float step, float value_const);

void OutputFile_CompareTimeList(float beginVar, float endVar, float step, int const_angle);

