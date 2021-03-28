#ifndef _MODEL_H_
#define _MODEL_H_

#define MAX_NEARESTTOUCHPOINT_DISTANCE_LIMIT 0.1
//#define TEST_CALSAMPLEPOINT_GROUNDTRUTH

//弃用该宏
//#define notuseComputeShader

//计时宏开启时不应该再开启其他测试、计算宏，不然会导致时间大幅度增加
//#define TEST_SHOW_TIME
//慎用，每个变形帧都会有io操作
//#define OUTPUT_TIME

#include <map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "mesh.h"
#include "texture.h"
#include "boundbox.h"
#include "camera.h"
#include "teapotUtil.h";
#include "bspinebody.h"
#include "mouse.h"
/*
* 代表一个模型 模型可以包含一个或多个Mesh
*/
/*
Shader computeShader_CP("shader/centroid_coordinates_interpolation.comp");
Shader ComputeShader_bsplineBase("shader/bsplineBase.comp");
Shader ComputeShader_deformation("shader/deformation.comp");
Shader ComputeShader_bezierTriangle("shader/bezierTriangleControlPoints.comp");
Shader computeShader_tessellation("shader/tessellation.comp");
*/
class Model
{
public:
	Model() {
		totalComputeTimers = 0;
	}
	int totalComputeTimers;
	std::vector<int> times;
	Boundbox* boundbox;
	Shader computeShader_CP, ComputeShader_bsplineBase, ComputeShader_deformation, ComputeShader_bezierTriangle, computeShader_tessellation;
	glm::vec3 light_Pos;
#ifdef TEST_CALSAMPLEPOINT_GROUNDTRUTH
	std::vector<int> objOriginIndices;
#endif
	void setComputeData()
	{		
#ifdef FFD
		boundbox->test_ffd_calBase();
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)//目前只适用于1个节点的obj
		{
			int vertSize = it->vertData.size();
			for (int i = 0; i < vertSize; i++) {
				Vertex v = it->vertData[i];
				it->vertData[i].position = glm::vec4(boundbox->test_calPositionAfterDeformation_FFD(glm::vec3(v.position)),0.0f);
				it->vertData[i].normal = glm::vec4(boundbox->test_calNormalAfterDeformation(glm::vec3(v.position), glm::vec3(v.normal)),0.0f);
			}
			it->setupDrawData_raw();
		}
		return;
#endif // FFD

		
		computeShader_CP.loadComputeFile("shader/centroid_coordinates_interpolation.comp");
		ComputeShader_bsplineBase.loadComputeFile("shader/bsplineBase.comp");
		ComputeShader_deformation.loadComputeFile("shader/deformation.comp");
		ComputeShader_bezierTriangle.loadComputeFile("shader/bezierTriangleControlPoints.comp");
		computeShader_tessellation.loadComputeFile("shader/tessellation.comp");

