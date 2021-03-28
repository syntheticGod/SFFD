#include "testCYM.h"
#include <iostream>
#include "mode.h"
#include "mesh.h"
#include "cutScheme.h"
#include<sstream>
#include<iostream>
#include<cmath>
#include<iomanip>
#include "texture.h"
#include <map>
const float ZERO = -0.001;

//读取文件获取原始采样点信息
void CYMResult::loadFile_OriginSamplePoint() {
	std::ifstream fs;
	fs.open(FileStr_OriginSamplePoint);
	if (fs) {
		int originTriangleNums=0;
		fs >> originTriangleNums;
		int listIndex = 0;
		for (int i = 0; i < originTriangleNums; i++) {
			int originTriangleIndex = 0;
			float centroidCoordinate[21][3];
			float pos[3][3];
			float tex[3][2];
			float nor[3][3];

			fs >> originTriangleIndex;
#ifdef MULTI_SamplePoints
			for (int k = 0; k < 21; k++)
#else
			for (int k = 0; k < 3; k++)
#endif
				fs >> centroidCoordinate[k][0] >> centroidCoordinate[k][1] >> centroidCoordinate[k][2];			 				
			
			for (int k = 0; k < 3; k++)
				fs >> pos[k][0] >> pos[k][1] >> pos[k][2];

			for (int k = 0; k < 3; k++)
				fs >> nor[k][0] >> nor[k][1] >> nor[k][2];

			for (int k = 0; k < 3; k++)
				fs >> tex[k][0] >> tex[k][1];

#ifdef MULTI_SamplePoints
			for (int k = 0; k < 21; k++){
#else
			for (int k = 0; k < 3; k++) {
#endif
			/*
			originSamplePoints.push_back(CYMSamplePoint(listIndex, originTriangleIndex,
					glm::vec3(centroidCoordinate[k][0], centroidCoordinate[k][1], centroidCoordinate[k][2]),
					glm::vec3(pos[k][0], pos[k][1], pos[k][2]),
					glm::vec2(tex[k][0], tex[k][1]),
					glm::vec3(nor[k][0], nor[k][1], nor[k][2])));
			
			*/
				originSamplePoints.push_back(CYMSamplePoint(listIndex, originTriangleIndex,
					glm::vec3(centroidCoordinate[k][0], centroidCoordinate[k][1], centroidCoordinate[k][2]),
					glm::vec3(),
					glm::vec2(),
					glm::vec3()));
				listIndex++;
			}			    
		}
	}
	fs.close();

	for (CYMSamplePoint samplePoint : originSamplePoints) {
		int originFaceIndex = samplePoint.originTriangleIndex;
		if (originSamplePointsMap.count(originFaceIndex) == 0) {
			std::vector<CYMSamplePoint> tempVec = std::vector<CYMSamplePoint>();
			tempVec.push_back(samplePoint);
			originSamplePointsMap[originFaceIndex] = tempVec;
		}
		else {
			originSamplePointsMap[originFaceIndex].push_back(samplePoint);
		}
	}

}

//读取CYM的结果数据，直接用于绘制（不计算）
void CYMResult::loadModel_CYMResult() {

	cymResultData.clear();
	indices.clear();
	const int tessPointNums = tessNums*(tessNums + 1) / 2;
	BigTriangleNums = 0;

	std::vector<glm::vec3> posList = std::vector<glm::vec3>();
	std::vector<glm::vec3> normalList = std::vector<glm::vec3>();
	std::vector<glm::vec2> texcordList = std::vector<glm::vec2>();
	std::ifstream fs;
	fs.open(FileStr_CYMResult_Position);
	if (fs) {
		fs >> BigTriangleNums;
		for (int bigIndex = 0; bigIndex < BigTriangleNums; bigIndex++) {
			for (int i = 0; i < tessPointNums; i++) {
				float value0, value1, value2;
				fs >> value0 >> value1 >> value2;
				posList.push_back(glm::vec3(value0, value1, value2));
			}
		}
	}
	else
		std::cout << "POS文件读取失败！" << std::endl;
	fs.close();

	fs.open(FileStr_CYMResult_Normal);
	if (fs) {
		fs >> BigTriangleNums;
		for (int bigIndex = 0; bigIndex < BigTriangleNums; bigIndex++) {
			for (int i = 0; i < tessPointNums; i++) {
				float value0, value1, value2;
				fs >> value0 >> value1 >> value2;
				normalList.push_back(glm::vec3(value0, value1, value2));
			}
		}
	}
	else
		std::cout << "normal文件读取失败！" << std::endl;
	fs.close();

#ifdef TEST_DISPLAY_CYM_WITHTEX
	fs.open(FileStr_CYMResult_Texcord);
	if (fs) {
		fs >> BigTriangleNums;
		for (int bigIndex = 0; bigIndex < BigTriangleNums; bigIndex++) {
			for (int i = 0; i < tessPointNums; i++) {
				float value0, value1;
				fs >> value0 >> value1 ;
				texcordList.push_back(glm::vec2(value0, 1-value1));
			}
		}
	}
	else
		std::cout << "纹理坐标文件读取失败！" << std::endl;
	fs.close();
#endif // TEST_DISPLAY_CYM_WITHTEX

	for (int i = 0; i < posList.size(); i++) {
		Vertex v = Vertex(glm::vec4(posList[i],0),glm::vec4(texcordList[i],0,0), glm::vec4(normalList[i], 0));
		cymResultData.push_back(v);
	}

	//计算索引
	
	int tessellationTimes = tessNums-1;
	int tessellationPointNums = tessPointNums;
	//先计算顶角朝上的部分
	for (int workGroupId = 0; workGroupId < BigTriangleNums; workGroupId++) {
		int baseOffset = workGroupId * tessellationPointNums;
		int point_index = 0;//起始点在本地的索引
		int temp_index = 0;
		for (int i = 1; i <= tessellationTimes; i++) {
			for (int j = 0; j < i; j++) {
				indices.push_back(baseOffset + point_index);
				indices.push_back(baseOffset + point_index + i);
				indices.push_back(baseOffset + point_index + i + 1);
				point_index++;
			}
		}

		//再计算顶角朝下的部分
		point_index = 1;//起始点在本地的索引
		for (int i = 2; i <= tessellationTimes; i++) {
			for (int j = 1; j < i; j++) {
				indices.push_back(baseOffset + point_index);
				indices.push_back(baseOffset + point_index + i + 1);
				indices.push_back(baseOffset + point_index + 1);
				point_index++;
			}
			point_index += 1;
		}
	}
	//读取obj并设定纹理
#ifdef TEST_DISPLAY_CYM_WITHTEX
	loadModel("resources/newmodels/chair-obj/chair.obj");
#endif
	//设定缓存
	setupBufffer();
}

void CYMResult::output_groundTruthCompareFile(float cutspace,float smoothSharplimit) {
	std::ostringstream oss;
	oss << round(cutspace*1000);
	std::string cutspaceStr(oss.str());

	std::ostringstream os;
	os << round(smoothSharplimit );
	std::string smoothSharpLimitStr(os.str());
	float angleTransform =180.0f / 3.141592f;

	loadFile_GroundTruth();
	loadFile_CYMSampleResult();
	loadFile_MySampleResult(cutspace, smoothSharplimit);
	string FileStr = FileStr_CompareFileStr + cutspaceStr+"_"+ smoothSharpLimitStr + ".txt";

	oss.str("");
	oss << cutspace;
	std::string cst(oss.str());
	oss.str("");
	oss << smoothSharplimit;
	std::string sst(oss.str());
	string FileStr_gc_posError= FileStr_ErrorFile_mypos+ cst + "_" + sst + ".txt";		
	string FileStr_gc_normalError = FileStr_ErrorFile_mynormal + cst + "_" + sst + ".txt";
	string FileStr_both_posError = FileStr_ErrorFile_bothpos + cst + "_" + sst + ".txt";
	string FileStr_both_normalError = FileStr_ErrorFile_bothnormal + cst + "_" + sst + ".txt";
	string FileStr_cym_posError = FileStr_ErrorFile_cympos;
	string FileStr_cym_normalError = FileStr_ErrorFile_cymnormal;
	std::vector<float> myError_posVec = std::vector<float>();
	std::vector<float> cymError_posVec = std::vector<float>();
	std::vector<float> bothError_posVec = std::vector<float>();
	std::vector<float> myError_normalVec = std::vector<float>();
	std::vector<float> cymError_normalVec = std::vector<float>();
	std::vector<float> bothError_normalVec = std::vector<float>();

	//开始数据比较
	int pointNums = position_groundTruth.size();
	float totalError_pos_me = 0;
	float totalError_normal_me = 0;
	float totalError_pos_cym = 0;
	float totalError_normal_cym = 0;
	float totalError_pos_both = 0;
	float totalError_normal_both = 0;

	float maxError_pos_me = 0;
	float maxError_normal_me = 0;
	float maxError_pos_cym = 0;
	float maxError_normal_cym = 0;
	float maxError_pos_both = 0;
	float maxError_normal_both = 0;

	for (int i = 0; i < pointNums; i++) {
		glm::vec3 realPos = position_groundTruth[i];
		float myError_pos = glm::length(realPos - position_myResult[i]);
		float cymError_pos = glm::length(realPos - position_cymResult[i]);
		float bothError_pos= glm::length(position_myResult[i] - position_cymResult[i]);
		if (myError_pos > maxError_pos_me)
			maxError_pos_me = myError_pos;
		if (cymError_pos > maxError_pos_cym)
			maxError_pos_cym = cymError_pos;
		if (bothError_pos > maxError_pos_both)
			maxError_pos_both = bothError_pos;
		myError_posVec.push_back(myError_pos);
		cymError_posVec.push_back(cymError_pos);
		bothError_posVec.push_back(bothError_pos);
		totalError_pos_me += myError_pos;
		totalError_pos_cym += cymError_pos;
		totalError_pos_both += bothError_pos;
		glm::vec3 realNormal = glm::normalize(normal_groundTruth[i]);
		glm::vec3 myNormal = glm::normalize(normal_myResult[i]);
		glm::vec3 cymNormal= glm::normalize(normal_cymResult[i]);		

		float temp = glm::dot(realNormal, cymNormal);
		if (temp > 1.0f)
			temp = 1.0f;
		float cymError_normal = angleTransform*acos(temp);

		temp = glm::dot(realNormal, myNormal);
		if (temp > 1.0f)
			temp = 1.0f;
		float myError_normal = angleTransform*acos(temp);

		temp = glm::dot(myNormal, cymNormal);
		if (temp > 1.0f)
			temp = 1.0f;
		float bothError_normal= angleTransform*acos(temp);

		if (myError_normal > maxError_normal_me)
			maxError_normal_me = myError_normal;
		if (cymError_normal > maxError_normal_cym)
		    maxError_normal_cym = cymError_normal;
		if (bothError_normal > maxError_normal_both)
			maxError_normal_both = bothError_normal;
		myError_normalVec.push_back(myError_normal);
		cymError_normalVec.push_back(cymError_normal);
		bothError_normalVec.push_back(bothError_normal);
		totalError_normal_me += myError_normal;
		totalError_normal_cym += cymError_normal;
		totalError_normal_both += bothError_normal;
		/*
		if (myError_pos > 0.8) {
			std::cout << i << ":  " << position_myResult[i].x << " " << position_myResult[i].y<<" " << position_myResult[i].z<< std::endl;
			std::cout <<   position_cymResult[i].x << " " << position_cymResult[i].y << " " << position_cymResult[i].z << std::endl;
			std::cout << realPos.x << " " << realPos.y << " " << realPos.z << std::endl;
			std::cout << std::endl;
		}
		    */
		//std::cout << i << ":  " << myError_normal << ",  " << cymError_normal << std::endl;
	}
	float myAverageError_pos = totalError_pos_me / pointNums;
	float cymAverageError_pos= totalError_pos_cym / pointNums;
	float bothAverageError_pos = totalError_pos_both / pointNums;

	float myAverageError_normal = totalError_normal_me / pointNums;
	float cymAverageError_normal = totalError_normal_cym / pointNums;
	float bothAverageError_normal = totalError_normal_both / pointNums;
	std::ofstream fs;
	fs.open(FileStr);
	if (fs) {
		fs << myAverageError_pos << " " << maxError_pos_me << std::endl;
		fs << cymAverageError_pos << " " << maxError_pos_cym << std::endl;
		fs<< std::endl;
		fs << myAverageError_normal << " " << maxError_normal_me << std::endl;
		fs << cymAverageError_normal << " " << maxError_normal_cym << std::endl;
		fs << std::endl;
		fs << bothAverageError_pos << " " << maxError_pos_both << std::endl;
		fs << bothAverageError_normal << " " << maxError_normal_both<< std::endl;
	}
	fs.close();
	
	fs.open(FileStr_gc_posError);
	if (fs) {
		fs << myError_posVec.size() << std::endl;
		fs << std::endl;

		for (float f : myError_posVec)
			fs << f << std::endl;
	}
	fs.close();

	fs.open(FileStr_gc_normalError);
	if (fs) {
		fs << myError_normalVec.size() << std::endl;
		fs << std::endl;

		for (float f : myError_normalVec)
			fs << f << std::endl;
	}
	fs.close();

	fs.open(FileStr_both_posError);
	if (fs) {
		fs << bothError_posVec.size() << std::endl;
		fs << std::endl;

		for (float f : bothError_posVec)
			fs << f << std::endl;
	}
	fs.close();

	fs.open(FileStr_both_normalError);
	if (fs) {
		fs << bothError_normalVec.size() << std::endl;
		fs << std::endl;

		for (float f : bothError_normalVec)
			fs << f << std::endl;
	}
	fs.close();
	
	fs.open(FileStr_cym_posError);
	if (fs) {
		fs << cymError_posVec.size() << std::endl;
		fs << std::endl;

		for (float f : cymError_posVec)
			fs << f << std::endl;
	}
	fs.close();

	fs.open(FileStr_cym_normalError);
	if (fs) {
		fs << cymError_normalVec.size() << std::endl;
		fs << std::endl;

		for (float f : cymError_normalVec)
			fs << f << std::endl;
	}
	fs.close();
	
}

void CYMResult::loadFile_GroundTruth() {
	std::ifstream fs;
	fs.open(FileStr_GroundTruth_Position);
	if (fs) {
		int pointNums;
		fs >> pointNums;
		
		for (int i = 0; i < pointNums; i++) {
			float x, y, z;
			fs >> x >> y >> z;
			position_groundTruth.push_back(glm::vec3(x, y, z));
		}
	}
	fs.close();

	fs.open(FileStr_GroundTruth_Normal);
	if (fs) {
		int pointNums;
		fs >> pointNums;

		for (int i = 0; i < pointNums; i++) {
			float x, y, z;
			fs >> x >> y >> z;
			normal_groundTruth.push_back(glm::vec3(x, y, z));
		}
	}
	fs.close();
}
void CYMResult::loadFile_CYMSampleResult() {
	std::ifstream fs;
	fs.open(FileStr_CYMSampleResult_Position);
	if (fs) {
		int pointNums;
		fs >> pointNums;
#ifdef MULTI_SamplePoints
		pointNums = pointNums * 21;
#else
		pointNums = pointNums * 3;
#endif
		for (int i = 0; i < pointNums; i++) {
			float x, y, z;
			fs >> x >> y >> z;
			position_cymResult.push_back(glm::vec3(x, y, z));
		}
	}
	fs.close();

	fs.open(FileStr_CYMSampleResult_Normal);
	if (fs) {
		int pointNums;
		fs >> pointNums;
#ifdef MULTI_SamplePoints
		pointNums = pointNums * 21;
#else
		pointNums = pointNums * 3;
#endif
		for (int i = 0; i < pointNums; i++) {
			float x, y, z;
			fs >> x >> y >> z;
			normal_cymResult.push_back(glm::vec3(x, y, z));
		}
	}
	fs.close();
}
void CYMResult::loadFile_MySampleResult(float cutspace, float smoothSharpLimit) {
	std::ostringstream oss;
	oss << cutspace;
	std::string cutspaceStr(oss.str());

	std::ostringstream os;
	os << smoothSharpLimit;
	std::string smoothSharpLimitStr(os.str());
	string fileStr_pos = FileStr_MySampleResult_Position + cutspaceStr +"_"+ smoothSharpLimitStr+".txt";
	string fileStr_normal = FileStr_MySampleResult_Normal + cutspaceStr + "_" + smoothSharpLimitStr + ".txt";

	std::ifstream fs;
	fs.open(fileStr_pos);
	if (fs) {
		int pointNums;
		fs >> pointNums;
#ifdef MULTI_SamplePoints
		pointNums = pointNums * 21;
#else
		pointNums = pointNums * 3;
#endif
		for (int i = 0; i < pointNums ; i++) {
			float x, y, z;
			fs >> x >> y >> z;
			position_myResult.push_back(glm::vec3(x, y, z));
		}
	}
	else 
		std::cout << fileStr_pos << "读取失败!" << std::endl;	
	fs.close();

	fs.open(fileStr_normal);
	if (fs) {
		int pointNums;
		fs >> pointNums;
#ifdef MULTI_SamplePoints
		pointNums = pointNums * 21;
#else
		pointNums = pointNums * 3;
#endif
		for (int i = 0; i < pointNums ; i++) {
			float x, y, z;
			fs >> x >> y >> z;
			normal_myResult.push_back(glm::vec3(x, y, z));
		}
	}
	else
		std::cout << fileStr_normal << "读取失败!" << std::endl;
	fs.close();
}

void CYMResult::setupBufffer() {
	glGenVertexArrays(1, &this->VAOId);
	glGenBuffers(1, &this->EBOId);
	glGenBuffers(1, &this->VBOId);
	glBindVertexArray(this->VAOId);


	glBindBuffer(GL_ARRAY_BUFFER, VBOId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)* cymResultData.size(), &cymResultData[0], GL_STATIC_DRAW);


	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// 顶点纹理坐标
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid*)(4 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(1);
	// 顶点法向量属性
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid*)(8 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(2);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBOId);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* this->indices.size(),&this->indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* indices.size(), &indices[0], GL_STATIC_DRAW);
	glBindVertexArray(0);
}

