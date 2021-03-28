#pragma once
#include "teapotUtil.h"
#include <fstream>
#include "mesh.h"
#include <map>


void Teapot::init() {
	glm::vec3 controlPoints[16];

	//std::vector<Vertex> vertexList_origin= std::vector<Vertex>();
	//std::vector<int> indicesList_origin = std::vector<int>();
	//std::map<Vertex, int> vertexMap= std::map<Vertex, int>();

	int vertexNums_perBezierPatch =(divs+1)*(divs+1);
	glm::vec4 TopNormal = glm::vec4(0, 0, 1, 0);
	glm::vec4 buttomNormal = glm::vec4(0, 0, -1, 0); 
	
	int offset_loop = 0;
	//对于每个bezier面片
	for (int np = 0; np < kTeapotNumPatches; ++np) { 
		std::vector<int> tempIndices = std::vector<int>();
		int pointNums_offset = 0;

		for (int i = 0; i < 16; ++i)
			controlPoints[i] = glm::vec3(teapotVertices[teapotPatches[np][i] - 1][0], teapotVertices[teapotPatches[np][i] - 1][1],teapotVertices[teapotPatches[np][i] - 1][2]);

		bool is_Degenerate_Pre = false;
		bool is_Degenerate_Buttom = false;
		
		if (np == 20 || np == 21 || np == 22 || np == 23)
			is_Degenerate_Pre = true;
		else if (np == 28 || np == 29 || np == 30 || np == 31)
			is_Degenerate_Buttom = true;
		//蜕化情况
		if (is_Degenerate_Pre|| is_Degenerate_Buttom) {
			
			Vec3f pos_1 = evalBezierPatch(controlPoints, 0, 0);
			if(is_Degenerate_Pre)
			   vertexList_origin.push_back(Vertex(glm::vec4(pos_1, 0), glm::vec4(), TopNormal));
			else
				vertexList_origin.push_back(Vertex(glm::vec4(pos_1, 0), glm::vec4(), buttomNormal));
			BezierSurfaceIndex.push_back(np);
			pointNums_offset++;


			//生成顶点
			for (int j = 1; j <= divs; ++j) {
				float v = j / (float)divs;
				for (int i = 0; i <= divs; ++i) {
						float u = i / (float)divs;
						Vec3f pos = evalBezierPatch(controlPoints, u, v);

						Vec3f dU = dUBezier(controlPoints, u, v);
						Vec3f dV = dVBezier(controlPoints, u, v);
						Vec3f nor;

						nor = glm::normalize(glm::cross(dU, dV));
						glm::vec4 position = glm::vec4(pos, 0);
						//这里纹理坐标用来存放uv
						glm::vec4 tex = glm::vec4(u, v, 0, 0);
						glm::vec4 normal = glm::vec4(nor, 0);
						vertexList_origin.push_back(Vertex(position, tex, normal));
						BezierSurfaceIndex.push_back(np);
						pointNums_offset++;								
				}
			}	
			//生成索引
			int offset = offset_loop;
			int divsP1 = divs + 1;
			int divsP2 = divs + 2;
			//先推入顶上的divs个三角形
			for (int i = 0; i < divs; ++i) {
				tempIndices.push_back(offset );
				tempIndices.push_back(offset + i + 2);
				tempIndices.push_back(offset + i + 1);
			}
			
			//生成顶角朝下的
			for (int i = 0; i < divs-1; ++i) {
				for (int j = 0; j < divs; ++j) {
					tempIndices.push_back(offset + i * divsP1 + j+1);
					tempIndices.push_back(offset + i * divsP1 + j + 2);
					tempIndices.push_back(offset + i * divsP1 + j + divsP2+1);
				}
			}
			//生成顶角朝上的
			for (int i = 0; i < divs-1; ++i) {
				for (int j = 0; j < divs; ++j) {
					tempIndices.push_back(offset + i * divsP1 + j+1);
					tempIndices.push_back(offset + i * divsP1 + j + divsP2+1);
					tempIndices.push_back(offset + i * divsP1 + j + divsP1+1);
				}
			}
		}
		else {
			//生成顶点
			for (int j = 0; j <= divs; ++j) {
				float v = j / (float)divs;
				for (int i = 0; i <= divs; ++i) {
					float u = i / (float)divs;
					Vec3f pos = evalBezierPatch(controlPoints, u, v);

					Vec3f dU = dUBezier(controlPoints, u, v);
					Vec3f dV = dVBezier(controlPoints, u, v);
					Vec3f nor;

					nor = glm::normalize(glm::cross(dU, dV));
					glm::vec4 position = glm::vec4(pos, 0);
					//这里纹理坐标用来存放uv
					glm::vec4 tex = glm::vec4(u, v, 0, 0);
					glm::vec4 normal = glm::vec4(nor, 0);
					vertexList_origin.push_back(Vertex(position, tex, normal));
					BezierSurfaceIndex.push_back(np);
					pointNums_offset++;
				}
			}
			//生成索引
			int offset = offset_loop;
			int divsP1 = divs + 1;
			int divsP2 = divs + 2;
			//先生成顶角朝下的
			for (int i = 0; i < divs; ++i) {
				for (int j = 0; j < divs; ++j) {
					tempIndices.push_back(offset + i * divsP1 + j);
					tempIndices.push_back(offset + i * divsP1 + j + 1);
					tempIndices.push_back(offset + i * divsP1 + j + divsP2);
				}
			}
			//再生成顶角朝上的
			for (int i = 0; i < divs; ++i) {
				for (int j = 0; j < divs; ++j) {
					tempIndices.push_back(offset + i * divsP1 + j);
					tempIndices.push_back(offset + i * divsP1 + j + divsP2);
					tempIndices.push_back(offset + i * divsP1 + j + divsP1);
				}
			}
		}
				
		for (int i = 0; i < tempIndices.size(); i+=3) {
		indicesList_origin.push_back(tempIndices[i]);
		indicesList_origin.push_back(tempIndices[i+1]);
		indicesList_origin.push_back(tempIndices[i+2]);			
		}
		offset_loop += pointNums_offset;
	}
	/*
	//合并相同顶点
	int index = 0;
	for (Vertex vertex : vertexList_origin) {
		if (vertexMap.count(vertex) == 0) {
			vertexMap[vertex] = index;
			index++;
	   }
	}
	for (int i = 0; i < indicesList_origin.size(); i++) {
		int newIndex = vertexMap[vertexList_origin[indicesList_origin[i]]];
	}
	*/
	
	hasInited = true;
}