		/*
		Shader computeShader_CP("shader/centroid_coordinates_interpolation.comp");
		Shader ComputeShader_bsplineBase("shader/bsplineBase.comp");
		Shader ComputeShader_deformation("shader/deformation.comp");
		Shader ComputeShader_bezierTriangle("shader/bezierTriangleControlPoints.comp");
		Shader computeShader_tessellation("shader/tessellation.comp");
		*/
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)//目前只适用于1个节点的obj
		{
#ifdef TEST_SHOW_TIME
			GLuint gLTimerId;
			GLint tempTime;
			glGenQueries(1, &gLTimerId);
#endif //  TEST_SHOW_TIME			
			
			//在重心插值的时候顺便生成纹理坐标（vec2）的缓存，绑定到点10一直保存
			
#ifdef TEST_SHOW_TIME
			glBeginQuery(GL_TIME_ELAPSED, gLTimerId);
#endif 	
			it->setupComputeData_CP(computeShader_CP);
#ifdef TEST_SHOW_TIME
			glEndQuery(GL_TIME_ELAPSED);
			glGetQueryObjectiv(gLTimerId, GL_QUERY_RESULT, &tempTime);
			tempTime /=  1000000;
			times.push_back(tempTime);
			totalComputeTimers += tempTime;
#endif 
			

#ifdef TEST_SHOW_TIME
			glBeginQuery(GL_TIME_ELAPSED, gLTimerId);
#endif 
			it->doPrecompute_perTriangle(computeShader_CP);			
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#ifdef TEST_SHOW_TIME
			glEndQuery(GL_TIME_ELAPSED);
			glGetQueryObjectiv(gLTimerId, GL_QUERY_RESULT, &tempTime);
			tempTime /= 1000000;
			times.push_back(tempTime);
			totalComputeTimers += tempTime ;
#endif 
			

			glDeleteBuffers(1, &it->inputVertexSSBOID);
			glDeleteBuffers(1, &it->inputIndicesSSBOID);
			//it->showComputeResult();

			int totalControlPointNums = boundbox->getTotalControlPointNums();
			int temp_nums = it->indices.size() / 3 * 64 * (totalControlPointNums * 2 - 3);
			GLfloat* outputData = (GLfloat*)malloc(sizeof(GLfloat) * temp_nums);
			setupSSBufferObjectList<GLfloat>(it->vertex_bsplineBase_SSBOID, 1, outputData, temp_nums, false);//存放每个点b样条基计算后的数值
			free(outputData);

#ifdef TEST_SHOW_TIME
			glBeginQuery(GL_TIME_ELAPSED, gLTimerId);
#endif 
			boundbox->setupComputeData_bsplineBase(ComputeShader_bsplineBase);

#ifdef TEST_CALSAMPLEPOINT_GROUNDTRUTH
			printGroundTruth();
#endif // TEST_CALSAMPLEPOINT_GROUNDTRUTH


			it->setupComputeData_point_nums(ComputeShader_bsplineBase);
			it->doPrecompute_perPoint(ComputeShader_bsplineBase);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#ifdef TEST_SHOW_TIME
			glEndQuery(GL_TIME_ELAPSED);
			glGetQueryObjectiv(gLTimerId, GL_QUERY_RESULT, &tempTime);
			tempTime /= 1000000;
			times.push_back(tempTime);
			totalComputeTimers += tempTime ;
#endif 

			//it->showComputeResult();
			for (int i = 0; i < 3; i++) {
				glDeleteBuffers(1, &boundbox->baseSSbo[i]);
				glDeleteBuffers(1, &boundbox->baseSSBO_low[i]);
			}

			/*
			//该步测试用，测试dffd
			glm::vec4 samplePointPos = it->vertData[0].position;
			boundbox->dffd_adjustControlPoints(samplePointPos.x, samplePointPos.y, samplePointPos.z, 3, 0, 0);
			*/
			/*
			glm::vec3 testResult=boundbox->test_calNormalAfterDeformation(glm::vec3(-0.5,0.5,0.5),glm::vec3(0,0,1));
			std::cout <<"测试法向结果："<< testResult.x << "," << testResult.y << "," << testResult.z << std::endl;
			*/


			//开始变形步骤（之后每次更新顶点都要重新计算
#ifdef TEST_SHOW_TIME
			glBeginQuery(GL_TIME_ELAPSED, gLTimerId);
#endif 
			boundbox->updateControlPoints();
#ifdef TEST_SHOW_TIME
			glEndQuery(GL_TIME_ELAPSED);
			glGetQueryObjectiv(gLTimerId, GL_QUERY_RESULT, &tempTime);
			tempTime /= 1000000;
			times.push_back(tempTime);
			totalComputeTimers += tempTime ;
#endif 


			it->setupComputeData_point_nums(ComputeShader_deformation);
			boundbox->setupComputeData_deformation(ComputeShader_deformation);
#ifdef TEST_SHOW_TIME
			glBeginQuery(GL_TIME_ELAPSED, gLTimerId);
#endif 
			it->doPrecompute_perPoint(ComputeShader_deformation);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#ifdef TEST_SHOW_TIME
			glEndQuery(GL_TIME_ELAPSED);
			glGetQueryObjectiv(gLTimerId, GL_QUERY_RESULT, &tempTime);
			tempTime /= 1000000;
			times.push_back(tempTime);
			totalComputeTimers += tempTime;
#endif 
			//it->showComputeResult();
			//绑定点1~6要保留
#ifdef TEST_SHOW_TIME
			glBeginQuery(GL_TIME_ELAPSED, gLTimerId);
#endif 
			it->setupComputeData_bezierTriangle(ComputeShader_bezierTriangle);
#ifdef TEST_SHOW_TIME
			glEndQuery(GL_TIME_ELAPSED);
			glGetQueryObjectiv(gLTimerId, GL_QUERY_RESULT, &tempTime);
			tempTime /= 1000000;
			times.push_back(tempTime);
			totalComputeTimers += tempTime ;
#endif 

#ifdef TEST_SHOW_TIME
			glBeginQuery(GL_TIME_ELAPSED, gLTimerId);
#endif 
			it->doPrecompute_perTriangle(ComputeShader_bezierTriangle);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#ifdef TEST_SHOW_TIME
			glEndQuery(GL_TIME_ELAPSED);
			glGetQueryObjectiv(gLTimerId, GL_QUERY_RESULT, &tempTime);
			tempTime /= 1000000;
			times.push_back(tempTime);
			totalComputeTimers += tempTime;
#endif 

#ifdef TEST_CALSAMPLEPOINT_CYM
			it->outputFile_CYMSamplePoint_MyResult();
#endif

			//it->showComputeResult();
			//开始细分
#ifdef TEST_SHOW_TIME
			glBeginQuery(GL_TIME_ELAPSED, gLTimerId);
#endif 
			it->setupComputeData_tessellation(computeShader_tessellation);
			boundbox->setupComputeData_precomputeMatrix_tessellation();
#ifdef TEST_SHOW_TIME
			glEndQuery(GL_TIME_ELAPSED);
			glGetQueryObjectiv(gLTimerId, GL_QUERY_RESULT, &tempTime);
			tempTime /= 1000000;
			times.push_back(tempTime);
			totalComputeTimers += tempTime ;
#endif 

#ifdef TEST_SHOW_TIME
			glBeginQuery(GL_TIME_ELAPSED, gLTimerId);
#endif
			it->doPrecompute_perTriangle(computeShader_tessellation);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#ifdef TEST_SHOW_TIME
			glEndQuery(GL_TIME_ELAPSED);
			glGetQueryObjectiv(gLTimerId, GL_QUERY_RESULT, &tempTime);
			tempTime /= 1000000;
			times.push_back(tempTime);
			totalComputeTimers += tempTime ;
#endif 
			//在gpu中绑定绘制数据
			it->setupDrawData();

#ifdef TEST_SHOW_TIME
			//给出每帧时间
			boundbox->bspinebody->controlPoints_needUpdate = true;
#endif
			//it->showComputeResult();
			//输出控制顶点，便于对比
			//boundbox->outputControlPoints();
		}
		
		
	}


	void drawModel(const Shader& renderShader) 
	{		
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)//以后要绘制复数个模型则需要一个shader数组，提前各自传输好数据，只有一个网格节点所以没有问题
		{
#ifndef FFD
			if (boundbox->isControlPointsNeedUpdate()) {//若控制顶点已更新，则进行新的计算
#ifdef TEST_SHOW_TIME
				GLuint gLTimerId;
				GLint tempTime;
				glGenQueries(1, &gLTimerId);
				glBeginQuery(GL_TIME_ELAPSED, gLTimerId);
#endif
				boundbox->updateControlPoints();
				it->doPrecompute_perPoint(ComputeShader_deformation);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				it->doPrecompute_perTriangle(ComputeShader_bezierTriangle);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				it->doPrecompute_perTriangle(computeShader_tessellation);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#ifdef TEST_SHOW_TIME
				glEndQuery(GL_TIME_ELAPSED);
				glGetQueryObjectiv(gLTimerId, GL_QUERY_RESULT, &tempTime);
				tempTime /= 1000000;				
				//totalComputeTimers += tempTime;
				//perFrameComputeTimers += tempTime;
				std::cout << "每帧时间:" << tempTime << std::endl;
#ifdef OUTPUT_TIME
				//开始输出时间文件
				//GC计时文件
				std::ostringstream oss;
				oss << CutScheme::cut_space<<"_";
				std::string cutspaceStr(oss.str());
				oss.str("");
				oss << CutScheme::smoothSharpAngle<<".txt";
				std::string cutangleStr(oss.str());
				float angleTransform = 180.0f / 3.141592f;
				std::ofstream fs;
				fs.open("GC计时文件/time_" + cutspaceStr + cutangleStr);
				if (fs) {
					fs << totalComputeTimers << std::endl;
					fs << tempTime<< std::endl;
					fs<< std::endl;
					for (int t : times)
						fs << t << std::endl;
				}
				fs.close();
#endif // OUTPUT_TIME				
#endif
		}
#endif 
			it->draw(renderShader);
 
		}

	}
	void drawSelectedPoint(const Shader& shader)const {
		boundbox->drawSeletedPoint(shader);
	}
	void drawBoundbox(const Shader& shader) const
	{
		boundbox->draw(shader);	
	}
	bool loadModel(const std::string& filePath)
	{
		boundbox = new Boundbox();
		Assimp::Importer importer;
		if (filePath.empty())
		{
			std::cerr << "Error:Model::loadModel, empty model file path." << std::endl;
			return false;
		}

#ifdef TEST_CALSAMPLEPOINT_GROUNDTRUTH
		//为了计算ground_truth需要原始的未改变的index（assimp会额外进行处理）
		std::ifstream myFs;
		myFs.open(filePath);
		string line;
		if (myFs) {
			while (std::getline(myFs, line)) // line中不包括每行的换行符  
			{
				std::istringstream iss(line);
				string primitive;
				iss >> primitive;
				if (primitive != "f")
					continue;
				else {
					char slash1, slash2;
					int index1, index2, index3;
					while (iss >> index1 >> slash1 >> index2 >> slash2 >> index3) {
						//转换为0基,且只获取uv（这里放置在texcord）的下标
						objOriginIndices.push_back(index2 - 1);
					}
				}
			}
		}
		myFs.close();
#endif // TEST_CALSAMPLEPOINT_GROUNDTRUTH

		

		const aiScene* sceneObjPtr = importer.ReadFile(filePath, //aiProcess_Triangulate |aiProcess_FlipUVs|aiProcess_GenSmoothNormals
			aiProcess_Triangulate | aiProcess_FlipUVs|aiProcess_JoinIdenticalVertices);//这里也可以直接在原数据上生成法向
	
		if (!sceneObjPtr
			|| sceneObjPtr->mFlags == AI_SCENE_FLAGS_INCOMPLETE
			|| !sceneObjPtr->mRootNode)
		{
			std::cerr << "Error:Model::loadModel, description: " 
				<< importer.GetErrorString() << std::endl;
			return false;
		}
		this->modelFileDir = filePath.substr(0, filePath.find_last_of('/'));

		numOfPatches = this->processNode(sceneObjPtr->mRootNode, sceneObjPtr);
		if (!numOfPatches)
		{
			std::cerr << "Error:Model::loadModel, process node failed."<< std::endl;
			return false;
		}
		
		if(!boundbox->generateBspinebody())//这里采用默认参数，以后添加读取txt设置参数，也有重载的参数初始化
			return false;

		//light_Pos = glm::vec3(boundbox->max[0]* scale_factor*2.0f,boundbox->max[1]* scale_factor*2.0f,boundbox->max[2]* scale_factor*2.0f);

		return true;
	}

	~Model()
	{
		for (std::vector<Mesh>::const_iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)
		{
			it->final();
		}
	}



	int getNumOfPatches() {
		return numOfPatches;
	}
	Vertex getNearestPoint(float pos_x, float pos_y, float pos_z) {
		Vertex result;
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)//目前只适用于1个节点的obj
		{
			result = it->getNearestPoint(pos_x, pos_y, pos_z);
		}
		return result;
	}
	float pointToLineDistance3D(glm::vec3 p, glm::vec3 l1, glm::vec3 l2)
	{
		glm::vec3 line_vec = l2 - l1; //AB
		glm::vec3 point_vec = p - l1; //AP
		float d = glm::dot(line_vec, point_vec) / glm::length(line_vec); //投影的长度 |AC|
		return pow(glm::length(point_vec), 2) - pow(d, 2); //勾股定理：|CP| = sqrt(AP^2-AC^2)
	}
	bool getNearestPoint_AmongControPoints(glm::vec3 point1, glm::vec3 point2, const Bspinebody* bsplinevolume, SelectPoint& selectP) {
		int index_x = 0;
		int index_y = 0;
		int index_z = 0;

		float min_distance = 100000;
		for (int i = 0; i < bsplinevolume->controlPointNums[0]; i++)
			for (int j = 0; j < bsplinevolume->controlPointNums[1]; j++)
				for (int k = 0; k < bsplinevolume->controlPointNums[2]; k++) {
					glm::vec4 temp_point = bsplinevolume->controlPoints[i][j][k];
					float distance = pointToLineDistance3D(glm::vec3(temp_point), point1, point2);
					if (distance <= min_distance) {
						index_x = i;
						index_y = j;
						index_z = k;
						min_distance = distance;
					}
				}
		
		if (min_distance>MAX_NEARESTTOUCHPOINT_DISTANCE_LIMIT)
			return false;
		else {
			glm::vec3 tempVec = glm::vec3(bsplinevolume->controlPoints[index_x][index_y][index_z]);
			selectP = SelectPoint(index_x, index_y, index_z, tempVec);
		}

		return true;
	}

