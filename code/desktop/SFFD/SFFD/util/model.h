#ifndef _MODEL_H_
#define _MODEL_H_

#define MAX_NEARESTTOUCHPOINT_DISTANCE_LIMIT 0.1
//#define TEST_CALSAMPLEPOINT_GROUNDTRUTH

//���øú�
//#define notuseComputeShader

//��ʱ�꿪��ʱ��Ӧ���ٿ����������ԡ�����꣬��Ȼ�ᵼ��ʱ����������
//#define TEST_SHOW_TIME
//���ã�ÿ������֡������io����
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
* ����һ��ģ�� ģ�Ϳ��԰���һ������Mesh
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
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)//Ŀǰֻ������1���ڵ��obj
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
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)//Ŀǰֻ������1���ڵ��obj
		{
#ifdef TEST_SHOW_TIME
			GLuint gLTimerId;
			GLint tempTime;
			glGenQueries(1, &gLTimerId);
#endif //  TEST_SHOW_TIME			
			
			//�����Ĳ�ֵ��ʱ��˳�������������꣨vec2���Ļ��棬�󶨵���10һֱ����
			
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
			setupSSBufferObjectList<GLfloat>(it->vertex_bsplineBase_SSBOID, 1, outputData, temp_nums, false);//���ÿ����b��������������ֵ
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
			//�ò������ã�����dffd
			glm::vec4 samplePointPos = it->vertData[0].position;
			boundbox->dffd_adjustControlPoints(samplePointPos.x, samplePointPos.y, samplePointPos.z, 3, 0, 0);
			*/
			/*
			glm::vec3 testResult=boundbox->test_calNormalAfterDeformation(glm::vec3(-0.5,0.5,0.5),glm::vec3(0,0,1));
			std::cout <<"���Է�������"<< testResult.x << "," << testResult.y << "," << testResult.z << std::endl;
			*/


			//��ʼ���β��裨֮��ÿ�θ��¶��㶼Ҫ���¼���
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
			//�󶨵�1~6Ҫ����
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
			//��ʼϸ��
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
			//��gpu�а󶨻�������
			it->setupDrawData();

#ifdef TEST_SHOW_TIME
			//����ÿ֡ʱ��
			boundbox->bspinebody->controlPoints_needUpdate = true;
#endif
			//it->showComputeResult();
			//������ƶ��㣬���ڶԱ�
			//boundbox->outputControlPoints();
		}
		
		
	}


	void drawModel(const Shader& renderShader) 
	{		
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)//�Ժ�Ҫ���Ƹ�����ģ������Ҫһ��shader���飬��ǰ���Դ�������ݣ�ֻ��һ������ڵ�����û������
		{
#ifndef FFD
			if (boundbox->isControlPointsNeedUpdate()) {//�����ƶ����Ѹ��£�������µļ���
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
				std::cout << "ÿ֡ʱ��:" << tempTime << std::endl;
#ifdef OUTPUT_TIME
				//��ʼ���ʱ���ļ�
				//GC��ʱ�ļ�
				std::ostringstream oss;
				oss << CutScheme::cut_space<<"_";
				std::string cutspaceStr(oss.str());
				oss.str("");
				oss << CutScheme::smoothSharpAngle<<".txt";
				std::string cutangleStr(oss.str());
				float angleTransform = 180.0f / 3.141592f;
				std::ofstream fs;
				fs.open("GC��ʱ�ļ�/time_" + cutspaceStr + cutangleStr);
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
		//Ϊ�˼���ground_truth��Ҫԭʼ��δ�ı��index��assimp�������д���
		std::ifstream myFs;
		myFs.open(filePath);
		string line;
		if (myFs) {
			while (std::getline(myFs, line)) // line�в�����ÿ�еĻ��з�  
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
						//ת��Ϊ0��,��ֻ��ȡuv�����������texcord�����±�
						objOriginIndices.push_back(index2 - 1);
					}
				}
			}
		}
		myFs.close();