void CYMResult::draw(const Shader& shader)const {
	shader.use();
	glBindVertexArray(this->VAOId);
#ifdef TEST_DISPLAY_CYM_WITHTEX
	int diffuseCnt = 0, specularCnt = 0, texUnitCnt = 0;
	for (std::vector<Texture>::const_iterator it = this->textures.begin();
		this->textures.end() != it; ++it)
	{
		switch (it->type)
		{
		case aiTextureType_DIFFUSE:
		{
			glActiveTexture(GL_TEXTURE0 + texUnitCnt);
			glBindTexture(GL_TEXTURE_2D, it->id);
			std::stringstream samplerNameStr;
			samplerNameStr << "texture_diffuse" << diffuseCnt++;
			glUniform1i(glGetUniformLocation(shader.programId,
				samplerNameStr.str().c_str()), texUnitCnt++);
		}
		break;
		case aiTextureType_SPECULAR:
		{
			glActiveTexture(GL_TEXTURE0 + texUnitCnt);
			glBindTexture(GL_TEXTURE_2D, it->id);
			std::stringstream samplerNameStr;
			samplerNameStr << "texture_specular" << specularCnt++;
			glUniform1i(glGetUniformLocation(shader.programId,
				samplerNameStr.str().c_str()), texUnitCnt++);
		}
		break;
		default:
			std::cerr << "Warning::Mesh::draw, texture type" << it->type
				<< " current not supported." << std::endl;
			break;
		}
	}
#endif // TEST_DISPLAY_CYM_WITHTEX	

	glPolygonMode(GL_FRONT_AND_BACK, (System_mode::draw_mode == normal_mode) ? GL_FILL : GL_LINE);

	GLint wireFrameboolLoc = glGetUniformLocation(shader.programId, "draw_wireframe");
	GLboolean wire = (System_mode::draw_mode == mesh_mode);
	glUniform1i(wireFrameboolLoc, wire);

	glLineWidth(1.0f);
	glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
		    		
	glBindVertexArray(0);
	glUseProgram(0);

}


