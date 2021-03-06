#pragma once
#ifndef _MESH_H_
#define _MESH_H_

//使用模型本身的法向，这里要求原始模型每个顶点的法向为精确值（也意味着是唯一值，即一个pos对应一个法向）
//#define useModelNormal
//#define TEST_CALSAMPLEPOINT_CYM
//传统FFD
//#define FFD

#include <GLEW/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <string>       
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "shader.h"
#include "mode.h"
#include "globalGPUTest.h"
#include "cutScheme.h"
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <map>
#include <set>
#include "ssbo.h"
#include "testCYM.h"



//#define SHOW_MESH_ORIGIN_DATA



const int test_space = 4;//粒度为4，即质量测试间隔为0.25
const int test_start = 2;//统计质量开始值为2
const int test_end = 6;//统计质量结束值为2

struct Vertex
{
	glm::vec4 position;
	glm::vec4 texCoords;
	glm::vec4 normal;
	Vertex() {}
	Vertex(glm::vec4 &pos, glm::vec4 &tex, glm::vec4 &nor) :position(pos), texCoords(tex), normal(nor) {}
	Vertex operator*(float y) {
		return Vertex(position*y, texCoords*y, normal*y);
	}
	Vertex operator+(Vertex &ver) {
		return Vertex(position + ver.position, texCoords + ver.texCoords, normal + ver.normal);
	}
	bool operator<(const Vertex& v)  const {
		if (position.x < v.position.x)
			return true;
		else if(position.x > v.position.x)
			return false;
		else {
			if (position.y < v.position.y)
				return true;
			else if (position.y > v.position.y)
				return false;
			else {
				if (position.z < v.position.z)
					return true;
				else if (position.z > v.position.z)
					return false;
				else {
					if (normal.x < v.normal.x)
						return true;
					else if (normal.x > v.normal.x)
						return false;
					else {
						if (normal.y < v.normal.y)
							return true;
						else if (normal.y > v.normal.y)
							return false;
						else {
							if (normal.z < v.normal.z)
								return true;
							else 
								return false;
						}
					}
				}
			}
		}

		/*
		bool result = false;
		bool xl = position.x < v.position.x;
		bool xe = position.x == v.position.x;
		bool yl = position.y < v.position.y;
		bool ye = position.y == v.position.y;
		bool zl = position.z < v.position.z;

		if (xl)
			result = true;
		else {
			if (xe && yl)
				result = true;
			else {
				if(xe&&ye&&zl)
					result = true;
			}
		}
		return result;*/
	}
};

struct Vertex_Position:glm::vec3 {
	Vertex_Position(const glm::vec4& v) {
		x = v.x;
		y = v.y;
		z = v.z;
	}
	bool operator<(const Vertex_Position& v) const {
		if (x < v.x)
			return true;
		else if (x > v.x)
			return false;
		else {
			if (y < v.y)
				return true;
			else if (y > v.y)
				return false;
			else {
				if (z < v.z)
					return true;
				else 
					return false;
			}
		}
	}
	/*
	bool operator==(const Vertex_Position& v) const {
		return (pos.x == v.pos.x) && (pos.y == v.pos.y) && (pos.z == v.pos.z);
	}
	*/
};

struct IndiceCouple {
	int indice_1;//ind 1一定比2大
	int indice_2;
	IndiceCouple() {}
	IndiceCouple(int ind_1,int ind_2) {
		indice_1 = std::max(ind_1, ind_2);
		indice_2 = std::min(ind_1, ind_2);
	}
	
	bool operator<(const IndiceCouple& i)  const {
		return ((indice_1<i.indice_1) || (!(indice_1>i.indice_1) && (indice_2<i.indice_2)));
	}
};
//有序的点对，用于查询点的法向，第一个值为起始点（即目标点），第二个值为边的另一个端点
struct IndiceCouple_Ordered {
	int indice_1;//ind 1一定比2大
	int indice_2;
	IndiceCouple_Ordered() {}
	IndiceCouple_Ordered(int ind_1, int ind_2) {
		indice_1 = ind_1;
		indice_2 = ind_2;
	}

	bool operator<(const IndiceCouple_Ordered& i)  const {
		return ((indice_1<i.indice_1) || (!(indice_1>i.indice_1) && (indice_2<i.indice_2)));
	}
};
//对应一条边的光滑尖锐情况
struct Edge_SmoothSharp {
	//领接的两个三角形下标
	int triangleIndex[2];
	int vertexIndex[2];//领接的两个三角形的除该边的另一点的下标
	glm::vec3 triangleNormal[2];
	bool is_smooth;
	bool is_boundary;
	glm::vec3 crossVec;//两个领接面的向量叉乘，仅在尖锐边的时候用到
};
struct Edge {
	int index;//0基，以ab开始,这个是指在一个三角形内的下标
	int length;//指段数
	Edge(){}
	Edge(int ind, int len) :index(ind), length(len) {}
	bool operator<(const Edge& e)const  {
		return length < e.length;
	}
};


// 纹理类型
enum ETextureType
{
	ETextureTypeDiffuse = 1,  // 漫反射
	ETextureTypeSpecular = 2, // 镜面反射
};

// 表示一个Texture
struct Texture
{
	GLuint id;
	aiTextureType type;
	std::string path;
};

// 表示一个用于渲染的最小实体
class Mesh
{
	friend class Model;
public:
	GLuint VAOId, VBOId, EBOId, inputVertexSSBOID, inputIndicesSSBOID, precomputeMatrix_triangleControlPointsSSbo, triangleControlPoints_positionSSbo, triangleControlPoints_normalSSbo,
		vertex_texCord_SSBOID, vertex_Normal_SSBOID,vertex_CP_SSBOID, vertex_bsplineBase_SSBOID, OutputBuffer_VertexSSBOID, OutputBuffer_IndicesSSBOID;//SSBO存放网格数据
	
	void draw(const Shader& shader) const// 绘制Mesh
	{
		shader.use();
		glBindVertexArray(this->VAOId);
		
		int diffuseCnt = 0, specularCnt = 0,texUnitCnt = 0;
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
		//glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, (System_mode::draw_mode==normal_mode)?GL_FILL:GL_LINE);
		
		GLint wireFrameboolLoc = glGetUniformLocation(shader.programId, "draw_wireframe");	
		GLboolean wire = (System_mode::draw_mode == mesh_mode);
		glUniform1i(wireFrameboolLoc, wire);


		glLineWidth(1.0f);
#ifdef FFD
		glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
#else
		glDrawElements(GL_TRIANGLES, this->indices.size()*CutScheme::tessellation_times*CutScheme::tessellation_times, GL_UNSIGNED_INT, 0);
#endif 		    		

		//glDrawArrays(GL_TRIANGLES,0, indices.size());//计算前后点的数量不变，但用这个数量不严谨
		glBindVertexArray(0);
		glUseProgram(0);
		
	}
	Mesh(){
		for (int i = 0; i < (test_end - test_start)*test_space + 1; i++)
			quality_nums[i] = 0;
	}
	Mesh(const std::vector<Vertex>& vertData, 
		const std::vector<Texture> & textures,
		const std::vector<GLuint>& indices) // 构造一个Mesh
	{
		for (int i = 0; i < (test_end - test_start)*test_space + 1; i++)
			quality_nums[i] = 0;
		setData(vertData, textures, indices);
	}
	void setData(const std::vector<Vertex>& vertData,
		const std::vector<Texture> & textures,
		const std::vector<GLuint>& indices)
	{
		//保存原始信息
		this->origin_vertData= vertData;
		this->origin_indices = indices;//三个index一组连续放入
		this->textures = textures;
		/*
#ifdef FFD
		this->vertData= vertData;
		this->indices = indices;
		return;
#endif
*/
#ifdef useModelNormal
		for (int i = 0; i < vertData.size(); i++) {
			Vertex v = vertData[i];
			position_normalMap[Vertex_Position(v.position)] = glm::vec3(v.normal);
		}		
#endif // useModelNormal

#ifdef SHOW_MESH_ORIGIN_DATA
		std::cout << this->origin_indices.size() << std::endl;
		for (int index : this->origin_indices)
			std::cout << this->origin_vertData[index].position.x << " " << this->origin_vertData[index].position.y << " " << this->origin_vertData[index].position.z << std::endl;
#endif // SHOW_MESH_ORIGIN_DATA



		
		std::set<Vertex> temp_positionSet = std::set<Vertex>();
		std::vector<Vertex> temp_positionVec = std::vector<Vertex>();
		for (Vertex tempv : this->origin_vertData) 
			temp_positionSet.insert(Vertex(tempv.position,glm::vec4(), glm::vec4()));
		
		std::set<Vertex>::iterator iter = temp_positionSet.begin();
		int tempIndex = 0;
		while (iter != temp_positionSet.end())
		{
			temp_positionMap[*iter]= tempIndex;
			temp_positionVec.push_back(*iter);
			tempIndex++;
			iter++;
		}
		//提取位置信息
		this->vertData = temp_positionVec;
		this->indices.clear();
		for (GLuint index : this->origin_indices) {
			glm::vec4 origin_pos = origin_vertData[index].position;
			Vertex tempV = Vertex(origin_pos, glm::vec4(), glm::vec4());
			this->indices.push_back(temp_positionMap[tempV]);
		}		        
		


		//为计算新的法向作准备，根据位置进行计算
		defineEdge_SmoothSharp();		
		defineNearMapsAndAngles();
		define_SmoothArea();
		UseCutSchemeNew();		
		
	}
	void final() const
	{
		glDeleteVertexArrays(1, &this->VAOId);
		glDeleteBuffers(1, &this->VBOId);
		glDeleteBuffers(1, &this->EBOId);
	}
	