private:
	/*
	* 递归处理模型的结点
	*/
	int processNode(const aiNode* node, const aiScene* sceneObjPtr)
	{
		int result = 0;
		if (!node || !sceneObjPtr)
		{
			return 0;
		}
		// 先处理自身结点
		for (size_t i = 0; i < node->mNumMeshes; ++i)
		{
			// 注意node中的mesh是对sceneObject中mesh的索引
			const aiMesh* meshPtr = sceneObjPtr->mMeshes[node->mMeshes[i]]; 
			if (meshPtr)
			{
				Mesh meshObj;
				int tempResult = this->processMesh(meshPtr, sceneObjPtr, meshObj);
				result += tempResult;
				if (tempResult)
				{
					this->meshes.push_back(meshObj);
				}
			}
		}
		
		// 处理孩子结点
		for (size_t i = 0; i < node->mNumChildren; ++i)
		{
			if(i==0)//目前只处理一个节点，即一个模型
			   result += this->processNode(node->mChildren[i], sceneObjPtr);
		}
		
		//std::cout << "节点面片数=" << result << std::endl;
		return result;
	}
	int processMesh(const aiMesh* meshPtr, const aiScene* sceneObjPtr, Mesh& meshObj)
	{
		if (!meshPtr || !sceneObjPtr)
		{
			return 0;
		}
		std::vector<Vertex> vertData;
		std::vector<Texture> textures;
		std::vector<GLuint> indices;
		std::cout << "顶点数量=" << meshPtr->mNumVertices << std::endl;
		std::cout << "索引数量=" << meshPtr->mNumFaces << std::endl;
		// 从Mesh得到顶点数据、法向量、纹理数据
		for (size_t i = 0; i < meshPtr->mNumVertices; ++i)
		{
			Vertex vertex;
			// 获取顶点位置
			if (meshPtr->HasPositions())
			{
				vertex.position.x = meshPtr->mVertices[i].x;
				vertex.position.y = meshPtr->mVertices[i].y;
				vertex.position.z = meshPtr->mVertices[i].z;
				boundbox->updateBoundary(vertex);              
			}
			// 获取纹理数据 目前只处理0号纹理
			if (meshPtr->HasTextureCoords(0))
			{
				vertex.texCoords.x = meshPtr->mTextureCoords[0][i].x;
				vertex.texCoords.y = meshPtr->mTextureCoords[0][i].y;
			}
			else
			{
				vertex.texCoords = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			// 获取法向量数据
			if (meshPtr->HasNormals())
			{
				vertex.normal.x = meshPtr->mNormals[i].x;
				vertex.normal.y = meshPtr->mNormals[i].y;
				vertex.normal.z = meshPtr->mNormals[i].z;
			}
			vertData.push_back(vertex);
		}

		/*****为了便于和CYM的程序比较，修改导入后模型的坐标****************/
		
		delta[0] = (boundbox->min[0] + boundbox->max[0])*-0.5f;
		delta[1] = (boundbox->min[1] + boundbox->max[1])*-0.5f;
		delta[2] = (boundbox->min[2] + boundbox->max[2])*-0.5f;
		
		boundbox->min[0] += delta[0];
		boundbox->max[0] += delta[0];
		boundbox->min[1] += delta[1];
		boundbox->max[1] += delta[1];
		boundbox->min[2] += delta[2];
		boundbox->max[2] += delta[2];

		maxXYZ=0;
		if (boundbox->max[0] > maxXYZ)
			maxXYZ = boundbox->max[0];
		if (boundbox->max[1] > maxXYZ)
			maxXYZ = boundbox->max[1];
		if (boundbox->max[2] > maxXYZ)
			maxXYZ = boundbox->max[2];

		boundbox->min[0] /= maxXYZ;
		boundbox->max[0] /= maxXYZ;
		boundbox->min[1] /= maxXYZ;
		boundbox->max[1] /= maxXYZ;
		boundbox->min[2] /= maxXYZ;
		boundbox->max[2] /= maxXYZ;

		for (int i = 0; i < vertData.size(); i++) {
			vertData[i].position.x += delta[0];
			vertData[i].position.x /= maxXYZ;
			vertData[i].position.y += delta[1];
			vertData[i].position.y /= maxXYZ;
			vertData[i].position.z += delta[2];
			vertData[i].position.z /= maxXYZ;
		}
				

		//输出节点盒大小
		float x_knotboxSize = (boundbox->max[0] - boundbox->min[0]) / 2;
		float y_knotboxSize = (boundbox->max[1] - boundbox->min[1]) / 2;
		float z_knotboxSize = (boundbox->max[2] - boundbox->min[2]) / 2;
		float average_knotboxSize = (x_knotboxSize + y_knotboxSize + z_knotboxSize) / 3;
		std::cout << "x轴节点盒大小="<< x_knotboxSize<<std::endl;
		std::cout << "y轴节点盒大小=" << y_knotboxSize << std::endl;
		std::cout << "z轴节点盒大小=" << z_knotboxSize << std::endl;
		std::cout << "平均节点盒大小=" << average_knotboxSize << std::endl;
		/****结束****************/



		// 获取索引数据
		for (size_t i = 0; i < meshPtr->mNumFaces; ++i)
		{
			aiFace face = meshPtr->mFaces[i];
			if (face.mNumIndices != 3)
			{
				std::cerr << "Error:Model::processMesh, mesh not transformed to triangle mesh." << std::endl;
				return 0;
			}
			for (size_t j = 0; j < face.mNumIndices; ++j)
			{
				indices.push_back(face.mIndices[j]);
			}
		}
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
		meshObj.setData(vertData, textures, indices);
#ifdef notuseComputeShader
		//meshObj.changeTrianlgeColor(141, 1);
		//meshObj.changeTrianlgeColor(176, 1);
		meshObj.printTrianlgeInfo(141, 1);
		meshObj.printTrianlgeInfo(176, 1);
		meshObj.setupDrawData_raw();	
#endif 
		return meshPtr->mNumFaces;
	}
	
	/*
	* 获取一个材质中的纹理
	*/
	bool processMaterial(const aiMaterial* matPtr, const aiScene* sceneObjPtr, 
		const aiTextureType textureType, std::vector<Texture>& textures)
	{
		textures.clear();

		if (!matPtr 
			|| !sceneObjPtr )
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
			std::string absolutePath = this->modelFileDir + "/" + textPath.C_Str();
			LoadedTextMapType::const_iterator it = this->loadedTextureMap.find(absolutePath);
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
	

private:
	std::vector<Mesh> meshes; // 保存Mesh，是读取文件后处理好的数据
	std::string modelFileDir; // 保存模型文件的文件夹路径
	typedef std::map<std::string, Texture> LoadedTextMapType; // key = texture file path
	LoadedTextMapType loadedTextureMap; // 保存已经加载的纹理
	int numOfPatches;//面片数
	float delta[3];//读取模型时对几何位置进行的调整量（便于和CYM对比）
	float maxXYZ;
#ifdef  TEST_CALSAMPLEPOINT_GROUNDTRUTH
					 //计算真实值
	void printGroundTruth() {
		CYMResult cymResult = CYMResult();
		cymResult.loadFile_OriginSamplePoint();

		//读取bezier曲面的顶点索引
		std::vector<int> vertex_bezierSurfaceIndex = std::vector<int>();
		std::ifstream fs;
		fs.open("utahTeapot_BezierSurfaceIndex.txt");
		if (fs) {
			int vertexNums;
			fs >> vertexNums;
			for (int i = 0; i < vertexNums; i++) {
				int temp;
				fs >> temp;
				vertex_bezierSurfaceIndex.push_back(temp);
			}
		}
		fs.close();

		std::vector<glm::vec3> groundTruthResult_position= std::vector<glm::vec3>();
		std::vector<glm::vec3> groundTruthResult_normal = std::vector<glm::vec3>();
		int index = -1;
		for (CYMSamplePoint samplePoint: cymResult.originSamplePoints) {
			index++;
			
				
			int originTriangleIndex = samplePoint.originTriangleIndex;
			//原始三角形三顶点的uv
			float u[3], v[3];
			//glm::vec3 origin_pos[3];
			for (int i = 0; i < 3; i++) {
				int vertexIndex = meshes[0].origin_indices[originTriangleIndex * 3 + i];
				u[i] = meshes[0].origin_vertData[vertexIndex].texCoords.x;
				v[i] = meshes[0].origin_vertData[vertexIndex].texCoords.y;
				//origin_pos[i]= glm::vec3(meshes[0].origin_vertData[vertexIndex].position);
			}		
			
			/********************************注意***********************************************/
			//这里该条件分支的宏不能和TEST_CALSAMPLEPOINT_CYM同时开启，否则samplePoint.centroidCoordinates会变成剖分后三角形内的质心坐标
			//采样点xyz质心坐标
			float x_paramter = samplePoint.centroidCoordinates.x;
			float y_paramter = samplePoint.centroidCoordinates.y;
			float z_paramter = samplePoint.centroidCoordinates.z;
			//各个点的groundTruth
			float u_truth = x_paramter*u[0] + y_paramter*u[1] + z_paramter*u[2];
			float v_truth = x_paramter*v[0] + y_paramter*v[1] + z_paramter*v[2];
			//由于生成的obj文件中一个三角形的三点一定同属于原始的单个bezier曲面，故只取三角形第一个顶点的surface下标
			int vertexIndex = objOriginIndices[originTriangleIndex * 3 ];
			int surfaceIndex = vertex_bezierSurfaceIndex[vertexIndex];
			glm::vec3 controlPoints[16];
			for (int i = 0; i < 16; ++i)
				controlPoints[i] = glm::vec3(teapotVertices[teapotPatches[surfaceIndex][i] - 1][0], teapotVertices[teapotPatches[surfaceIndex][i] - 1][1], teapotVertices[teapotPatches[surfaceIndex][i] - 1][2]);
			
			Vec3f pos = evalBezierPatch(controlPoints, u_truth, v_truth);
			
			pos += glm::vec3(delta[0], delta[1], delta[2]);
			pos /= maxXYZ;
			Vec3f dU = dUBezier(controlPoints, u_truth, v_truth);
			Vec3f dV = dVBezier(controlPoints, u_truth, v_truth);
			Vec3f nor=glm::vec3();
			if (dU == glm::vec3(0, 0, 0) || dV == glm::vec3(0, 0, 0)) {
				//对于两个蜕化点则取人工指定法向
				if ((surfaceIndex == 20 || surfaceIndex == 21 || surfaceIndex == 22 || surfaceIndex == 23))
					nor = glm::vec3(0, 0, 1);
				else if((surfaceIndex == 28 || surfaceIndex == 29 || surfaceIndex == 30 || surfaceIndex == 31))
					nor = glm::vec3(0, 0, -1);
			}
			else
			    nor= glm::normalize(glm::cross(dU, dV));

			
			/*
			Vec3f pos_1= evalBezierPatch(controlPoints, u[0], v[0]);
			pos_1 += glm::vec3(delta[0], delta[1], delta[2]);
			Vec3f pos_2 = evalBezierPatch(controlPoints, u[1], v[1]);
			pos_2 += glm::vec3(delta[0], delta[1], delta[2]);
			Vec3f pos_3 = evalBezierPatch(controlPoints, u[2], v[2]);
			pos_3 += glm::vec3(delta[0], delta[1], delta[2]);
			*/
			//带入b样条体和包围盒，求出变形后的位置与法向

			glm::vec3 normal_groundTruth = boundbox->test_calNormalAfterDeformation(pos, nor);
			glm::vec3 position_groundTruth = boundbox->test_calPositionAfterDeformation_FFD(pos);
			groundTruthResult_position.push_back(position_groundTruth);
			groundTruthResult_normal.push_back(normal_groundTruth);
		}
		//输出结果
		std::ofstream ofs;
		ofs.open("采样点变形后GroundTruth_几何坐标.txt");
		if (fs) {
			int size = groundTruthResult_position.size();
			ofs << size << std::endl;
			for (int i = 0; i < size; i++) {
				ofs << groundTruthResult_position[i].x << " " << groundTruthResult_position[i].y << " " << groundTruthResult_position[i].z << std::endl;
				if(i%3==2)
					ofs  << std::endl;
			}		
		}
		ofs.close();

		ofs.open("采样点变形后GroundTruth_法向量.txt");
		if (fs) {
			int size = groundTruthResult_normal.size();
			ofs << size << std::endl;
			for (int i = 0; i < size; i++) {
				ofs << groundTruthResult_normal[i].x << " " << groundTruthResult_normal[i].y << " " << groundTruthResult_normal[i].z << std::endl;
				if (i % 3 == 2)
					ofs << std::endl;
			}
		}
		ofs.close();
	}
#endif //  TEST_CALSAMPLEPOINT_GROUNDTRUTH
};

#endif