bool Point_Is_In_Triangle(const Vector3& A, const Vector3& B, const Vector3& C, const Vector3& P)
{
	glm::vec3 p1 = glm::vec3(P.x, P.y, 1.0f - P.x - P.y);
	Vector3 v0 = C - A;
	Vector3 v1 = B - A;
	Vector3 v2 = p1 - A;

	double dot00 = glm::dot(v0, v0);
	double dot01 = glm::dot(v0, v1);
	double dot02 = glm::dot(v0, v2);
	double dot11 = glm::dot(v1, v1);
	double dot12 = glm::dot(v1, v2);

	double inverDeno = 1 / (dot00 * dot11 - dot01 * dot01);

	double u = (dot11 * dot02 - dot01 * dot12) * inverDeno;
	if (u < ZERO || u > 1.01f) // if u out of range, return directly
	{
		return false;
	}

	float v = (dot00 * dot12 - dot01 * dot02) * inverDeno;
	if (v < ZERO || v > 1.01f) // if v out of range, return directly
	{
		return false;
	}
	//注意边界问题
	return u + v <= 1.01f;
}

void Error::init() {
	std::ostringstream oss;
	oss << cutSpace;
	std::string cutspaceStr(oss.str());

	std::ostringstream os;
	os << smoothSharpLimit;
	std::string smoothSharpLimitStr(os.str());
	string latterStr = cutspaceStr + "_" + smoothSharpLimitStr+".txt";
	string FileStr_error="";
	string FileStr_posData="";

#ifdef TEST_DISPLAY_GC_POS_ERROR
	FileStr_posData = FileStr_MySampleResult_Position + latterStr;
	FileStr_error = FileStr_ErrorFile_mypos + latterStr;	
#endif 

#ifdef TEST_DISPLAY_CYM_POS_ERROR
	FileStr_posData = FileStr_CYMSampleResult_Position;
	FileStr_error = FileStr_ErrorFile_cympos;
#endif 

#ifdef TEST_DISPLAY_GC_NORMAL_ERROR
	FileStr_posData = FileStr_MySampleResult_Position + latterStr;
	FileStr_error = FileStr_ErrorFile_mynormal + latterStr;
#endif
	
#ifdef TEST_DISPLAY_CYM_NORMAL_ERROR
	FileStr_posData = FileStr_CYMSampleResult_Position;
	FileStr_error = FileStr_ErrorFile_cymnormal;
#endif
	
#ifdef TEST_DISPLAY_BOTH_POS_ERROR
	FileStr_posData = FileStr_MySampleResult_Position + latterStr;
	FileStr_error = FileStr_ErrorFile_bothpos + latterStr;
#endif 

#ifdef TEST_DISPLAY_BOTH_NORMAL_ERROR
	FileStr_posData = FileStr_MySampleResult_Position + latterStr;
	FileStr_error = FileStr_ErrorFile_bothnormal + latterStr;
#endif 
	std::ifstream fs;
	fs.open(FileStr_posData);
	if (fs) {
		int nums;
		fs >> nums;
#ifdef MULTI_SamplePoints
		nums = nums * 21;
#else
		nums = nums * 3;
#endif
		for (int i = 0; i < nums; i++) {
			float x, y, z;
			fs >> x >> y >> z;
			posVec.push_back(glm::vec3(x, y, z));
		}
	}
	else {
		std::cout << FileStr_posData << "读取失败！" << std::endl;
		return;
	}		
	fs.close();

	fs.open(FileStr_error);
	if (fs) {
		int nums;
		fs >> nums;
		for (int i = 0; i < nums; i++) {
			float x;
			fs >> x ;
			errorVec.push_back(x);
		}
	}
	else {
		std::cout << FileStr_error << "读取失败！" << std::endl;
		return;
	}
	fs.close();

	//设置绘制用buffer
	glGenVertexArrays(1, &this->VAOId);	
	glGenBuffers(2, this->VBOId);
#ifdef MULTI_SamplePoints
	glGenBuffers(1, &this->EBOId);
#endif // MULTI_SamplePoints

	glBindVertexArray(this->VAOId);
	glBindBuffer(GL_ARRAY_BUFFER, VBOId[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)* posVec.size(), &posVec[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		sizeof(glm::vec3), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, VBOId[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)* errorVec.size(), &errorVec[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
		sizeof(float), (GLvoid*)0);
	glEnableVertexAttribArray(1);

//如果采样点为加密形式还需要添加索引
#ifdef MULTI_SamplePoints
	//生成索引
	//先计算顶角朝上的部分
	int tessellationTimes = 5;
	int tessellationPointNums = (1 + tessellationTimes)*(2 + tessellationTimes) / 2;

	for (int workGroupId = 0; workGroupId < posVec.size() / 21; workGroupId++) {
		int baseOffset = workGroupId * tessellationPointNums;
		int point_index = 0;//起始点在本地的索引
		int temp_index = 0;
		for (int i = 1; i <= tessellationTimes; i++) {
			for (int j = 0; j < i; j++) {
				indices_tessellation.push_back(baseOffset + point_index);
				indices_tessellation.push_back(baseOffset + point_index + i);
				indices_tessellation.push_back(baseOffset + point_index + i + 1);
				point_index++;
			}
		}

		//再计算顶角朝下的部分
		point_index = 1;//起始点在本地的索引
		for (int i = 2; i <= tessellationTimes; i++) {
			for (int j = 1; j < i; j++) {
				indices_tessellation.push_back(baseOffset + point_index);
				indices_tessellation.push_back(baseOffset + point_index + i + 1);
				indices_tessellation.push_back(baseOffset + point_index + 1);
				point_index++;
			}
			point_index += 1;
		}
	}
	
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBOId);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* this->indices_tessellation.size(),&this->indices_tessellation[0], GL_STATIC_DRAW);

#endif // MULTI_SamplePoints
		
	glBindVertexArray(0);
	hasInited = true;
}

