#ifndef _MODEL_H_
#define _MODEL_H_

//#define SHOW_ORIGINMODEL
#include <map>
#include <../inc/assimp/Importer.hpp>
#include <../inc/assimp/scene.h>
#include <../inc/assimp/postprocess.h>
#include "mesh.h"
#include "../sample/boundbox.h"
#include "../util/LogUtil.h"
#include <opencv2/opencv.hpp>
#include "../util/ssbo.h"
#include <math.h>
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
	}
	Boundbox* boundbox;
	Shader computeShader_CP, ComputeShader_bsplineBase, ComputeShader_deformation, ComputeShader_bezierTriangle, computeShader_tessellation;
	bool show_originMesh_remesh=false;

	void setupDrawData(){
		if(show_originMesh_remesh)
		    meshes[0].setupDrawData_raw();
		else
			meshes[0].setupDrawData();
	}

	void loadComputeShader(){
		computeShader_CP.loadComputeFile("/sdcard/shader/centroid_coordinates_interpolation.comp");
		ComputeShader_bsplineBase.loadComputeFile("/sdcard/shader/bsplineBase.comp");
		ComputeShader_deformation.loadComputeFile("/sdcard/shader/deformation.comp");
		ComputeShader_bezierTriangle.loadComputeFile("/sdcard/shader/bezierTriangleControlPoints.comp");
		computeShader_tessellation.loadComputeFile("/sdcard/shader/tessellation.comp");
	}
	void setComputeData()
	{
	    if(show_originMesh_remesh)
	        return;
		/*
		Shader computeShader_CP("shader/centroid_coordinates_interpolation.comp");
		Shader ComputeShader_bsplineBase("shader/bsplineBase.comp");
		Shader ComputeShader_deformation("shader/deformation.comp");
		Shader ComputeShader_bezierTriangle("shader/bezierTriangleControlPoints.comp");
		Shader computeShader_tessellation("shader/tessellation.comp");
		*/
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)//Ŀǰֻ������1���ڵ��obj
		{
            int glerrorint=(int)glGetError();

			//�����Ĳ�ֵ��ʱ��˳�������������꣨vec2���Ļ��棬�󶨵���10һֱ����
			it->setupComputeData_CP(computeShader_CP);
			it->doPrecompute_perTriangle(computeShader_CP);			
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			//it->showComputeResult();
			glDeleteBuffers(1, it->inputVertexSSBOID);
			glDeleteBuffers(1, it->inputIndicesSSBOID);


			int totalControlPointNums = boundbox->getTotalControlPointNums();
			int temp_nums = it->indices.size() / 3 * 64 * (totalControlPointNums * 2 - 3);
			GLfloat* outputData = (GLfloat*)malloc(sizeof(GLfloat) * temp_nums);
			setupSSBufferObjectList<GLfloat>(it->vertex_bsplineBase_SSBOID, 1, outputData, temp_nums, false);//���ÿ����b��������������ֵ
			free(outputData);
			glerrorint=(int)glGetError();
			boundbox->setupComputeData_bsplineBase(ComputeShader_bsplineBase);
			it->setupComputeData_point_nums(ComputeShader_bsplineBase);
			glerrorint=(int)glGetError();
			it->doPrecompute_perPoint(ComputeShader_bsplineBase);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			glerrorint=(int)glGetError();

			//it->showComputeResult();

			for (int i = 0; i < 3; i++) {
				glDeleteBuffers(1, boundbox->baseSSbo[i]);
				glDeleteBuffers(1, boundbox->baseSSBO_low[i]);
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
			boundbox->updateControlPoints();
			glerrorint=(int)glGetError();
			it->setupComputeData_point_nums(ComputeShader_deformation);
			boundbox->setupComputeData_deformation(ComputeShader_deformation);
			glerrorint=(int)glGetError();
			it->doPrecompute_perPoint(ComputeShader_deformation);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glerrorint=(int)glGetError();
			//it->showComputeResult();
			//�󶨵�1~6Ҫ����
			it->setupComputeData_bezierTriangle(ComputeShader_bezierTriangle);
			glerrorint=(int)glGetError();
			it->doPrecompute_perTriangle(ComputeShader_bezierTriangle);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glerrorint=(int)glGetError();
			//it->showComputeResult();
			//��ʼϸ��
			it->setupComputeData_tessellation(computeShader_tessellation);
			boundbox->setupComputeData_precomputeMatrix_tessellation();
			glerrorint=(int)glGetError();
			it->doPrecompute_perTriangle(computeShader_tessellation);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glerrorint=(int)glGetError();
			//��gpu�а󶨻�������
			//it->setupDrawData();
			//it->showComputeResult();
		}
	}