#endif // TEST_CALSAMPLEPOINT_GROUNDTRUTH

		

		const aiScene* sceneObjPtr = importer.ReadFile(filePath, //aiProcess_Triangulate |aiProcess_FlipUVs|aiProcess_GenSmoothNormals
			aiProcess_Triangulate | aiProcess_FlipUVs|aiProcess_JoinIdenticalVertices);//����Ҳ����ֱ����ԭ���������ɷ���
	
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
		
		if(!boundbox->generateBspinebody())//�������Ĭ�ϲ������Ժ���Ӷ�ȡtxt���ò�����Ҳ�����صĲ�����ʼ��
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
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)//Ŀǰֻ������1���ڵ��obj
		{
			result = it->getNearestPoint(pos_x, pos_y, pos_z);
		}
		return result;
	}
	float pointToLineDistance3D(glm::vec3 p, glm::vec3 l1, glm::vec3 l2)
	{
		glm::vec3 line_vec = l2 - l1; //AB
		glm::vec3 point_vec = p - l1; //AP
		float d = glm::dot(line_vec, point_vec) / glm::length(line_vec); //ͶӰ�ĳ��� |AC|
		return pow(glm::length(point_vec), 2) - pow(d, 2); //���ɶ���|CP| = sqrt(AP^2-AC^2)
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
	* �ݹ鴦��ģ�͵Ľ��
	*/
	int processNode(const aiNode* node, const aiScene* sceneObjPtr)
	{
		int result = 0;
		if (!node || !sceneObjPtr)
		{
			return 0;
		}
		// �ȴ���������
		for (size_t i = 0; i < node->mNumMeshes; ++i)
		{
			// ע��node�е�mesh�Ƕ�sceneObject��mesh������
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
		
		// �����ӽ��
		for (size_t i = 0; i < node->mNumChildren; ++i)
		{
			if(i==0)//Ŀǰֻ����һ���ڵ㣬��һ��ģ��
			   result += this->processNode(node->mChildren[i], sceneObjPtr);
		}
		
		//std::cout << "�ڵ���Ƭ��=" << result << std::endl;
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
		std::cout << "��������=" << meshPtr->mNumVertices << std::endl;
		std::cout << "��������=" << meshPtr->mNumFaces << std::endl;
		// ��Mesh�õ��������ݡ�����������������
		for (size_t i = 0; i < meshPtr->mNumVertices; ++i)
		{
			Vertex vertex;
			// ��ȡ����λ��
			if (meshPtr->HasPositions())
			{
				vertex.position.x = meshPtr->mVertices[i].x;
				vertex.position.y = meshPtr->mVertices[i].y;
				vertex.position.z = meshPtr->mVertices[i].z;
				boundbox->updateBoundary(vertex);              
			}
			// ��ȡ�������� Ŀǰֻ����0������
			if (meshPtr->HasTextureCoords(0))
			{
				vertex.texCoords.x = meshPtr->mTextureCoords[0][i].x;
				vertex.texCoords.y = meshPtr->mTextureCoords[0][i].y;
			}
			else
			{
				vertex.texCoords = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			// ��ȡ����������
			if (meshPtr->HasNormals())
			{
				vertex.normal.x = meshPtr->mNormals[i].x;
				vertex.normal.y = meshPtr->mNormals[i].y;
				vertex.normal.z = meshPtr->mNormals[i].z;
			}
			vertData.push_back(vertex);
		}

		/*****Ϊ�˱��ں�CYM�ĳ���Ƚϣ��޸ĵ����ģ�͵�����****************/
		
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
				

		//����ڵ�д�С
		float x_knotboxSize = (boundbox->max[0] - boundbox->min[0]) / 2;
		float y_knotboxSize = (boundbox->max[1] - boundbox->min[1]) / 2;
		float z_knotboxSize = (boundbox->max[2] - boundbox->min[2]) / 2;
		float average_knotboxSize = (x_knotboxSize + y_knotboxSize + z_knotboxSize) / 3;
		std::cout << "x��ڵ�д�С="<< x_knotboxSize<<std::endl;
		std::cout << "y��ڵ�д�С=" << y_knotboxSize << std::endl;
		std::cout << "z��ڵ�д�С=" << z_knotboxSize << std::endl;
		std::cout << "ƽ���ڵ�д�С=" << average_knotboxSize << std::endl;
		/****����****************/



		// ��ȡ��������
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
		// ��ȡ��������
		if (meshPtr->mMaterialIndex >= 0)
		{
			const aiMaterial* materialPtr = sceneObjPtr->mMaterials[meshPtr->mMaterialIndex];
			// ��ȡdiffuse����
			std::vector<Texture> diffuseTexture;
			this->processMaterial(materialPtr, sceneObjPtr, aiTextureType_DIFFUSE, diffuseTexture);
			textures.insert(textures.end(), diffuseTexture.begin(), diffuseTexture.end());
			// ��ȡspecular����
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
	* ��ȡһ�������е�����
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
			if (it == this->loadedTextureMap.end()) // ����Ƿ��Ѿ����ع���
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
	std::vector<Mesh> meshes; // ����Mesh���Ƕ�ȡ�ļ�����õ�����
	std::string modelFileDir; // ����ģ���ļ����ļ���·��
	typedef std::map<std::string, Texture> LoadedTextMapType; // key = texture file path
	LoadedTextMapType loadedTextureMap; // �����Ѿ����ص�����
	int numOfPatches;//��Ƭ��
	float delta[3];//��ȡģ��ʱ�Լ���λ�ý��еĵ����������ں�CYM�Աȣ�
	float maxXYZ;
#ifdef  TEST_CALSAMPLEPOINT_GROUNDTRUTH
					 //������ʵֵ
	void printGroundTruth() {
		CYMResult cymResult = CYMResult();
		cymResult.loadFile_OriginSamplePoint();

		//��ȡbezier����Ķ�������
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
			//ԭʼ�������������uv
			float u[3], v[3];
			//glm::vec3 origin_pos[3];
			for (int i = 0; i < 3; i++) {
				int vertexIndex = meshes[0].origin_indices[originTriangleIndex * 3 + i];
				u[i] = meshes[0].origin_vertData[vertexIndex].texCoords.x;
				v[i] = meshes[0].origin_vertData[vertexIndex].texCoords.y;
				//origin_pos[i]= glm::vec3(meshes[0].origin_vertData[vertexIndex].position);
			}		
			
			/********************************ע��***********************************************/
			//�����������֧�ĺ겻�ܺ�TEST_CALSAMPLEPOINT_CYMͬʱ����������samplePoint.centroidCoordinates�����ʷֺ��������ڵ���������
			//������xyz��������
			float x_paramter = samplePoint.centroidCoordinates.x;
			float y_paramter = samplePoint.centroidCoordinates.y;
			float z_paramter = samplePoint.centroidCoordinates.z;
			//�������groundTruth
			float u_truth = x_paramter*u[0] + y_paramter*u[1] + z_paramter*u[2];
			float v_truth = x_paramter*v[0] + y_paramter*v[1] + z_paramter*v[2];
			//�������ɵ�obj�ļ���һ�������ε�����һ��ͬ����ԭʼ�ĵ���bezier���棬��ֻȡ�����ε�һ�������surface�±�
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
				//���������ɻ�����ȡ�˹�ָ������
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
			//����b������Ͱ�Χ�У�������κ��λ���뷨��

			glm::vec3 normal_groundTruth = boundbox->test_calNormalAfterDeformation(pos, nor);
			glm::vec3 position_groundTruth = boundbox->test_calPositionAfterDeformation_FFD(pos);
			groundTruthResult_position.push_back(position_groundTruth);
			groundTruthResult_normal.push_back(normal_groundTruth);
		}
		//������
		std::ofstream ofs;
		ofs.open("��������κ�GroundTruth_��������.txt");
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

		ofs.open("��������κ�GroundTruth_������.txt");
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