void Error::display(const Shader& shader) {
	shader.use();
	glBindVertexArray(this->VAOId);

#ifdef MULTI_SamplePoints
	glDrawElements(GL_TRIANGLES, this->indices_tessellation.size(), GL_UNSIGNED_INT, 0);
#else
	glDrawArrays(GL_TRIANGLES, 0, posVec.size());
#endif 		    		
	glBindVertexArray(0);
	glUseProgram(0);
}


void OutputFile_CompareTimeList(float beginVar, float endVar, float step, int const_angle) {
	std::ifstream fs;
	std::vector<int> totalTimeVec = std::vector<int>();
	std::vector<int> deformationFrameTimeVec = std::vector<int>();

	std::ostringstream os;
	os <<"时间列表"<< beginVar << "_" << endVar << "_" << step<< "_"<<const_angle << ".txt";
	std::string outputFileStr(os.str());

	for (float var = beginVar; var <= endVar; var += step) {
		std::ostringstream oss;
		oss << std::setprecision(4) << var;
		string cutspaceStr(oss.str());
		oss.str("");
		oss << "_"<<const_angle << ".txt";
		string angleStr(oss.str());
		string tempfileStr = "GC计时文件/time_" + cutspaceStr + angleStr;
		
		fs.open(tempfileStr);
		if (fs) {
			int totalTime, frameTime;
			fs >> totalTime >> frameTime ;
			totalTimeVec.push_back(totalTime);
			deformationFrameTimeVec.push_back(frameTime);
		}
		else {
			std::cout << tempfileStr << "读取失败!" << std::endl;
			fs.close();
			return;
		}
		fs.close();
	}

	std::ofstream ofs_1;
	ofs_1.open(outputFileStr);
	if (ofs_1) {
		int index = 0;
		for (float var = beginVar; var <= endVar; var += step) {
			ofs_1 << var << " " << totalTimeVec[index] << " " << deformationFrameTimeVec[index] << std::endl;
			index++;
		}
	}
	ofs_1.close();

	
}