/***
 * �޸�mesh�����������ڵ������Ʒ�ʽ
 * @param lines_triangles false������lines��ʽ���ƣ�true����triangle��ʽ����
 */
	void changeMeshIndicesBuffer(bool lines_triangles)
    {
        for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)
            it->setupIndicesDrawData(lines_triangles);
    }

	void drawModel(const Shader& renderShader,bool lines_triangles)
	{		
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)//�Ժ�Ҫ���Ƹ�����ģ������Ҫһ��shader���飬��ǰ���Դ�������ݣ�ֻ��һ������ڵ�����û������
		{
			if(!show_originMesh_remesh){
				if (boundbox->isControlPointsNeedUpdate()) {//�����ƶ����Ѹ��£�������µļ���
					boundbox->updateControlPoints();
					it->doPrecompute_perPoint(ComputeShader_deformation);
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
					it->doPrecompute_perTriangle(ComputeShader_bezierTriangle);
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
					it->doPrecompute_perTriangle(computeShader_tessellation);
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				}
			}
			it->draw(renderShader,lines_triangles);
		}
	}
	void drawSelectedPoint(const Shader& shader)const {
		boundbox->drawSeletedPoint(shader);
	}
	void drawBoundbox(const Shader& shader) const
	{
		boundbox->draw(shader);	
	}

	void generateModel(std::vector<glm::vec2>& pointsVec,float y_factor){
#ifdef SHOW_ORIGINMESH
    show_originMesh_remesh= true;
#endif
		boundbox = new Boundbox();
		Mesh meshobj;
		std::vector<glm::vec3> vertexPos;
		const int rotateTimes=72;//��ת���ܹ�������ת�ߵĴ���
		const float PI=3.1415926f;

        for(int i=0;i<pointsVec.size();i++){
            pointsVec[i].y*=y_factor;
        }

		//��ʼ��������
        //������ͷβ�������㣨������ת�߹���)
		vertexPos.push_back(glm::vec3(0,pointsVec[2].y,0));
		vertexPos.push_back(glm::vec3(0,pointsVec[pointsVec.size()-1].y,0));
        //������ÿ����ת�߳���ͷ����ĵ�
        //Ϊ�˿���������������ʱ������
        float angle=0;
        float angle_space=2*PI/rotateTimes;
        int eachLine_pointNums_trimmed=(pointsVec.size()-4)/2;//ÿ����ת�߳�ȥͷβ���ɵĵ������
        //Ҫע��һ����Ļ��߱ȵ����⣬����x�ķ��Ⱥ�yʵ���ϲ�һ��
        for(int i=0;i<rotateTimes;i++){
        	for(int j=3;j<pointsVec.size()-1;j+=2){
        		glm::vec2 tempVec=pointsVec[j];
        		float x=std::cos(angle)*tempVec.x;
        		float y=tempVec.y;
				float z=std::sin(angle)*tempVec.x;
				vertexPos.push_back(glm::vec3(x,y,z));
        	}
			angle+=angle_space;
        }
        //��������
        //��������ϵ�һȦ
		for(int i=0;i<rotateTimes;i++){
			indices.push_back(2+((i+1)%rotateTimes)*eachLine_pointNums_trimmed);
			indices.push_back(2+(i%rotateTimes)*eachLine_pointNums_trimmed);
			indices.push_back(0);
		}
		//����������µ�һȦ
		for(int i=0;i<rotateTimes;i++){
			indices.push_back(1+eachLine_pointNums_trimmed+((i+1)%rotateTimes)*eachLine_pointNums_trimmed);
			indices.push_back(1);
			indices.push_back(1+eachLine_pointNums_trimmed+(i%rotateTimes)*eachLine_pointNums_trimmed);
		}
        //�����м���ı��Σ������ǻ�
		for(int i=0;i<rotateTimes;i++){
			for(int j=0;j<eachLine_pointNums_trimmed-1;j++){
				indices.push_back(3+j+((i+1)%rotateTimes)*eachLine_pointNums_trimmed);
				indices.push_back(3+j+(i%rotateTimes)*eachLine_pointNums_trimmed);
				indices.push_back(2+j+(i%rotateTimes)*eachLine_pointNums_trimmed);

				indices.push_back(2+j+(i%rotateTimes)*eachLine_pointNums_trimmed);
				indices.push_back(2+j+((i+1)%rotateTimes)*eachLine_pointNums_trimmed);
				indices.push_back(3+j+((i+1)%rotateTimes)*eachLine_pointNums_trimmed);
			}
		}
		LOGCATE("vertsize size=%d",vertexPos.size());
		LOGCATE("indices size=%d",indices.size());

		for(int i=0;i<vertexPos.size();i++){
			Vertex v=Vertex(glm::vec4(vertexPos[i],0.0f),glm::vec4(),glm::vec4());
			boundbox->updateBoundary(v);
			vertData.push_back(v);
		}
		//�Լ���λ�ý��е���
		float delta[3];
		delta[0] = (boundbox->min[0] + boundbox->max[0])*-0.5f;
		delta[1] = (boundbox->min[1] + boundbox->max[1])*-0.5f;
		delta[2] = (boundbox->min[2] + boundbox->max[2])*-0.5f;

		boundbox->min[0] += delta[0];
		boundbox->max[0] += delta[0];
		boundbox->min[1] += delta[1];
		boundbox->max[1] += delta[1];
		boundbox->min[2] += delta[2];
		boundbox->max[2] += delta[2];

		float maxXYZ=0;
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

		meshobj.setData(vertData, textures_loaded, indices);
		meshes.push_back(meshobj);

		boundbox->generateBspinebody();
        //+���Ҫ�ͷſռ�
        //std::vector<Vertex>().swap(vertData);
        //std::vector<int>().swap(indices);
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
		const aiScene* sceneObjPtr = importer.ReadFile(filePath, //aiProcess_Triangulate |aiProcess_FlipUVs|aiProcess_GenSmoothNormals
			aiProcess_Triangulate | aiProcess_JoinIdenticalVertices );//����Ҳ����ֱ����ԭ���������ɷ���
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
			
		return true;
	}

	~Model()
	{
		for (std::vector<Mesh>::const_iterator it = this->meshes.begin(); this->meshes.end() != it; ++it)
		{
			it->final();
		}
	}

	void Destroy()
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



	glm::vec3 GetAdjustModelPosVec()
	{
		glm::vec3 minXyz = glm::vec3(boundbox->min[0], boundbox->min[1], boundbox->min[2]);
		glm::vec3 maxXyz = glm::vec3(boundbox->max[0], boundbox->max[1], boundbox->max[2]);
		return (minXyz + maxXyz) / 2.0f;
	}
	float GetMaxViewDistance()
	{
		glm::vec3 minXyz = glm::vec3(boundbox->min[0], boundbox->min[1], boundbox->min[2]);
		glm::vec3 maxXyz = glm::vec3(boundbox->max[0], boundbox->max[1], boundbox->max[2]);
		glm::vec3 vec3 = (abs(minXyz) + abs(maxXyz)) / 2.0f;
		float maxDis = fmax(vec3.x, fmax(vec3.y, vec3.z));
		//LOGCATE("Model::GetMaxViewDistance maxDis=%f", maxDis);
		return maxDis;
	}
	Mesh* getMesh(int index){
		return &(meshes[index]);
	}