void Teapot::outputObjFile() {
	if (!hasInited)
		return;
	
	std::ofstream fs;
	fs.open("utahTeapotObjFile.obj");
	if (fs) {
		fs << "mtllib heart.mtl" << std::endl;
		fs << "o teapot" << std::endl;
		
		for (int i = 0; i < vertexList_origin.size(); i++) 
			fs << "v " << vertexList_origin[i].position.x << " " << vertexList_origin[i].position.y << " " << vertexList_origin[i].position.z << std::endl;

		for (int i = 0; i < vertexList_origin.size(); i++)
			fs  << "vn " << vertexList_origin[i].normal.x << " " << vertexList_origin[i].normal.y << " " << vertexList_origin[i].normal.z << std::endl;

		for (int i = 0; i < vertexList_origin.size(); i++)
			fs  << "vt " << vertexList_origin[i].texCoords.x << " " << vertexList_origin[i].texCoords.y << std::endl;

		fs << "s 1" << std::endl;
		fs << "usemtl DefaultShader" << std::endl;

		for (int i = 0; i < indicesList_origin.size()/3; i++) {
			fs  << "f " << indicesList_origin[i*3]+1 << "/" << indicesList_origin[i*3] + 1 << "/" << indicesList_origin[i*3] + 1 << " ";
			fs << indicesList_origin[i * 3+1] + 1 << "/" << indicesList_origin[i * 3+1] + 1 << "/" << indicesList_origin[i * 3+1] + 1 << " ";
			fs << indicesList_origin[i * 3+2] + 1 << "/" << indicesList_origin[i * 3+2] + 1 << "/" << indicesList_origin[i * 3+2] + 1 << " ";
			fs << std::endl;
		}
		fs << "s off" << std::endl;
	}
	fs.close();

	fs.open("utahTeapot_BezierSurfaceIndex.txt");
	if (fs) {
		fs << BezierSurfaceIndex.size() << std::endl;
		fs<< std::endl;
		for (int index : BezierSurfaceIndex) 
			fs<< index<< std::endl;
	}
	fs.close();

}

Vec3f evalBezierCurve(const Vec3f *P, const float &t)
{
	float b0 = (1 - t) * (1 - t) * (1 - t);
	float b1 = 3 * t * (1 - t) * (1 - t);
	float b2 = 3 * t * t * (1 - t);
	float b3 = t * t * t;

	return P[0] * b0 + P[1] * b1 + P[2] * b2 + P[3] * b3;
}

Vec3f evalBezierPatch(const Vec3f *controlPoints, const float &u, const float &v)
{
	Vec3f uCurve[4];
	for (int i = 0; i < 4; ++i)
		uCurve[i] = evalBezierCurve(controlPoints + 4 * i, u);

	return evalBezierCurve(uCurve, v);
}

Vec3f derivBezier(const Vec3f *P, const float &t)
{
	return -3 * (1 - t) * (1 - t) * P[0] +
		(3 * (1 - t) * (1 - t) - 6 * t * (1 - t)) * P[1] +
		(6 * t * (1 - t) - 3 * t * t) * P[2] +
		3 * t * t * P[3];
}

Vec3f dUBezier(const Vec3f *controlPoints, const float &u, const float &v)
{
	Vec3f P[4];
	Vec3f vCurve[4];
	for (int i = 0; i < 4; ++i) {
		P[0] = controlPoints[i];
		P[1] = controlPoints[4 + i];
		P[2] = controlPoints[8 + i];
		P[3] = controlPoints[12 + i];
		vCurve[i] = evalBezierCurve(P, v);
	}

	return derivBezier(vCurve, u);
}

Vec3f dVBezier(const Vec3f *controlPoints, const float &u, const float &v)
{
	Vec3f uCurve[4];
	for (int i = 0; i < 4; ++i) {
		uCurve[i] = evalBezierCurve(controlPoints + 4 * i, u);
	}

	return derivBezier(uCurve, v);
}