void OutputFile_CompareDataList(bool cutSpaceChanging, float beginVar, float endVar, float step, float value_const) {
	string str1 = (cutSpaceChanging ? "切割间距变量_" : "尖锐阈值变量_");

	std::ostringstream oss;
	oss << beginVar << "_" << endVar << "_" << step <<"_"<< value_const<< ".txt";
	std::string str2(oss.str());

	string fileStr_pos = FileStr_CompareDataList_pos + str1 + str2;
	string fileStr_normal = FileStr_CompareDataList_normal + str1 + str2;
	string fileStr_bothDifference= FileStr_CompareDataList_difference+ str1 + str2;
	std::vector<float> posErrorVec = std::vector<float>();
	std::vector<float> normalErrorVec = std::vector<float>();
	std::vector<float> both_posDifferenceVec = std::vector<float>();
	std::vector<float> both_normalDifferenceVec = std::vector<float>();
	float cymError_pos, cymError_normal;
	std::ifstream fs;
	int dataNums = 0;
	for (float var = beginVar; var <= endVar; var += step) {
		dataNums++;
		std::ostringstream os;
		string tempFileStr = "";
		if (cutSpaceChanging)
			os << FileStr_CompareFileStr << round(var*1000) << "_" << round(value_const) << ".txt";
		else
			os << FileStr_CompareFileStr << round(value_const * 1000) << "_" << round(var) << ".txt";
		tempFileStr = string(os.str());

		fs.open(tempFileStr);
		if (fs) {
			float posError, normalError,posDifference,normalDifference;
			float slash[8];
			fs >> posError >> slash[0] >> cymError_pos >> slash[2] >> normalError >> slash[3] >> cymError_normal >> slash[5];
			fs >> posDifference >> slash[6] >> normalDifference;
			posErrorVec.push_back(posError);
			normalErrorVec.push_back(normalError);
			both_posDifferenceVec.push_back(posDifference);
			both_normalDifferenceVec.push_back(normalDifference);
		}
		else {
			std::cout << tempFileStr << "读取失败!" << std::endl;
			fs.close();
			return;
		}
		fs.close();
	}


	std::ofstream ofs_1;
	ofs_1.open(fileStr_pos);
	std::ofstream ofs_2;
	ofs_2.open(fileStr_normal);
	std::ofstream ofs_3;
	ofs_3.open(fileStr_bothDifference);
	if (ofs_1&&ofs_2&&ofs_3) {
		ofs_1 << dataNums << std::endl;
		ofs_2 << dataNums << std::endl;
		ofs_3 << dataNums << std::endl;
		int index = 0;
		for (float var = beginVar; var <= endVar; var += step) {
			ofs_1 << var << " "<< posErrorVec[index]<< " " << cymError_pos<<std::endl;
			ofs_2 << var << " " << normalErrorVec[index] << " " << cymError_normal << std::endl;
			ofs_3 << var << " " << both_posDifferenceVec[index] << " " << both_normalDifferenceVec[index] << std::endl;
			index++;
		}
	}
	ofs_1.close();
	ofs_2.close();
	ofs_3.close();
}