#ifdef SHOW_ORIGINMODEL
    void test_Draw(const Shader& shader){
		shader.use();
		glBindVertexArray(test_vao);
		GLint wireFrameboolLoc = glGetUniformLocation(shader.programId, "draw_wireframe");
		glUniform1i(wireFrameboolLoc, false);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glUseProgram(0);
	}

	void test_setUpBuffer(){
		glGenVertexArrays(1, &this->test_vao);
		glGenBuffers(1, &this->test_vbo);
		glGenBuffers(1, &this->test_ebo);
		glBindVertexArray(test_vao);
		glBindBuffer(GL_ARRAY_BUFFER, test_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)* vertData.size(),&vertData[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
							  sizeof(Vertex), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		// ������������
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
							  sizeof(Vertex), (GLvoid*)(4 * sizeof(GL_FLOAT)));
		glEnableVertexAttribArray(1);
		// ���㷨��������
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
							  sizeof(Vertex), (GLvoid*)(8 * sizeof(GL_FLOAT)));
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, test_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* indices.size(), &indices[0], GL_STATIC_DRAW);
		glBindVertexArray(0);
	}
#endif

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
			aiMaterial* material = sceneObjPtr->mMaterials[meshPtr->mMaterialIndex];

			// 1. diffuse maps
			std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			// 2. specular maps
			std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			// 3. normal maps
			std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
			// 4. height maps
			std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
			textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		}
		meshObj.setData(vertData, textures, indices);
		return meshPtr->mNumFaces;
	}

	// checks all material textures of a given type and loads the textures if they're not loaded yet.
	// the required info is returned as a Texture struct.
	std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName)
	{
		//DEBUG_LOGCATE();
		std::vector<Texture> textures;
		//LOGCATE("TextureFromFile type %s nums= %d", typeName.c_str(),mat->GetTextureCount(type));
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
			bool skip = false;
			for (unsigned int j = 0; j < textures_loaded.size(); j++)
			{
				if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
				{
					textures.push_back(textures_loaded[j]);
					skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
					break;
				}
			}
			if (!skip)
			{   // if texture hasn't been loaded already, load it
				Texture texture;
				texture.id = TextureFromFile(str.C_Str(), this->modelFileDir);
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
			}
		}
		return textures;
	}
	unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false)
	{
		std::string filename = std::string(path);
		filename = directory + '/' + filename;

		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char *data = nullptr;

		// load the texture using OpenCV
		LOGCATE("TextureFromFile Loading texture %s", filename.c_str());
		cv::Mat textureImage = cv::imread(filename);
		if (!textureImage.empty())
		{
			//hasTexture = true;
			// opencv reads textures in BGR format, change to RGB for GL
			cv::cvtColor(textureImage, textureImage, CV_BGR2RGB);
			// opencv reads image from top-left, while GL expects it from bottom-left
			// vertically flip the image
			//cv::flip(textureImage, textureImage, 0);

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureImage.cols,
				textureImage.rows, 0, GL_RGB, GL_UNSIGNED_BYTE,
				textureImage.data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			GO_CHECK_GL_ERROR();
		}
		else {
			LOGCATE("TextureFromFile Texture failed to load at path: %s", path);
		}

		return textureID;
	}


private:
	std::vector<Mesh> meshes; // ����Mesh���Ƕ�ȡ�ļ�����õ�����
	std::vector<Texture> textures_loaded;//�Ѷ�ȡ�Ĳ���
	std::string modelFileDir; // ����ģ���ļ����ļ���·��
	typedef std::map<std::string, Texture> LoadedTextMapType; // key = texture file path
	LoadedTextMapType loadedTextureMap; // �����Ѿ����ص�����
	int numOfPatches;//��Ƭ��

	std::vector<GLuint> indices;
	std::vector<Vertex> vertData;
	GLuint test_vao,test_vbo,test_ebo;

};

#endif