	~Mesh()
	{
		// 不要再这里释放VBO等空间 因为Mesh对象传递时 临时对象销毁后这里会清理VBO等空间
	}

	Vertex getNearestPoint(float pos_x, float pos_y, float pos_z) {
		/*
		Vertex targetV = Vertex(glm::vec4(pos_x, pos_y, pos_z, 0), glm::vec4(), glm::vec4());
		std::set<Vertex>::iterator it = vertSet.lower_bound(targetV);
		if (it == vertSet.end())
			--it;
		return *it;
		*/
		Vertex result;
		float distanceMin = 1000;
		for (Vertex v : vertData) {
			float dx = pos_x - v.position.x;
			float dy = pos_y - v.position.y;
			float dz = pos_z - v.position.z;
			float tempD = dx*dx + dy*dy + dz*dz;
			if (tempD< distanceMin) {
				distanceMin = tempD;
				result = v;
			}			
		}
		return result;
	}

	void setupComputeData_CP(const Shader& computeShader) {
		glUseProgram(0);
		computeShader.use();
		int pointNums = indices.size();
		setupSSBufferObjectVec<Vertex>(inputVertexSSBOID, 0, vertData, false);
		setupSSBufferObjectVec<GLuint>(inputIndicesSSBOID, 1, indices, false);

		Vertex* outputData = (Vertex*)malloc(sizeof(Vertex) * pointNums/3*64);
		setupSSBufferObjectList<Vertex>(vertex_CP_SSBOID, 2, outputData, pointNums / 3 * 64, false);
		free(outputData);

		glm::vec2* texCordData = (glm::vec2*)malloc(sizeof(glm::vec2) * pointNums);
		setupSSBufferObjectList<glm::vec2>(vertex_texCord_SSBOID, 10, texCordData, pointNums, false);
		free(texCordData);

		glm::vec4* normalData = (glm::vec4*)malloc(sizeof(glm::vec4) * pointNums / 3 * 64);
		setupSSBufferObjectList<glm::vec4>(vertex_Normal_SSBOID, 14, normalData, pointNums / 3 * 64, false);
		free(normalData);

		std::cout << "原始顶点数=" << vertData.size() << std::endl;
		std::cout << "原始索引数=" << indices.size() << std::endl;
		glUniform1i(glGetUniformLocation(computeShader.programId, "vertex_nums"), indices.size());
		glUseProgram(0);
	}
	
	//设置细分所需的数据，只需做一次
	void setupComputeData_tessellation(const Shader& computeShader) {
		computeShader.use();
		int tessellation_times = CutScheme::tessellation_times;
		glUniform1i(glGetUniformLocation(computeShader.programId, "triangle_nums"), indices.size()/3);
		glUniform1i(glGetUniformLocation(computeShader.programId, "tessellationPointNums"), (tessellation_times+2)*(tessellation_times+1)/2);
		glUniform1i(glGetUniformLocation(computeShader.programId, "tessellationTriangleNums"), tessellation_times*tessellation_times);
		glUniform1i(glGetUniformLocation(computeShader.programId, "tessellationTimes"), tessellation_times);

		Vertex* outputData_resultVertex = (Vertex*)malloc(sizeof(Vertex) * indices.size()/3*21);
		setupSSBufferObjectList<Vertex>(OutputBuffer_VertexSSBOID, 12, outputData_resultVertex, indices.size() / 3 * 21, true);
		free(outputData_resultVertex);

		setupIndicesBuffer();

		glUseProgram(0);
	}
	//设置计算三角形控制顶点的数据
	void setupComputeData_bezierTriangle(const Shader& computeShader) {
		glUseProgram(0);
		computeShader.use();
		int triangle_nums = indices.size() / 3;
		glUniform1i(glGetUniformLocation(computeShader.programId, "triangle_nums"), triangle_nums);
		//读取并设置约束拟合矩阵
		std::string fileStr = "precompute_matrix/restraintAndFittingMatrix3_3_3.txt";
		const int fittingPointNums = 55;
		std::ifstream fs(fileStr);
		GLfloat data[10 * (55 + 9)];//最大所需空间
		int index = 0;
		for (int i = 0; i < 10; i++) {//只读前十行
			for (int j = 0; j < 9 + fittingPointNums; j++) {
				fs >> data[index];
				index++;
			}
		}
		fs.close();
		setupSSBufferObjectList<GLfloat>(precomputeMatrix_triangleControlPointsSSbo, 7, data, 10 * (55 + 9), false);

		glm::vec4* contorlPoints_position = (glm::vec4*)malloc(sizeof(glm::vec4) * indices.size() / 3 * 10);
		setupSSBufferObjectList<glm::vec4>(triangleControlPoints_positionSSbo, 8, contorlPoints_position, indices.size() / 3 * 10, true);
		free(contorlPoints_position);
		glm::vec4* contorlPoints_normal = (glm::vec4*)malloc(sizeof(glm::vec4) * indices.size() / 3 * 10);
		setupSSBufferObjectList<glm::vec4>(triangleControlPoints_normalSSbo, 9, contorlPoints_normal, indices.size() / 3 * 10, true);
		free(contorlPoints_normal);
		glUseProgram(0);
	}