void CYMResult::loadModel(const std::string& filePath) {
	Assimp::Importer importer;
	const aiScene* sceneObjPtr = importer.ReadFile(filePath, //aiProcess_Triangulate |aiProcess_FlipUVs|aiProcess_GenSmoothNormals
		aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);//这里也可以直接在原数据上生成法向	
	this->modelFileDir = filePath.substr(0, filePath.find_last_of('/'));
    this->processNode(sceneObjPtr->mRootNode, sceneObjPtr);
}

void CYMResult::processNode(const aiNode* node, const aiScene* sceneObjPtr) {
	
	if (!node || !sceneObjPtr)
	{
		return ;
	}
	// 先处理自身结点
	for (size_t i = 0; i < node->mNumMeshes; ++i)
	{
		// 注意node中的mesh是对sceneObject中mesh的索引
		const aiMesh* meshPtr = sceneObjPtr->mMeshes[node->mMeshes[i]];
		if (meshPtr)
		{			
			this->processMesh(meshPtr, sceneObjPtr);
		}
	}

	// 处理孩子结点
	for (size_t i = 0; i < node->mNumChildren; ++i)
	{
		if (i == 0)//目前只处理一个节点，即一个模型
			this->processNode(node->mChildren[i], sceneObjPtr);
	}	
}

