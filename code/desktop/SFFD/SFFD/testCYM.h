#pragma once



//#define TEST_DISPLAY_GC_POS_ERROR
//#define TEST_DISPLAY_CYM_POS_ERROR
//#define TEST_DISPLAY_GC_NORMAL_ERROR
//#define TEST_DISPLAY_CYM_NORMAL_ERROR
//#define TEST_DISPLAY_BOTH_POS_ERROR
//#define TEST_DISPLAY_BOTH_NORMAL_ERROR

//含纹理信息及文件
#define TEST_DISPLAY_CYM_WITHTEX

//采样点被加密
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

const string FileStr_OriginSamplePoint = "采样点原始信息.txt";
const string FileStr_CYMResult_Normal = "CYM绘制用法向.txt";
const string FileStr_CYMResult_Position = "CYM绘制用几何坐标.txt";
const string FileStr_CYMResult_Texcord = "CYM绘制用纹理坐标.txt";

const string FileStr_CYMSampleResult_Normal = "CYM采样点结果法向.txt";
const string FileStr_CYMSampleResult_Position = "CYM采样点结果几何坐标.txt";
const string FileStr_MySampleResult_Position = "GC采样点结果几何坐标_";
const string FileStr_MySampleResult_Normal = "GC采样点结果法向_";
const string FileStr_GroundTruth_Position = "采样点变形后GroundTruth_几何坐标.txt";
const string FileStr_GroundTruth_Normal = "采样点变形后GroundTruth_法向量.txt";

const string FileStr_CompareFileStr = "对比结果/对比结果_";
const string FileStr_ErrorFile_mypos = "GCErrorFile/pos_GC_";
const string FileStr_ErrorFile_cympos = "CYMErrorFile/pos.txt";
const string FileStr_ErrorFile_mynormal = "GCErrorFile/normal_GC_";
const string FileStr_ErrorFile_cymnormal = "CYMErrorFile/normal.txt";
const string FileStr_ErrorFile_bothpos = "BothErrorFile/pos_both_";
const string FileStr_ErrorFile_bothnormal = "BothErrorFile/normal_both_";

const string FileStr_CompareDataList_pos = "对比数据流_几何位置_";
const string FileStr_CompareDataList_normal = "对比数据流_法向量_";
const string FileStr_CompareDataList_difference = "对比数据流_差值_";
//细化段数
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
	//键为原始大三角形的下标
	std::map<int, std::vector<CYMSamplePoint>> originSamplePointsMap;

	//读取文件获取原始采样点信息
	void loadFile_OriginSamplePoint();
	//读取CYM的结果数据，直接用于绘制（不计算）
	void loadModel_CYMResult();
	//float cutspace即选用的剖分间隔
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

//误差
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

//cutSpaceChanging指是否为切割间距作变量，否则以尖锐阈值作变量
//beginVar和endVar代表变量的起始值与结束值，step为变量变化间隔值
//这里为了避免小数计算的误差，用于文件名判断的变量会乘上一千倍
void OutputFile_CompareDataList(bool cutSpaceChanging, float beginVar, float endVar, float step, float value_const);

void OutputFile_CompareTimeList(float beginVar, float endVar, float step, int const_angle);