	//设置计算点数
	void setupComputeData_point_nums(const Shader& computeShader) {
		glUseProgram(0);
		computeShader.use();
		int point_nums = indices.size() / 3 * 64;
		glUniform1i(glGetUniformLocation(computeShader.programId, "point_nums"), point_nums);		
		glUseProgram(0);
	}

#ifdef TEST_CALSAMPLEPOINT_CYM
	void outputFile_CYMSamplePoint_MyResult() {
		std::ostringstream oss;
		oss << CutScheme::cut_space;
		std::string cutspaceStr(oss.str());
		
		std::ostringstream os;
		os << CutScheme::smoothSharpAngle;
		std::string smoothSharpLimitStr(os.str());

		std::ofstream fs;
		fs.open("GC采样点结果几何坐标_"+ cutspaceStr+"_"+ smoothSharpLimitStr+".txt");
		if (fs) {
			fs << samplePointNums << std::endl;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleControlPoints_positionSSbo); //triangleControlPoints_normalSSbo
			glm::vec4* posOut = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			for (int i = 0; i < samplePointNums; i++) {
				CYMSamplePoint tempPoint = cymResult.originSamplePoints[i];
				int triangleIndex = tempPoint.originTriangleIndex;
				glm::vec3 temp_controlPoints[10];				

				for (int k = 0; k < 10; k++)
					temp_controlPoints[k] = glm::vec3(posOut[triangleIndex * 10 + k]);

				//计算“细化后”采样点的计算值
				float up = tempPoint.centroidCoordinates.x;
				float vp = tempPoint.centroidCoordinates.y;
				float wp = tempPoint.centroidCoordinates.z;

				glm::vec3 resultPos = temp_controlPoints[0] * up*up*up
					+ temp_controlPoints[1] * 3.0f*up*up*vp + temp_controlPoints[2] * 3.0f*up*up*wp
					+ temp_controlPoints[3] * 3.0f*up*vp*vp + temp_controlPoints[4] * 6.0f*up*vp*wp + temp_controlPoints[5] * 3.0f*up*wp*wp
					+ temp_controlPoints[6] * vp*vp*vp + temp_controlPoints[7] * 3.0f*vp*vp*wp + temp_controlPoints[8] * 3.0f*vp*wp*wp + temp_controlPoints[9] * wp*wp*wp;

				fs << resultPos.x << " " << resultPos.y << " " << resultPos.z << std::endl;
				if(i%3==2)
					fs<<std::endl;
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		fs.close();

		fs.open("GC采样点结果法向_" + cutspaceStr + "_" + smoothSharpLimitStr + ".txt");
		if (fs) {
			fs << samplePointNums << std::endl;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleControlPoints_normalSSbo); 
			glm::vec4* normalOut = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			for (int i = 0; i < samplePointNums; i++) {
				CYMSamplePoint tempPoint = cymResult.originSamplePoints[i];
				int triangleIndex = tempPoint.originTriangleIndex;
				glm::vec3 temp_controlPoints[10];

				for (int k = 0; k < 10; k++)
					temp_controlPoints[k] = glm::vec3(normalOut[triangleIndex * 10 + k]);

				//计算“细化后”采样点的计算值
				float up = tempPoint.centroidCoordinates.x;
				float vp = tempPoint.centroidCoordinates.y;
				float wp = tempPoint.centroidCoordinates.z;

				glm::vec3 resultPos = temp_controlPoints[0] * up*up*up
					+ temp_controlPoints[1] * 3.0f*up*up*vp + temp_controlPoints[2] * 3.0f*up*up*wp
					+ temp_controlPoints[3] * 3.0f*up*vp*vp + temp_controlPoints[4] * 6.0f*up*vp*wp + temp_controlPoints[5] * 3.0f*up*wp*wp
					+ temp_controlPoints[6] * vp*vp*vp + temp_controlPoints[7] * 3.0f*vp*vp*wp + temp_controlPoints[8] * 3.0f*vp*wp*wp + temp_controlPoints[9] * wp*wp*wp;

				if (resultPos == glm::vec3())
					resultPos = glm::vec3(-1, 0, 0);						
			
				fs << resultPos.x << " " << resultPos.y << " " << resultPos.z << std::endl;
				if (i % 3 == 2)
					fs << std::endl;
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		fs.close();
	}
#endif 


	void showComputeResult() {
		/*
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertex_CP_SSBOID);
		Vertex* pOut = (Vertex*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, vertData.size() * sizeof(Vertex), GL_MAP_READ_BIT);
		for (GLuint i = 0; i < 64; ++i)
		{
			std::cout << "位置："<<pOut[i].position.x<<","<< pOut[i].position.y << "," << pOut[i].position.z << "," << pOut[i].position.w<<std::endl;
			std::cout << "tex：" << pOut[i].texCoords.x << "," << pOut[i].texCoords.y << "," << pOut[i].texCoords.z << "," << pOut[i].texCoords.w << std::endl;
			std::cout << "normal：" << pOut[i].normal.x << "," << pOut[i].normal.y << "," << pOut[i].normal.z << "," << pOut[i].normal.w << std::endl;
			std::cout <<std::endl;
		}
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;
		for (GLuint i = 64; i < 128; ++i)
		{
			std::cout << "位置：" << pOut[i].position.x << "," << pOut[i].position.y << "," << pOut[i].position.z << "," << pOut[i].position.w << std::endl;
			std::cout << "tex：" << pOut[i].texCoords.x << "," << pOut[i].texCoords.y << "," << pOut[i].texCoords.z << "," << pOut[i].texCoords.w << std::endl;
			std::cout << "normal：" << pOut[i].normal.x << "," << pOut[i].normal.y << "," << pOut[i].normal.z << "," << pOut[i].normal.w << std::endl;
			std::cout << std::endl;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		*/
		/*
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertex_Normal_SSBOID);
		glm::vec4* normalOut = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, indices.size()/3*64 * sizeof(glm::vec4), GL_MAP_READ_BIT);
		for (GLuint i = 0; i < 64; ++i)
		{
			std::cout << "normal：" << normalOut[i].x << "," << normalOut[i].y << "," << normalOut[i].z << "," << normalOut[i].w << std::endl;
			std::cout << std::endl;
		}
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;
		for (GLuint i = 64; i < 128; ++i)
		{
			std::cout << "normal：" << normalOut[i].x << "," << normalOut[i].y << "," << normalOut[i].z << "," << normalOut[i].w << std::endl;
			std::cout << std::endl;
		}
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;
		*/
		/*
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertex_bsplineBase_SSBOID);
		GLfloat* fOut = (GLfloat*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 27 * sizeof(GLfloat)*64, GL_MAP_READ_BIT);
		int tempOffset = 0 * 27;
		float x[5];
		float y[5];
		float z[5];
		for (int i = 0; i < 4; i++)
			x[i] = fOut[tempOffset+i+5];
		for (int i = 0; i < 4; i++)
			y[i] = fOut[tempOffset+i+14];
		for (int i = 0; i < 4; i++)
			z[i] = fOut[tempOffset+i+23];

		float total = 0;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				for (int k = 0; k < 4; k++) {
					total += x[i] * y[j] * z[k];
				}
			}
		}
		
		std::cout << "总和：" << total<< std::endl;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		*/

		/*
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleControlPoints_normalSSbo);
		glm::vec4* cOut = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 20 * sizeof(glm::vec4), GL_MAP_READ_BIT);
		for (GLuint i = 0; i < 10; ++i)
		{
			std::cout << "控制顶点：" << cOut[i].x << "," << cOut[i].y << "," << cOut[i].z << "," << cOut[i].w << std::endl;
		}
		std::cout << std::endl;
		std::cout << std::endl;
		for (GLuint i = 10; i < 20; ++i)
		{
			std::cout << "控制顶点：" << cOut[i].x << "," << cOut[i].y << "," << cOut[i].z << "," << cOut[i].w << std::endl;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		*/
		
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, OutputBuffer_VertexSSBOID);
		Vertex* pOut = (Vertex*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
		//Vertex* pOut = (Vertex*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, indices.size() * sizeof(Vertex), GL_MAP_READ_BIT);
		for (int workGroupId = 0; workGroupId < indices.size() / 3; workGroupId++)
		{
			for (int k = 0; k < 21; k++) {
				int i = workGroupId * 21 + k;
				if (pOut[i].texCoords.x > 1.0f || pOut[i].texCoords.y > 1.0f || pOut[i].texCoords.x < 0.0f || pOut[i].texCoords.y < 0.0f) {
					std::cout << "异常纹理点:" << i << std::endl;
					std::cout << "tex：" << pOut[i].texCoords.x << "," << pOut[i].texCoords.y << "," << pOut[i].texCoords.z << "," << pOut[i].texCoords.w << std::endl;
					std::cout << std::endl;
				}

				
			}
			
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		std::cout << std::endl;
		std::cout << std::endl;
		/*
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, OutputBuffer_IndicesSSBOID);
		GLuint* iout = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, indices.size() * sizeof(GLuint), GL_MAP_READ_BIT);
		for (GLuint j = 0; j < 12; ++j)	
		{
			std::cout << "索引：" << iout[j*3] << "," << iout[j*3+1] << "," << iout[j*3+2] << std::endl;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		*/
	}
	void doPrecompute_perTriangle(const Shader& computeShader)  {//对每个三角片进行计算
		glUseProgram(0);
		int triangle_nums = indices.size() / 3;
		int groupnums_z = 1;
		if (triangle_nums > 400)
			groupnums_z = 10;
		int groupnums_xy=ceil(sqrt(triangle_nums / groupnums_z));
		computeShader.use();
		glDispatchCompute(groupnums_xy, groupnums_xy, groupnums_z);
		glUseProgram(0);
	}
	void doPrecompute_perPoint(const Shader& computeShader) {//对每个点进行计算，点指的是经过重心坐标插值后的三角面片数*64个点
		glUseProgram(0);
		int point_nums = indices.size() / 3*64;
		int groupnums_z = 10;
		int groupnums_xy = ceil(sqrt(point_nums / groupnums_z));
		computeShader.use();
		glDispatchCompute(groupnums_xy, groupnums_xy, groupnums_z);//B样条体位置预计算
		glUseProgram(0);
	}
	//将变形前的结果作为绘制数据进行缓存设定
	void setupDrawData_raw() {
		glGenVertexArrays(1, &this->VAOId);
		glGenBuffers(1, &this->EBOId);
		glGenBuffers(1, &this->VBOId);
		glBindVertexArray(this->VAOId);


		glBindBuffer(GL_ARRAY_BUFFER, VBOId);
	    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)* vertData.size(),&vertData[0], GL_STATIC_DRAW);
		
		
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



	//将绘制所需的数据绑定好（计算在之前步骤中做）
	void setupDrawData()
	{
		
		//triangleQualityTest();	     

		
		glGenVertexArrays(1, &this->VAOId);
		glGenBuffers(1, &this->EBOId);
		glBindVertexArray(this->VAOId);

		glBindBuffer(GL_ARRAY_BUFFER, this->OutputBuffer_VertexSSBOID);
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
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->OutputBuffer_IndicesSSBOID);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		
		
	}
#ifdef TEST_CALSAMPLEPOINT_CYM
	CYMResult cymResult;
#endif
private:
	std::vector<Vertex> vertData;//处理后的顶点及索引信息，近包含位置信息（无纹理和法向）
	std::vector<GLuint> indices;
	std::vector<Vertex> origin_vertData;//从obj中读取的顶点及索引信息，包含位置纹理法向
	std::vector<GLuint> origin_indices;

#ifdef TEST_CALSAMPLEPOINT_CYM	
	int samplePointNums;
#endif
	std::vector<GLuint> indices_tessellation;
	std::vector<Texture> textures;
	//std::map<IndiceCouple, int> length_map;
	std::map<IndiceCouple, Edge_SmoothSharp> edge_map;
	std::map<IndiceCouple_Ordered, glm::vec3> normal_map;////有序的点对，用于查询点的法向，第一个值为起始点（即目标点），第二个值为边的另一个端点
	std::map<int, glm::vec3> special_twoSharpVertexMap;//特殊的点_法向表，即有两条尖锐边对应两个尖锐区间的点，值为crossvec
	std::map<int, std::set<int>> vertexNear_map;//点的领接表
	std::map<Vertex, GLuint> temp_positionMap;//根据点的位置建立的顶点_下标表（即点仅根据位置区分）
	std::vector<float> angles_perTrianle;//每个三角形的角度，与indices相对应，三个一组紧密排列

#ifdef useModelNormal
	std::map<Vertex_Position, glm::vec3> position_normalMap;//几何位置对应法向的表，适用于精确法向唯一模型
#endif // useModelNormal


	std::vector<glm::vec3> faceNormals;

	//std::set<Vertex> vertSet;
	//质量测试用：
	int quality_nums[(test_end - test_start)*test_space + 1];
	float quality_proportion[(test_end - test_start)*test_space + 1];
	

	//新的样式计算与套用法
	void UseCutSchemeNew() {
		std::map<Vertex, int> vertex_map = std::map<Vertex, int>();//保证顶点的独特性，值为顶点的下标
		std::vector<Vertex> generated_vertData = std::vector<Vertex>();
		std::vector<GLuint> generated_indices = std::vector<GLuint>();

#ifdef TEST_CALSAMPLEPOINT_CYM		
		cymResult = CYMResult();
		cymResult.loadFile_OriginSamplePoint();
		//samplePointList_CYM = new CYMSamplePoint[cymResult.originSamplePoints.size()]();
		samplePointNums = cymResult.originSamplePoints.size();
		//当前判断所处于的小三角形（即剖分后的三角形）下标
		int CYMSamplePoint_smallTriangleIndex = 0;
		//每轮判断开始时所处于的小三角形（即剖分后的三角形）下标
		int CYMSamplePoint_smallTriangleIndex_Loopbegin = 0;
#endif

		/*
		//所有向量单位化
		for (int i = 0; i < vertData.size(); i++) {
			glm::vec3 tempV = glm::vec3(vertData[i].normal.x, vertData[i].normal.y, vertData[i].normal.z);
			vertData[i].normal = glm::vec4(glm::normalize(tempV), 0.0);
		}
        */

		for (int triangle_index = 0; triangle_index < origin_indices.size() / 3; triangle_index++) {
			

			int length[3];
			int index[3] = { origin_indices[triangle_index * 3] ,origin_indices[triangle_index * 3 + 1],origin_indices[triangle_index * 3 + 2] };
			int pos_index[3];//根据点的位置确定的下标
			for (int i = 0; i < 3; i++) {
				Vertex tempV = origin_vertData[index[i]];
				pos_index[i] = this->temp_positionMap[Vertex(tempV.position, glm::vec4(), glm::vec4())];
			}
			Vertex v[3] = { origin_vertData[index[0]],origin_vertData[index[1]] ,origin_vertData[index[2]] };



			//开始计算该三角形的pn三角形
			//先只算法向场，几何直接重心坐标插值
			glm::vec3 originNormal[3];
			glm::vec3 generatedNormal[3];
			glm::vec3 originPos[3];
			for (int i = 0; i < 3; i++) {
				//originNormal[i] = glm::vec3(v[i].normal.x, v[i].normal.y, v[i].normal.z);
				originPos[i]= glm::vec3(v[i].position.x, v[i].position.y, v[i].position.z);
			}				
			//glm::vec4 generatedNormal[3];//生成的新三点法向
										 //计算新三点以及插入点的法向
			for (int j = 0; j < 3; j++) {
				IndiceCouple edge_temp1 = IndiceCouple(pos_index[j], pos_index[(j + 1) % 3]);
				IndiceCouple edge_temp2 = IndiceCouple(pos_index[j], pos_index[(j + 2) % 3]);
				Edge_SmoothSharp es1 = edge_map[edge_temp1];
				Edge_SmoothSharp es2 = edge_map[edge_temp2];

				if (es1.is_smooth) {					
					generatedNormal[j] = normal_map[IndiceCouple_Ordered(pos_index[j], pos_index[(j + 1) % 3])];
				}
				else {
					if (es2.is_smooth) {
						//generatedNormal[j] = (es2.triangleNormal[0] + es2.triangleNormal[1]) / 2.0f;
						//generatedNormal[j] = glm::vec3(v[j].normal);
						generatedNormal[j] = normal_map[IndiceCouple_Ordered(pos_index[j], pos_index[(j + 2) % 3])];
					}
					else//两边均尖锐
						generatedNormal[j] = faceNormals[triangle_index];
				}
				
			}


			IndiceCouple edge_12 = IndiceCouple(pos_index[0], pos_index[1]);
			IndiceCouple edge_23 = IndiceCouple(pos_index[1], pos_index[2]);
			IndiceCouple edge_31 = IndiceCouple(pos_index[0], pos_index[2]);
			

			glm::vec3 P21 = (originPos[1] - originPos[0]);								
			glm::vec3 P32 = (originPos[2] - originPos[1]);
			glm::vec3 P13 = (originPos[0] - originPos[2]);
			

			/*
			float v12 = 2.0f * glm::dot(P21, generatedNormal[0] + generatedNormal[1])/glm::dot(P21, P21);
			float v23 = 2.0f * glm::dot(P32, generatedNormal[1] + generatedNormal[2]) / glm::dot(P32, P32);
			float v31 = 2.0f *glm::dot(P13, generatedNormal[0] + generatedNormal[2]) / glm::dot(P13, P13);
			glm::vec3 h110 = generatedNormal[0] + generatedNormal[1] - v12*P21;
			glm::vec3 h011 = generatedNormal[1] + generatedNormal[2] - v23*P32;
			glm::vec3 h101 = generatedNormal[2] + generatedNormal[0] - v31*P13;
			glm::vec4 n110 = glm::vec4(glm::normalize(h110),0.0f);
			glm::vec4 n011 = glm::vec4(glm::normalize(h011), 0.0f);
			glm::vec4 n101 = glm::vec4(glm::normalize(h101), 0.0f);
			*/


			for (int i = 0; i < 3; i++)
				originNormal[i] = generatedNormal[i];
			//尝试直接进行几何调整（remesh）
			float w12 = glm::dot(P21, originNormal[0]);
			float w21= glm::dot(originPos[0] - originPos[1], originNormal[1]);
			float w23 = glm::dot(P32, originNormal[1]);
			float w32 = glm::dot(originPos[1] - originPos[2], originNormal[2]);
			float w31 = glm::dot(P13, originNormal[2]);
			float w13 = glm::dot(originPos[2] - originPos[0], originNormal[0]);
			glm::vec4 b210, b120, b021, b012, b102, b201;
			if (edge_map[edge_12].is_smooth) {
				b210 = glm::vec4((2.0f * originPos[0] + originPos[1] - w12*originNormal[0]) / 3.0f, 0.0f);
				b120 = glm::vec4((2.0f * originPos[1] + originPos[0] - w21*originNormal[1]) / 3.0f, 0.0f);
				/*
				glm::vec3 adNormal = (edge_map[edge_12].triangleNormal[0] + edge_map[edge_12].triangleNormal[1]) / 2.0f;
				b210 = glm::vec4((2.0f * originPos[0] + originPos[1] - w12*adNormal) / 3.0f, 0.0f);
				b120 = glm::vec4((2.0f * originPos[1] + originPos[0] - w21*adNormal) / 3.0f, 0.0f);
				*/
			}			     
			else {
				if (edge_map[edge_12].is_boundary) {
					if (special_twoSharpVertexMap.count(pos_index[0]) != 0) {
						glm::vec3 v = special_twoSharpVertexMap[pos_index[0]];
						b210 = glm::vec4(originPos[0] + v*(glm::dot(v, (2.0f * originPos[0] + originPos[1]) / 3.0f - originPos[0]) / glm::dot(v, v)), 0.0f);
					}
					else
						b210 = glm::vec4((2.0f * originPos[0] + originPos[1]) / 3.0f, 0.0f);

					if (special_twoSharpVertexMap.count(pos_index[1]) != 0) {
						glm::vec3 v = special_twoSharpVertexMap[pos_index[1]];
						b120 = glm::vec4(originPos[1] + v*(glm::dot(v, (2.0f * originPos[1] + originPos[0]) / 3.0f - originPos[1]) / glm::dot(v, v)), 0.0f);
					}
					else
						b120 = glm::vec4((2.0f * originPos[1] + originPos[0]) / 3.0f, 0.0f);
				}
				else {
					glm::vec3 v = edge_map[edge_12].crossVec;
					if (special_twoSharpVertexMap.count(pos_index[0]) != 0)
						v = special_twoSharpVertexMap[pos_index[0]];
					b210 = glm::vec4(originPos[0] + v*(glm::dot(v, (2.0f * originPos[0] + originPos[1]) / 3.0f - originPos[0]) / glm::dot(v, v)), 0.0f);

					if (special_twoSharpVertexMap.count(pos_index[1]) != 0)
						v = special_twoSharpVertexMap[pos_index[1]];
					else
						v = edge_map[edge_12].crossVec;
					b120 = glm::vec4(originPos[1] + v*(glm::dot(v, (2.0f * originPos[1] + originPos[0]) / 3.0f - originPos[1]) / glm::dot(v, v)), 0.0f);
				}									
			}

			if (edge_map[edge_23].is_smooth) {
				b021 = glm::vec4((2.0f * originPos[1] + originPos[2] - w23*originNormal[1]) / 3.0f, 0.0f);
				b012 = glm::vec4((2.0f * originPos[2] + originPos[1] - w32*originNormal[2]) / 3.0f, 0.0f);
			}
			else {
				if (edge_map[edge_23].is_boundary) {
					if (special_twoSharpVertexMap.count(pos_index[1]) != 0) {
						glm::vec3 v = special_twoSharpVertexMap[pos_index[1]];
						b021 = glm::vec4(originPos[1] + v*(glm::dot(v, (2.0f * originPos[1] + originPos[2]) / 3.0f - originPos[1]) / glm::dot(v, v)), 0.0f);
					}
					else
						b021 = glm::vec4((2.0f * originPos[1] + originPos[2]) / 3.0f, 0.0f);

					if (special_twoSharpVertexMap.count(pos_index[2]) != 0) {
						glm::vec3 v = special_twoSharpVertexMap[pos_index[2]];
						b012 = glm::vec4(originPos[2] + v*(glm::dot(v, (2.0f * originPos[2] + originPos[1]) / 3.0f - originPos[2]) / glm::dot(v, v)), 0.0f);
					}
					else
						b012 = glm::vec4((2.0f * originPos[2] + originPos[1]) / 3.0f, 0.0f);
				}
				else {
					glm::vec3 v = edge_map[edge_23].crossVec;
					if (special_twoSharpVertexMap.count(pos_index[1]) != 0)
						v = special_twoSharpVertexMap[pos_index[1]];
					b021 = glm::vec4(originPos[1] + v*(glm::dot(v, (2.0f * originPos[1] + originPos[2]) / 3.0f - originPos[1]) / glm::dot(v, v)), 0.0f);

					if (special_twoSharpVertexMap.count(pos_index[2]) != 0)
						v = special_twoSharpVertexMap[pos_index[2]];
					else
						v = edge_map[edge_23].crossVec;
					b012 = glm::vec4(originPos[2] + v*(glm::dot(v, (2.0f * originPos[2] + originPos[1]) / 3.0f - originPos[2]) / glm::dot(v, v)), 0.0f);
				}			
			}

			if (edge_map[edge_31].is_smooth) {
				b102 = glm::vec4((2.0f * originPos[2] + originPos[0] - w31*originNormal[2]) / 3.0f, 0.0f);
				b201 = glm::vec4((2.0f * originPos[0] + originPos[2] - w13*originNormal[0]) / 3.0f, 0.0f);				
			}
			else {
				if (edge_map[edge_31].is_boundary) {
					if (special_twoSharpVertexMap.count(pos_index[2]) != 0) {
						glm::vec3 v = special_twoSharpVertexMap[pos_index[2]];
						b102 = glm::vec4(originPos[2] + v*(glm::dot(v, (2.0f * originPos[2] + originPos[0]) / 3.0f - originPos[2]) / glm::dot(v, v)), 0.0f);
					}
					else
						b102 = glm::vec4((2.0f * originPos[2] + originPos[0]) / 3.0f, 0.0f);

					if (special_twoSharpVertexMap.count(pos_index[0]) != 0) {
						glm::vec3 v = special_twoSharpVertexMap[pos_index[0]];
						b201 = glm::vec4(originPos[0] + v*(glm::dot(v, (2.0f * originPos[0] + originPos[2]) / 3.0f - originPos[0]) / glm::dot(v, v)), 0.0f);
					}
					else
						b201 = glm::vec4((2.0f * originPos[0] + originPos[2]) / 3.0f, 0.0f);
				}
				else {
					glm::vec3 v = edge_map[edge_31].crossVec;
					if (special_twoSharpVertexMap.count(pos_index[2]) != 0)
						v = special_twoSharpVertexMap[pos_index[2]];
					b102 = glm::vec4(originPos[2] + v*(glm::dot(v, (2.0f * originPos[2] + originPos[0]) / 3.0f - originPos[2]) / glm::dot(v, v)), 0.0f);

					if (special_twoSharpVertexMap.count(pos_index[0]) != 0)
						v = special_twoSharpVertexMap[pos_index[0]];
					else
						v = edge_map[edge_31].crossVec;
					b201 = glm::vec4(originPos[0] + v*(glm::dot(v, (2.0f * originPos[0] + originPos[2]) / 3.0f - originPos[0]) / glm::dot(v, v)), 0.0f);
				}			
			}
			
			glm::vec4 Eb = (b210+ b120+b021+b012+b201+b102) / 6.0f;
			glm::vec4 b111 = Eb + (Eb-(v[0].position+ v[1].position+ v[2].position)/3.0f) / 2.0f;
			//bool calculated[3]={ false,false,false };
			//Edge edge[3];
			//float temp1, temp2, temp3;
			//int already_calculated_nums = 0;
			//std::map<IndiceCouple, int>::iterator iter;

			/*
			for (int i = 0; i < 3; i++) {
				iter = length_map.find(IndiceCouple(index[i], index[(i+1)%3]));
				if (iter != length_map.end()) {
					edge[i] = Edge(i, iter->second);
					calculated[i] = true;
					already_calculated_nums++;
				}
				else {
					temp1 = v[i].position.x - v[(i + 1) % 3].position.x;
					temp2 = v[i].position.y - v[(i + 1) % 3].position.y;
					temp3 = v[i].position.z - v[(i + 1) % 3].position.z;
					float length = sqrt(temp1*temp1 + temp2*temp2 + temp3*temp3);
					length = length / CutScheme::cut_space;
					length = (length < 1 ? 1 : round(length));
					edge[i] = Edge(i, length);
				}
			}
			
			 if (already_calculated_nums != 3) {//有需要计算的边
				std::stable_sort(edge, edge+3);//非降序
				if (edge[2].length > edge[0].length + edge[1].length) {//四舍五入异常情况，额外处理
					if (!calculated[edge[2].index]) {//最大边是新计算值
						edge[2].length = edge[0].length + edge[1].length;
						for (int i = 0; i < 3; i++)
							length[edge[i].index] = edge[i].length;
						for (int i = 0; i < 3; i++) 
							if (calculated[i] == false) {
								calculated[i] = true;
								length_map[IndiceCouple(index[i], index[(i + 1) % 3])] = length[i];
							}						
					}
					else {//非最大边是新计算值
						     while (edge[2].length > edge[0].length + edge[1].length) {
								      if(!calculated[edge[1].index])
							                    edge[1].length++;
							          if(edge[2].length > edge[0].length + edge[1].length&& !calculated[edge[0].index])
								                edge[0].length++;
						      }
							 for (int i = 0; i < 3; i++)
								 length[edge[i].index] = edge[i].length;
							 for (int i = 0; i < 3; i++)
								 if (calculated[i] == false) {
									 calculated[i] = true;
									 length_map[IndiceCouple(index[i], index[(i + 1) % 3])] = length[i];
								 }
					}
				}
				else {//非异常情况
					for (int i = 0; i < 3; i++)
						length[edge[i].index] = edge[i].length;
					for (int i = 0; i < 3; i++)
						if (calculated[i] == false) {
							calculated[i] = true;
							length_map[IndiceCouple(index[i], index[(i + 1) % 3])] = length[i];
						}
				}
			}
			 else {//三边均已计算完毕
				 //std::cout << "三边均已计算：" ;
				 for (int i = 0; i < 3; i++) {
					 length[edge[i].index] = edge[i].length;
					 //std::cout << edge[i].length << ",";
				 }
				// std::cout << std::endl;
			 }
			 */
			
			 for (int i = 0; i < 3; i++) {
				 float temp1 = v[i].position.x - v[(i + 1) % 3].position.x;
				 float temp2 = v[i].position.y - v[(i + 1) % 3].position.y;
				 float temp3 = v[i].position.z - v[(i + 1) % 3].position.z;
				 float l = sqrt(temp1*temp1 + temp2*temp2 + temp3*temp3);
				 l = l / CutScheme::cut_space;
				 length[i] = (l < 1 ? 1 : round(l));
			 }
			
			

			 int length_ab = (length[0] > CutScheme::MAX_CUTLENGTH ? CutScheme::MAX_CUTLENGTH : length[0]);
			 int length_bc = (length[1] > CutScheme::MAX_CUTLENGTH ? CutScheme::MAX_CUTLENGTH : length[1]);
			 int length_ca = (length[2] > CutScheme::MAX_CUTLENGTH ? CutScheme::MAX_CUTLENGTH : length[2]);
			//std::cout << "当前方案：" << length_ab << "*" << length_bc << "*" << length_ca << std::endl;
			//当前属于第几号切割方案
			 int scheme_index = CutScheme::scheme_index[length_ab - 1][length_bc - 1][length_ca - 1];
			 DelaunayResult currentScheme= CutScheme::cutScheme[scheme_index];

#ifdef TEST_CALSAMPLEPOINT_CYM
			 //计算当前大三角片中的所有CYM采样点在哪个子三角形中
			 //CYMResult cymResult = CYMResult();
			 //cymResult.loadFile_OriginSamplePoint();
			 //CYMSamplePoint* samplePointList_CYM = new CYMSamplePoint[cymResult.originSamplePoints.size()];
			 
			 
			 if (cymResult.originSamplePointsMap.count(triangle_index) != 0) {
				 
				 std::vector<CYMSamplePoint> tempSamplePointVec = cymResult.originSamplePointsMap[triangle_index];
				 if (triangle_index == 343) {
					 int t = 0;
				 }
				 for (CYMSamplePoint& samplePoint : tempSamplePointVec) {
					 CYMSamplePoint_smallTriangleIndex = CYMSamplePoint_smallTriangleIndex_Loopbegin;
					 int temp_index = 0;
					 for (int i = 0; i < currentScheme.vector_indices.size() / 3; i++) {
						 glm::vec3 pointA_centroid = currentScheme.vector_positions[currentScheme.vector_indices[temp_index]];
						 temp_index++;
						 glm::vec3 pointB_centroid = currentScheme.vector_positions[currentScheme.vector_indices[temp_index]];
						 temp_index++;
						 glm::vec3 pointC_centroid = currentScheme.vector_positions[currentScheme.vector_indices[temp_index]];
						 temp_index++;
						 glm::vec3 pointA = originPos[0] * pointA_centroid.x + originPos[1] * pointA_centroid.y + originPos[2] * pointA_centroid.z;
						 glm::vec3 pointB = originPos[0] * pointB_centroid.x + originPos[1] * pointB_centroid.y + originPos[2] * pointB_centroid.z;
						 glm::vec3 pointC = originPos[0] * pointC_centroid.x + originPos[1] * pointC_centroid.y + originPos[2] * pointC_centroid.z;

						 if (Point_Is_In_Triangle(pointA_centroid, pointB_centroid, pointC_centroid, samplePoint.centroidCoordinates)) {
							 CYMSamplePoint tempSamplePoint = samplePoint;
							 tempSamplePoint.originTriangleIndex = CYMSamplePoint_smallTriangleIndex;
							 //tempSamplePoint.centroidCoordinates= samplePoint.centroidCoordinates.x*
							 glm::mat3 centroid_mat = glm::mat3(pointA_centroid, pointB_centroid, pointC_centroid);
							 tempSamplePoint.centroidCoordinates = glm::inverse(centroid_mat)*samplePoint.centroidCoordinates;
							 //samplePointList_CYM[samplePoint.samplePointsListIndex] = tempSamplePoint;
							 cymResult.originSamplePoints[samplePoint.samplePointsListIndex]= tempSamplePoint;
						 }
						 CYMSamplePoint_smallTriangleIndex++;
					 }
				 }
				 
			 }
			 CYMSamplePoint_smallTriangleIndex_Loopbegin += currentScheme.vector_indices.size() / 3;
#endif // TEST_CALSAMPLEPOINT_CYM



			 std::vector<int> local_indicesVec = std::vector<int>();//每轮循环都要清空
			 //std::vector<bool> local_indicesBoolVec = std::vector<bool>();//对应本次循环位置的顶点是否已经存在
			 int map_size = vertex_map.size();
			 int index_accumlate = 0;

			//对顶点
			for (int i = 0; i < currentScheme.vector_positions.size(); i++) {
				glm::vec3 tempPosition = currentScheme.vector_positions[i];
				//经过质心坐标调整后的新顶点
				float up = tempPosition.x;
				float vp = tempPosition.y;
				float wp = tempPosition.z;				
				//Vertex vertex_adjuested = v[0] * tempPosition.x + v[1] * tempPosition.y + v[2] * tempPosition.z;
				glm::vec4 tex_new = v[0].texCoords*tempPosition.x + v[1].texCoords*tempPosition.y + v[2].texCoords*tempPosition.z;


				//glm::vec4 pos_new = v[0].position*tempPosition.x + v[1].position*tempPosition.y + v[2].position*tempPosition.z;
				
				glm::vec4 pos_new = v[0].position*up*up*up
					               + b210*3.0f*up*up*vp + b201*3.0f*up*up*wp
					               +b120*3.0f*up*vp*vp+b111*6.0f*up*vp*wp+b102*3.0f*up*wp*wp
					               + v[1].position*vp*vp*vp+b021*3.0f*vp*vp*wp+b012*3.0f*vp*wp*wp+ v[2].position*wp*wp*wp;
				
				
				/*
				std::cout << pos_new.x <<"[]"<< pos_new.y << "[]" << pos_new.z << "[]" << pos_new.w << std::endl;
				std::cout << v[0].position.x << "[]" << v[0].position.y << "[]" << v[0].position.z << "[]" << v[0].position.w << std::endl;
				std::cout << v[1].position.x << "[]" << v[1].position.y << "[]" << v[1].position.z << "[]" << v[1].position.w << std::endl;
				std::cout << v[2].position.x << "[]" << v[2].position.y << "[]" << v[2].position.z << "[]" << v[2].position.w << std::endl;
				std::cout << std::endl;
				*/
				/*
				glm::vec4 normal_new = up*up*glm::vec4(generatedNormal[0],0) + up*vp*n110 +
					 up*wp*n101 + vp*vp*glm::vec4(generatedNormal[1], 0) +
					vp*wp*n011 + wp*wp*glm::vec4(generatedNormal[2], 0);
				*/
				glm::vec4 normal_new = glm::vec4(generatedNormal[0], 0)*tempPosition.x + glm::vec4(generatedNormal[1], 0)*tempPosition.y + glm::vec4(generatedNormal[2], 0)*tempPosition.z;
				Vertex vertex_adjuested = Vertex(pos_new, tex_new, normal_new);
				

				//Vertex vertex_adjuested=Vertex(pos_new, tex_new, glm::vec4(glm::normalize(glm::vec3(normal_new)),0));
				if (vertex_map.count(vertex_adjuested) == 0) {//该顶点未加入，则加入
					vertex_map[vertex_adjuested] = map_size + index_accumlate;
					generated_vertData.push_back(vertex_adjuested);
					local_indicesVec.push_back(map_size + index_accumlate);	
					//local_indicesBoolVec.push_back(false);
					index_accumlate++;
				}
				else {							
					local_indicesVec.push_back(vertex_map[vertex_adjuested]);
					//local_indicesBoolVec.push_back(true);
				}
			}
			map_size = vertex_map.size();
			//对下标
			for (int i = 0; i < currentScheme.vector_indices.size(); i++) {
				int temp_index = currentScheme.vector_indices[i];
				currentScheme.vector_indices[i] = local_indicesVec[temp_index];
			}
			generated_indices.insert(generated_indices.end(), currentScheme.vector_indices.begin(), currentScheme.vector_indices.end());
		}//所有原始三角形处理完毕

		vertData = generated_vertData;
		indices = generated_indices;

	}
	
	void triangleQualityTest() {
		float space = 1.0f / test_space;
		int size = vertData.size();
		int goodTriangleNums = 0;
		float result_1_upbound = 5.0f;
		for (int i = 0; i < size; i+=3) {
			glm::vec4 point_a = vertData[i].position;
			glm::vec4 point_b = vertData[i+1].position;
			glm::vec4 point_c = vertData[i+2].position;
			float length_ab = sqrt((point_a.x - point_b.x)*(point_a.x - point_b.x) + (point_a.y - point_b.y)*(point_a.y - point_b.y) + (point_a.z - point_b.z)*(point_a.z - point_b.z));
			float length_ac = sqrt((point_a.x - point_c.x)*(point_a.x - point_c.x) + (point_a.y - point_c.y)*(point_a.y - point_c.y) + (point_a.z - point_c.z)*(point_a.z - point_c.z));
			float length_bc = sqrt((point_c.x - point_b.x)*(point_c.x - point_b.x) + (point_c.y - point_b.y)*(point_c.y - point_b.y) + (point_c.z - point_b.z)*(point_c.z - point_b.z));
			float p = (length_ab + length_ac + length_bc) / 2.0f;
			float result_1 = length_ab*length_ac*length_bc / (4.0f*(p - length_ab)*(p - length_ac)*(p - length_bc));

			int index;
			if (result_1 >= test_end)
				index = (test_end - test_start)*test_space;
			else
				index = (result_1 - test_start) / space;
			 
			quality_nums[index]++;
				//if (result_1 < result_1_upbound)
				//goodTriangleNums++;
		}

		for (int i = 0; i < (test_end - test_start)*test_space + 1; i++) {
			std::cout << quality_nums[i] << std::endl;
			quality_proportion[i] = 300.0f*quality_nums[i] / size;
		}
		std::cout << std::endl;
		std::cout << std::endl;

		for (int i = 0; i < (test_end - test_start)*test_space + 1; i++) 
			std::cout << quality_proportion[i] << std::endl;  
		
		std::cout <<  std::endl;
		std::cout << std::endl;
		//std::cout << "好三角形数=" << goodTriangleNums << std::endl;
		//printf("好三角形百分比=%.2f\n", (float)goodTriangleNums / vertData.size()*300.0f); 		
	}

	void setupIndicesBuffer() {
		//计算连接索引
		//先计算顶角朝上的部分
		int tessellationTimes = CutScheme::tessellation_times;
		int tessellationPointNums = (1+ tessellationTimes)*(2+ tessellationTimes)/2;
		
		for (int workGroupId = 0; workGroupId < indices.size() / 3; workGroupId++) {
			int baseOffset = workGroupId * tessellationPointNums;
			int point_index = 0;//起始点在本地的索引
			int temp_index = 0;
			for (int i = 1; i <= tessellationTimes; i++) {
				for (int j = 0; j < i; j++) {
					indices_tessellation.push_back(baseOffset + point_index);
					indices_tessellation.push_back(baseOffset + point_index+i);
					indices_tessellation.push_back(baseOffset + point_index+i+1);
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
		setupSSBufferObjectVec<GLuint>(OutputBuffer_IndicesSSBOID, 13, indices_tessellation, false);
		/*
		for (GLuint j = 0; j < 2; ++j) {
			for (GLuint i = 0; i < 25; ++i)
			{
				std::cout << "索引：" << indices_tessellation[j * 75 + i * 3] << "," << indices_tessellation[j * 75 + i * 3 + 1] << "," << indices_tessellation[j * 75 + i * 3 + 2] << std::endl;
			}
			std::cout << std::endl;
		}
			*/
	}



	//定义每条边为尖锐边还是光滑边
	void defineEdge_SmoothSharp() {
		float smoothSharpLimit = CutScheme::smoothSharpLimit;
		//const float smoothSharpLimit = 0.75f;
		//edge_map
		for (int triangle_index = 0; triangle_index < indices.size() / 3; triangle_index++) {
			int index[3] = { indices[triangle_index * 3] ,indices[triangle_index * 3 + 1],indices[triangle_index * 3 + 2] };
			glm::vec3 originPos[3];
			for (int i = 0; i < 3; i++) 
				originPos[i] = glm::vec3(vertData[index[i]].position);
			glm::vec3 faceNormal = glm::normalize(glm::cross(originPos[1] - originPos[0], originPos[2] - originPos[1]));
			faceNormals.push_back(faceNormal);
			IndiceCouple edge_1 = IndiceCouple(index[0], index[1]);
			IndiceCouple edge_2 = IndiceCouple(index[1], index[2]);
			IndiceCouple edge_3 = IndiceCouple(index[2], index[0]);			
	       

			if (edge_map.count(edge_1) == 0) {
				Edge_SmoothSharp ss = Edge_SmoothSharp();
				ss.triangleIndex[0] = triangle_index;
				ss.triangleNormal[0] = faceNormal;
				ss.vertexIndex[0] = index[2];
				ss.is_boundary = true;
				ss.is_smooth = false;
				edge_map[edge_1] = ss;
			}
			else {
				edge_map[edge_1].is_boundary = false;
				edge_map[edge_1].triangleIndex[1]= triangle_index;
				edge_map[edge_1].triangleNormal[1] = faceNormal;
				edge_map[edge_1].vertexIndex[1] = index[2];
				if (glm::dot(edge_map[edge_1].triangleNormal[0], faceNormal) > smoothSharpLimit)//阈值
					edge_map[edge_1].is_smooth = true;
				else {
					edge_map[edge_1].crossVec = glm::cross(edge_map[edge_1].triangleNormal[0], faceNormal);
				}					
			}
				
			if (edge_map.count(edge_2) == 0) {
				Edge_SmoothSharp ss = Edge_SmoothSharp();
				ss.triangleIndex[0] = triangle_index;
				ss.triangleNormal[0] = faceNormal;
				ss.vertexIndex[0] = index[0];
				ss.is_boundary = true;
				ss.is_smooth = false;
				edge_map[edge_2] = ss;
			}
			else {
				edge_map[edge_2].is_boundary = false;
				edge_map[edge_2].triangleIndex[1] = triangle_index;
				edge_map[edge_2].triangleNormal[1] = faceNormal;
				edge_map[edge_2].vertexIndex[1] = index[0];
				if (glm::dot(edge_map[edge_2].triangleNormal[0], faceNormal) > smoothSharpLimit)//阈值
					edge_map[edge_2].is_smooth = true;
				else {
					edge_map[edge_2].crossVec = glm::cross(edge_map[edge_2].triangleNormal[0], faceNormal);
				}
			}

			if (edge_map.count(edge_3) == 0) {
				Edge_SmoothSharp ss = Edge_SmoothSharp();
				ss.triangleIndex[0] = triangle_index;
				ss.triangleNormal[0] = faceNormal;
				ss.vertexIndex[0] = index[1];
				ss.is_boundary = true;
				ss.is_smooth = false;
				edge_map[edge_3] = ss;
			}
			else {
				edge_map[edge_3].is_boundary = false;
				edge_map[edge_3].triangleIndex[1] = triangle_index;
				edge_map[edge_3].triangleNormal[1] = faceNormal;
				edge_map[edge_3].vertexIndex[1] = index[1];
				if (glm::dot(edge_map[edge_3].triangleNormal[0], faceNormal) > smoothSharpLimit)//阈值
					edge_map[edge_3].is_smooth = true;
				else {
					edge_map[edge_3].crossVec = glm::cross(edge_map[edge_3].triangleNormal[0], faceNormal);
				}
			}
		}
		
	}
	//建立领接表和三角片角度表
	void defineNearMapsAndAngles() {
		for (int triangle_index = 0; triangle_index < indices.size() / 3; triangle_index++) {
			int index[3] = { indices[triangle_index * 3] ,indices[triangle_index * 3 + 1],indices[triangle_index * 3 + 2] };
			for (int i = 0; i < 3; i++) {
				if (vertexNear_map.count(index[i]) == 0) {
					std::set<int> se = std::set<int>();
					se.insert(index[(i + 1) % 3]);
					se.insert(index[(i + 2) % 3]);
					vertexNear_map[index[i]] = se;
				}
				else {
					vertexNear_map[index[i]].insert(index[(i + 1) % 3]);
					vertexNear_map[index[i]].insert(index[(i + 2) % 3]);
				}
			}//建立点领接表		

			//建立角度表
			glm::vec3 originPos[3];
			for (int i = 0; i < 3; i++)
				originPos[i] = glm::vec3(vertData[index[i]].position);
			//计算三个角度大小
			float length[3];
			float cos[3];
			for (int i = 0; i < 3; i++) {
				float temp1 = originPos[i].x - originPos[(i + 1) % 3].x;
				float temp2 = originPos[i].y - originPos[(i + 1) % 3].y;
				float temp3 = originPos[i].z - originPos[(i + 1) % 3].z;
				length[i] = sqrt(temp1*temp1 + temp2*temp2 + temp3*temp3);
			}
			cos[0] = (length[0] * length[0] + length[2] * length[2] - length[1] * length[1]) / (2.0f*length[0] * length[2]);
			cos[1] = (length[0] * length[0] + length[1] * length[1] - length[2] * length[2]) / (2.0f*length[0] * length[1]);
			cos[2] = (length[1] * length[1] + length[2] * length[2] - length[0] * length[0]) / (2.0f*length[1] * length[2]);
			angles_perTrianle.push_back(acosf(cos[0]));
			angles_perTrianle.push_back(acosf(cos[1]));
			angles_perTrianle.push_back(acosf(cos[2]));
		}

		
	}
	//获取角度值
	float getAngle(int triangleIndex,int pointIndex) {
		int offset = 0;
		while (indices[triangleIndex * 3 + offset] != pointIndex)
			offset++;
		return angles_perTrianle[triangleIndex * 3 + offset];
	}
	//建立点和边的光滑区间
	void define_SmoothArea() {
		//两条尖锐边的阈值
		const float sharpEdgecrossLimit = -0.5;
		
		for (int vertex_index = 0; vertex_index < vertData.size(); vertex_index++) {
			std::set<int> NearbyPointsSet = vertexNear_map[vertex_index];

			bool isAllNoBoundary = true;
			bool isAllSmoothEdges = true;//该点的所有领接边都是光滑边
			std::set<int>::iterator iter = NearbyPointsSet.begin();
			int initialVertexIndex = (*iter);
			for (int anotherIndex : NearbyPointsSet) {				
				if (!edge_map[IndiceCouple(vertex_index, anotherIndex)].is_smooth) {
					isAllSmoothEdges = false;
					initialVertexIndex = anotherIndex;
					break;
				}
			}

			for (int anotherIndex : NearbyPointsSet) {
				if (edge_map[IndiceCouple(vertex_index, anotherIndex)].is_boundary) {
					isAllNoBoundary = false;
					initialVertexIndex = anotherIndex;
					break;
				}
			}
		

			if (isAllNoBoundary) {
				if (isAllSmoothEdges) {//所有边均为光滑边
#ifdef useModelNormal
					for (int anotherIndex : NearbyPointsSet)
						normal_map[IndiceCouple_Ordered(vertex_index, anotherIndex)] = position_normalMap[Vertex_Position(vertData[vertex_index].position)];
#else
									   //直接加权平均
					glm::vec3 totalNormal = glm::vec3(0, 0, 0);
					float totalAngles = 0;//角度和，加权用
										  //向后递归，直到遇到第一个尖锐边（包括起始边）或者回到初始点
					int lastIndex = initialVertexIndex;//上一条边的另一端点的index
					IndiceCouple tempEdge = IndiceCouple(vertex_index, initialVertexIndex);
					int currentPointIndex = initialVertexIndex;
					while (true) {
						Edge_SmoothSharp smTemp = edge_map[tempEdge];
						int sideIndex = 0;//选边的哪一侧
						if (smTemp.vertexIndex[0] == lastIndex)
							sideIndex = 1;
						float angle = getAngle(smTemp.triangleIndex[sideIndex], vertex_index);
						totalAngles += angle;
						totalNormal += angle*smTemp.triangleNormal[sideIndex];
						tempEdge = IndiceCouple(vertex_index, smTemp.vertexIndex[sideIndex]);
						lastIndex = currentPointIndex;
						currentPointIndex = smTemp.vertexIndex[sideIndex];
						if (currentPointIndex == initialVertexIndex)
							break;
					}
					glm::vec3 avarageNormal = glm::normalize(totalNormal / totalAngles);
					for (int anotherIndex : NearbyPointsSet)
						normal_map[IndiceCouple_Ordered(vertex_index, anotherIndex)] = avarageNormal;
#endif // 

									   
				}
				else {//至少有一条尖锐边	
					std::vector<glm::vec3> smoothArea_vec = std::vector<glm::vec3>();
					std::vector<glm::vec3> sharpEdges = std::vector<glm::vec3>();//对于本来夹角就很小的两条尖锐边不做平滑处理
																				 //遇到尖锐边，则以该尖锐边为起始，直到遇到下一条尖锐边(包括自己）
					int smoothAreaBeginIndex = initialVertexIndex;//一个光滑区间的起始边端点
					int lastIndex = smoothAreaBeginIndex;//上一条边的另一端点的index
					int currentPointIndex = smoothAreaBeginIndex;

					while (true) {//直到回到起始边才跳出循环
						std::vector<int> smoothAreaVertexList = std::vector<int>();//光滑区间，指在两条尖锐边所夹的区域内的点（光滑边上的另一端点）
						glm::vec3 totalNormal = glm::vec3(0, 0, 0);
						float totalAngles = 0;//角度和，加权用
											  //向后递归，直到遇到第一个尖锐边（包括起始边）或者回到初始点					
						IndiceCouple tempEdge = IndiceCouple(vertex_index, smoothAreaBeginIndex);

						while (true) {//一个光滑区间（对于两边都是尖锐边的triangle不视作光滑区间）
							Edge_SmoothSharp smTemp = edge_map[tempEdge];
							int sideIndex = 0;//选边的哪一侧
							if (smTemp.vertexIndex[0] == lastIndex)
								sideIndex = 1;
							float angle = getAngle(smTemp.triangleIndex[sideIndex], vertex_index);
							totalAngles += angle;
							totalNormal += angle*smTemp.triangleNormal[sideIndex];
							lastIndex = currentPointIndex;
							currentPointIndex = smTemp.vertexIndex[sideIndex];
							tempEdge = IndiceCouple(vertex_index, currentPointIndex);
							if (!edge_map[tempEdge].is_smooth) {//该光滑区间已到边界，遇到下一条尖锐边退出
								glm::vec3 edgeVec = glm::normalize(glm::vec3(vertData[vertex_index].position - vertData[currentPointIndex].position));
								sharpEdges.push_back(edgeVec);
								glm::vec3 avarageNormal = glm::normalize(totalNormal / totalAngles);
								smoothArea_vec.push_back(avarageNormal);
								for (int anotherIndex : smoothAreaVertexList)
									normal_map[IndiceCouple_Ordered(vertex_index, anotherIndex)] = avarageNormal;
								smoothAreaBeginIndex = currentPointIndex;
								break;
							}
							smoothAreaVertexList.push_back(currentPointIndex);
						}

						if (smoothAreaBeginIndex == initialVertexIndex)
							break;
					}
					if (smoothArea_vec.size() == 2) {
						if (glm::dot(sharpEdges[0], sharpEdges[1])<sharpEdgecrossLimit)//只对本来夹角比较大的两条尖锐边做平滑处理（即二者比较接近同一直线上）
							special_twoSharpVertexMap[vertex_index] = glm::normalize(glm::cross(smoothArea_vec[0], smoothArea_vec[1]));
					}

				}//至少有一条尖锐边
			}//全为非边界边
			else {//至少有一条边界边
				//边界边本质上属于尖锐边

				std::vector<glm::vec3> smoothArea_vec = std::vector<glm::vec3>();
				std::vector<glm::vec3> boundaryEdges = std::vector<glm::vec3>();																			 
				float BoundaryLength1, BoundaryLength2;

				int smoothAreaBeginIndex = initialVertexIndex;//一个光滑区间的起始边端点
				int lastIndex = smoothAreaBeginIndex;//上一条边的另一端点的index
				int currentPointIndex = smoothAreaBeginIndex;
				glm::vec3 temp_boundaryVec = glm::vec3(vertData[vertex_index].position - vertData[currentPointIndex].position);
				boundaryEdges.push_back(glm::normalize(temp_boundaryVec));
				BoundaryLength1 = glm::length(temp_boundaryVec);
				while (true) {//直到遇到下一条尖锐边跳出循环
					std::vector<int> smoothAreaVertexList = std::vector<int>();//光滑区间，指在两条尖锐边所夹的区域内的点（光滑边上的另一端点）
					glm::vec3 totalNormal = glm::vec3(0, 0, 0);
					float totalAngles = 0;//角度和，加权用
										  //向后递归，直到遇到第一个尖锐边（包括起始边）或者回到初始点					
					IndiceCouple tempEdge = IndiceCouple(vertex_index, smoothAreaBeginIndex);

					while (true) {//一个光滑区间（对于两边都是尖锐边的triangle不视作光滑区间）
						Edge_SmoothSharp smTemp = edge_map[tempEdge];
						int sideIndex = 0;//选边的哪一侧
						if (smTemp.vertexIndex[0] == lastIndex)
							sideIndex = 1;
						float angle = getAngle(smTemp.triangleIndex[sideIndex], vertex_index);
						totalAngles += angle;
						totalNormal += angle*smTemp.triangleNormal[sideIndex];
						lastIndex = currentPointIndex;
						currentPointIndex = smTemp.vertexIndex[sideIndex];
						tempEdge = IndiceCouple(vertex_index, currentPointIndex);
						if (!edge_map[tempEdge].is_smooth) {//该光滑区间已到边界，遇到下一条尖锐边退出
							glm::vec3 edgeVec = glm::normalize(glm::vec3(vertData[vertex_index].position - vertData[currentPointIndex].position));
							glm::vec3 avarageNormal = glm::normalize(totalNormal / totalAngles);
							smoothArea_vec.push_back(avarageNormal);
							for (int anotherIndex : smoothAreaVertexList)
#ifdef useModelNormal								
								normal_map[IndiceCouple_Ordered(vertex_index, anotherIndex)] = position_normalMap[Vertex_Position(vertData[vertex_index].position)];
#else
								normal_map[IndiceCouple_Ordered(vertex_index, anotherIndex)] = avarageNormal;
#endif							
							smoothAreaBeginIndex = currentPointIndex;
							break;
						}
						smoothAreaVertexList.push_back(currentPointIndex);
					}
					
					if (edge_map[IndiceCouple(vertex_index, smoothAreaBeginIndex)].is_boundary) {
						temp_boundaryVec = glm::vec3(vertData[vertex_index].position - vertData[currentPointIndex].position);
						boundaryEdges.push_back(glm::normalize(temp_boundaryVec));
						BoundaryLength2 = glm::length(temp_boundaryVec);

						if (glm::dot(boundaryEdges[0], boundaryEdges[1]) < sharpEdgecrossLimit) {//只对本来夹角比较大的两条边界边做平滑处理（即二者比较接近同一直线上）
							glm::vec3 avarageNormal = (BoundaryLength1*boundaryEdges[0] - BoundaryLength2*boundaryEdges[1])/(BoundaryLength1+ BoundaryLength2);
							special_twoSharpVertexMap[vertex_index] = glm::normalize(avarageNormal);
						}
						break;
					}
											
				}
				
			}


		}
	}
	/************************************************************测试用代码******************************************************/
	//beginTriangleIndex即开始的三角形下标，nums为三角形个数
	void changeTrianlgeColor(int beginTriangleIndex, int nums) {
		for (int offset = 0; offset < nums; offset++) {
			vertData[indices[beginTriangleIndex * 3 + offset * 3 ]].texCoords.z = -1;
			vertData[indices[beginTriangleIndex * 3 + offset * 3 + 1]].texCoords.z = -1;
			vertData[indices[beginTriangleIndex * 3 + offset * 3 + 2]].texCoords.z = -1;
		}
	}
	//输出三角形信息，beginTriangleIndex即开始的三角形下标，nums为三角形个数
	void printTrianlgeInfo(int beginTriangleIndex, int nums) {
		for (int offset = 0; offset < nums; offset++) {
			Vertex tempV1 = vertData[indices[beginTriangleIndex * 3 + offset * 3]];
			Vertex tempV2 = vertData[indices[beginTriangleIndex * 3 + offset * 3+1]];
			Vertex tempV3 = vertData[indices[beginTriangleIndex * 3 + offset * 3+2]];
			
			std::cout << tempV1.position.x << "," << tempV1.position.y << ","<<tempV1.position.z << std::endl;
			std::cout << tempV1.normal.x << "," << tempV1.normal.y << "," << tempV1.normal.z << std::endl;
			std::cout << std::endl;
			std::cout << tempV2.position.x << "," << tempV2.position.y << "," << tempV2.position.z << std::endl;
			std::cout << tempV2.normal.x << "," << tempV2.normal.y << "," << tempV2.normal.z << std::endl;
			std::cout << std::endl;
			std::cout << tempV3.position.x << "," << tempV3.position.y << "," << tempV3.position.z << std::endl;
			std::cout << tempV3.normal.x << "," << tempV3.normal.y << "," << tempV3.normal.z << std::endl;
			std::cout << "*******************************" << std::endl;
		}
	}


	

};

#endif 