void CYMResult::processMesh(const aiMesh* meshPtr, const aiScene* sceneObjPtr) {
	// 获取纹理数据
	if (meshPtr->mMaterialIndex >= 0)
	{
		const aiMaterial* materialPtr = sceneObjPtr->mMaterials[meshPtr->mMaterialIndex];
		// 获取diffuse类型
		std::vector<Texture> diffuseTexture;
		this->processMaterial(materialPtr, sceneObjPtr, aiTextureType_DIFFUSE, diffuseTexture);
		textures.insert(textures.end(), diffuseTexture.begin(), diffuseTexture.end());
		// 获取specular类型
		std::vector<Texture> specularTexture;
		this->processMaterial(materialPtr, sceneObjPtr, aiTextureType_SPECULAR, specularTexture);
		textures.insert(textures.end(), specularTexture.begin(), specularTexture.end());
	}
}


bool CYMResult::processMaterial(const aiMaterial* matPtr, const aiScene* sceneObjPtr, const aiTextureType textureType, std::vector<Texture>& textures)
{
	textures.clear();

	if (!matPtr
		|| !sceneObjPtr)
	{
		return false;
	}
	if (matPtr->GetTextureCount(textureType) <= 0)
	{
		return true;
	}
	for (size_t i = 0; i < matPtr->GetTextureCount(textureType); ++i)
	{
		Texture text;
		aiString textPath;
		aiReturn retStatus = matPtr->GetTexture(textureType, i, &textPath);
		if (retStatus != aiReturn_SUCCESS
			|| textPath.length == 0)
		{
			std::cerr << "Warning, load texture type=" << textureType
				<< "index= " << i << " failed with return value= "
				<< retStatus << std::endl;
			continue;
		}
		std::string absolutePath = modelFileDir + "/" + textPath.C_Str();
		std::map<std::string, Texture>::const_iterator it = loadedTextureMap.find(absolutePath);
		if (it == this->loadedTextureMap.end()) // 检查是否已经加载过了
		{
			GLuint textId = TextureHelper::load2DTexture(absolutePath.c_str());
			text.id = textId;
			text.path = absolutePath;
			text.type = textureType;
			textures.push_back(text);
			loadedTextureMap[absolutePath] = text;
		}
		else
		{
			textures.push_back(it->second);
		}
	}
	return true;
}