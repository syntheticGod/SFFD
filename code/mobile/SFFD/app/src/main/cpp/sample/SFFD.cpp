

#include "SFFD.h"
#include "GLUtils.h"
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <fstream>
#include <../util/LogUtil.h>
#include <algorithm>
#include <cmath>
float point_size = 15;
const float point_size_max = 40;
const float point_size_min = 5;
glm::vec3 lampPos(0.5f, 1.0f, 1.5f);



SFFD::SFFD()
{
	has_inited = false;
    ffd_dffd=false;
	controlPoint_NeedReSet=false;
	m_AngleX = 0;
	m_AngleY = 0;
	pickedPoint_z=0;
	camera_translationX=0;
	camera_translationY=0;

	point_select=SelectPoint();
	//不显示包围盒代表处于调整视图模式，反之为变形模式
	show_boundbox=false;
	//为false代表旋转调整视图，反之平移调整视图
	manipulation_rotate_translation=false;
	deformation_process=false;
	renderMode_lines_triangles=true;
	controlPoint_hasSelected=false;

	directPoint_hasSelected=false;
	selectedDirectPoint_index=-1;

}

SFFD::~SFFD()
{

}

//加载渲染着色器并读取模型
void SFFD::Init()
{
	if (has_inited)
		return;

	rotateLines_pointVec.push_back(glm::vec2(0,1));
	rotateLines_pointVec.push_back(glm::vec2(0,-1));
	glGenVertexArrays(1, &this->rotateLines_Vao);
	glGenBuffers(1, &this->rotateLines_Vbo);

	loadFile_cutScheme();
	//objModel.loadModel("/sdcard/model/heart/utahteapot.obj");
	//objModel.loadModel("/sdcard/model/heart/heart.obj");
	rotateLineShader.loadShadingFile("/sdcard/shader/rotateLines.vertex", "/sdcard/shader/rotateLines.frag");
	shader.loadShadingFile("/sdcard/shader/model.vertex", "/sdcard/shader/model_notex.frag");
	boundboxShader.loadShadingFile("/sdcard/shader/boundbox.vertex", "/sdcard/shader/boundbox.frag");
	selectedPointShader.loadShadingFile("/sdcard/shader/selectedPoint.vertex", "/sdcard/shader/selectedPoint.frag");

	selectedPointShader.use();
	glUniform1f(glGetUniformLocation(selectedPointShader.programId, "pointSize"), 35.0f);
	glUseProgram(0);

    objModel.loadComputeShader();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//常数参数设置
	shader.use();
	// 设置光源属性 点光源
	GLint lightAmbientLoc = glGetUniformLocation(shader.programId, "light.ambient");
	GLint lightDiffuseLoc = glGetUniformLocation(shader.programId, "light.diffuse");
	GLint lightSpecularLoc = glGetUniformLocation(shader.programId, "light.specular");
	GLint attConstant = glGetUniformLocation(shader.programId, "light.constant");
	GLint attLinear = glGetUniformLocation(shader.programId, "light.linear");
	GLint attQuadratic = glGetUniformLocation(shader.programId, "light.quadratic");
	glUniform3f(lightAmbientLoc, 0.5f, 0.5f, 0.5f);
	glUniform3f(lightDiffuseLoc, 0.8f, 0.8f, 0.8f);
	glUniform3f(lightSpecularLoc, 1.0f, 1.0f, 1.0f);

	// 设置衰减系数
	glUniform1f(attConstant, 1.0f);
	glUniform1f(attLinear, 0.0f);
	glUniform1f(attQuadratic, 0.0f);
	glUseProgram(0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glPointSize(point_size);

	//后面光源和shader改成新的，以视点为光源
	/*

	float xAxisLength=objModel.boundbox->max[0]-objModel.boundbox->min[0];
	float yAxisLength=objModel.boundbox->max[1]-objModel.boundbox->min[1];
    float zAxisLength=objModel.boundbox->max[2]-objModel.boundbox->min[2];
	lampPos=glm::vec3(objModel.boundbox->max[0]+xAxisLength*2,objModel.boundbox->max[1]+yAxisLength*2,objModel.boundbox->min[2]-zAxisLength*2);
	*/
	camera_translationZ=3.0f;
	has_inited = true;

	LOGCATE("SFFD init complete");
}

void SFFD::LoadImage(NativeImage *pImage)
{
}

void SFFD::Draw(int screenW, int screenH)
{

	if (!has_inited)
		return;

    WINDOW_WIDTH=screenW;
    WINDOW_HEIGHT=screenH;
    //LOGCATE("WINDOW_WIDTH=%d,WINDOW_HEIGHT=%d",WINDOW_WIDTH,WINDOW_HEIGHT);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(!setLines_deformation){//处理旋转线
		glBindVertexArray(rotateLines_Vao);
		glBindBuffer(GL_ARRAY_BUFFER, rotateLines_Vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)* rotateLines_pointVec.size(),&rotateLines_pointVec[0], GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,sizeof(glm::vec2), (GLvoid*)0);
		glEnableVertexAttribArray(0);

		rotateLineShader.use();
		glLineWidth(6.0f);
		glDrawArrays(GL_LINES, 0, rotateLines_pointVec.size());
		glBindVertexArray(0);
		glUseProgram(0);
		return;
	}



	UpdateMVPMatrix_new( m_AngleX, m_AngleY, (float)screenW / screenH);

    shader.use();
    GLint vpl = glGetUniformLocation(shader.programId, "viewPos");
    glUniform3f(vpl, 0.0f, 0.0f, 1.0f);
    lampPos = glm::vec3(camera_translationX,camera_translationY,camera_translationZ);//注意现在的代码里没有camera
    GLint lightPosLoc = glGetUniformLocation(shader.programId, "light.position");
    glUniform3f(lightPosLoc, lampPos.x, lampPos.y, lampPos.z);

    glUniformMatrix4fv(glGetUniformLocation(shader.programId, "projection"), 1, GL_FALSE, glm::value_ptr(mat_Projection));
    glUniformMatrix4fv(glGetUniformLocation(shader.programId, "view"), 1, GL_FALSE, glm::value_ptr(mat_View));
    glUniformMatrix4fv(glGetUniformLocation(shader.programId, "model"),
                       1, GL_FALSE, glm::value_ptr(mat_Model));
    glUseProgram(0);
#ifdef SHOW_ORIGINMODEL
	if(!testDraw_hassetup){
		objModel.test_setUpBuffer();
		testDraw_hassetup=true;
	}
	objModel.test_Draw(shader);
	LOGCATE("test_Draw");
	return;
#endif

    if(objModel.show_originMesh_remesh){
        if(!originMesh_remesh_hasSetup){
            objModel.setupDrawData();
			originMesh_remesh_hasSetup=true;
        }
        objModel.drawModel(shader,true);
        return;
    }

    if(!model_hascompute){
        objModel.setComputeData();
        model_hascompute=true;
    }
     if(!mesh_hasSetup){
		 objModel.setupDrawData();
		 mesh_hasSetup=true;
     }
     objModel.drawModel(shader,renderMode_lines_triangles);

	if(show_boundbox){
		if(!bsplinebody_hasSetup){
			objModel.boundbox->bspinebody->setupBuffer();
			bsplinebody_hasSetup=true;
		}
		else{
			if(controlPoint_NeedReSet){
				controlPoint_NeedReSet=false;
				objModel.boundbox->bspinebody->reSetupBuffer();
			}
		}
		boundboxShader.use();
		glUniformMatrix4fv(glGetUniformLocation(boundboxShader.programId, "projection"), 1, GL_FALSE, glm::value_ptr(mat_Projection));
		glUniformMatrix4fv(glGetUniformLocation(boundboxShader.programId, "view"), 1, GL_FALSE, glm::value_ptr(mat_View));
		glUniformMatrix4fv(glGetUniformLocation(boundboxShader.programId, "model"),
						   1, GL_FALSE, glm::value_ptr(mat_Model));
		GLint colorLoc = glGetUniformLocation(boundboxShader.programId, "colorInput");
		glUniform3f(colorLoc, 65, 105, 225);
		objModel.drawBoundbox(boundboxShader);
		glBindVertexArray(0);
		glUseProgram(0);
	}
    if(ffd_dffd){
    	if(directPointsHasSetup){
			objModel.boundbox->setupSeletedPointBuffer();
			directPointsHasSetup=false;
    	}
        LOGCATE("Direct Point nums=%d",objModel.boundbox->directPoints.size());
		selectedPointShader.use();
		glUniformMatrix4fv(glGetUniformLocation(selectedPointShader.programId, "projection"), 1, GL_FALSE, glm::value_ptr(mat_Projection));
		glUniformMatrix4fv(glGetUniformLocation(selectedPointShader.programId, "view"), 1, GL_FALSE, glm::value_ptr(mat_View));
		glUniformMatrix4fv(glGetUniformLocation(selectedPointShader.programId, "model"),
						   1, GL_FALSE, glm::value_ptr(mat_Model));
		objModel.drawSelectedPoint(selectedPointShader);
		glBindVertexArray(0);
		glUseProgram(0);
    }


	glBindVertexArray(0);
	glUseProgram(0);
	//LOGCATE("Draw");
}

void SFFD::Destroy()
{
    LOGCATE("SFFD::Destroy");
	objModel.Destroy();

}
//在已有数据的基础上计算mvp矩阵
void SFFD::UpdateMVPMatrix_new(int angleX, int angleY, float ratio)
{
	//LOGCATE("SFFD::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX, angleY, ratio);
	angleX = angleX % 360;
	angleY = angleY % 360;

	//转化为弧度角
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);


	// Projection matrix
	//glm::mat4 Projection = glm::ortho(-ratio, ratio, -1.0f, 1.0f, 0.1f, 100.0f);
	//mat_Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 1.0f, objModel.GetMaxViewDistance() * 4);
	//glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);
	//mat_Projection = glm::perspective(glm::radians(60.0f),ratio, 1.0f,100.f);
	mat_Projection = glm::perspective(glm::radians(45.0f),ratio, 1.0f,100.f);
	// View matrix
	mat_View = glm::lookAt(
			glm::vec3(camera_translationX, camera_translationY, camera_translationZ), // Camera pos
			glm::vec3(camera_translationX, camera_translationY, -100.0f), // and looks at the origin
			glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	// Model matrix
	mat_Model = glm::mat4(1.0f);
	mat_Model = glm::scale(mat_Model, glm::vec3(1.0f, 1.0f, 1.0f));
	mat_Model = glm::rotate(mat_Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
	mat_Model = glm::rotate(mat_Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
	//mat_Model = glm::translate(mat_Model, objModel.GetAdjustModelPosVec()+glm::vec3(camera_translationX,camera_translationY,0));
	//mat_Model = glm::translate(mat_Model, glm::vec3(0,0,0));

}

//不同模式参数代表不同含义，变形模式下后两参数代表是否移动/是否选中
void SFFD::UpdateTransformMatrix(float offsetX, float offsetY, float scaleX, float scaleY)
{
	if(!setLines_deformation){
		if(scaleX==0&&scaleY==0)//刚开始加点
			rotateLines_pointVec.push_back(glm::vec2(offsetX,offsetY));
		else{
			glm::vec2 tempEnd=rotateLines_pointVec[rotateLines_pointVec.size()-1];
			rotateLines_pointVec.push_back(tempEnd);
			rotateLines_pointVec.push_back(glm::vec2(offsetX,offsetY));
		}
		LOGCATE("旋转线vector大小=%d",rotateLines_pointVec.size());
		return;
	}


	if(!deformation_process){
		//GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
		if(!manipulation_rotate_translation){
			m_AngleX = static_cast<int>(offsetX);
			m_AngleY = static_cast<int>(offsetY);
		}
		else{
			camera_translationX=static_cast<float>(offsetX);
			camera_translationY = static_cast<float>(offsetY);
		}
		camera_translationZ+= scaleX;
	}
    else{//变形过程，对控制节点进行操作
        if(!ffd_dffd){//传统变形方式
            if(show_boundbox){
                if(scaleX==0){//控制顶点选择操作
                    if(scaleY==0){//手指放弃控制顶点
                        controlPoint_hasSelected=false;
#ifdef PERSIST_RENDING
						objModel.boundbox->bspinebody->controlPoints_needUpdate = true;
#endif
                    }
                    else{
                        glm::vec3 touch_position_near=Get3Dpos(offsetX,offsetY,1,mat_Model, mat_View, mat_Projection,WINDOW_WIDTH,WINDOW_HEIGHT);
                        glm::vec3 touch_position_far=Get3Dpos(offsetX,offsetY,-1,mat_Model, mat_View, mat_Projection,WINDOW_WIDTH,WINDOW_HEIGHT);
                        //特殊情况，平行光线(z轴)
                        if(touch_position_near.x==touch_position_far.x&&touch_position_near.y==touch_position_far.y)
                            controlPoint_hasSelected=false;
                        else{
                            if(getNearestPoint_AmongControPoints(touch_position_near, touch_position_far,objModel.boundbox->bspinebody,point_select)){
                                controlPoint_hasSelected=true;
                                //计算裁剪空间的深度，供后面变形挑取位移使用
                                glm::vec4 clip_position=mat_Projection*mat_View*mat_Model*glm::vec4(point_select.pos,1.0f);
                                if(clip_position.w!=0)
                                    clip_position=clip_position/clip_position.w;
                                pickedPoint_z=clip_position.z;
                                LOGCATE("pickedPoint_z=%f",pickedPoint_z);
                            }

                            else//触点距离可用区域太远，找不到适合的目标点
                                controlPoint_hasSelected=false;
                        }
                    }
                }
                else{//控制顶点位移操作
                    if(controlPoint_hasSelected){
                        directPoint_hasSelected=true;
                        glm::vec3 touch_position=Get3Dpos(offsetX,offsetY,pickedPoint_z,mat_Model, mat_View, mat_Projection,WINDOW_WIDTH,WINDOW_HEIGHT);
                        //LOGCATE("touch_position=%f,%f,%f",touch_position.x,touch_position.y,touch_position.z);
                        //LOGCATE("point_select index=%d,%d,%d",point_select.index_x,point_select.index_y,point_select.index_z);
                        objModel.boundbox->bspinebody->controlPoints[point_select.index_x][point_select.index_y][point_select.index_z].x = touch_position.x;
                        objModel.boundbox->bspinebody->controlPoints[point_select.index_x][point_select.index_y][point_select.index_z].y = touch_position.y;
                        objModel.boundbox->bspinebody->controlPoints[point_select.index_x][point_select.index_y][point_select.index_z].z = touch_position.z;
                        controlPoint_NeedReSet=true;
#ifndef PERSIST_RENDING
						objModel.boundbox->bspinebody->controlPoints_needUpdate = true;
#endif
                    }
                }
            }
        }
        else{//dffd变形方式
            if(scaleX==0){//控制顶点选择操作
                glm::vec3 touch_position_near=Get3Dpos(offsetX,offsetY,1,mat_Model, mat_View, mat_Projection,WINDOW_WIDTH,WINDOW_HEIGHT);
                glm::vec3 touch_position_far=Get3Dpos(offsetX,offsetY,-1,mat_Model, mat_View, mat_Projection,WINDOW_WIDTH,WINDOW_HEIGHT);
                //特殊情况，平行光线(z轴)
                if(touch_position_near.x==touch_position_far.x&&touch_position_near.y==touch_position_far.y)
                    return;

                if(scaleY==0){//单击（选择已有点）
					SelectPoint tempPoint=SelectPoint();
					if(!getNearestPoint_AmongDirectPoints(touch_position_near, touch_position_far,objModel.boundbox->directPoints,tempPoint))
						return;

                    std::vector<SelectPoint>::iterator it;
                    it=std::find(objModel.boundbox->directPoints.begin(),objModel.boundbox->directPoints.end(),tempPoint);
					selectedDirectPoint_index=it-objModel.boundbox->directPoints.begin();

					glm::vec4 clip_position=mat_Projection*mat_View*mat_Model*glm::vec4(tempPoint.pos,1.0f);
					if(clip_position.w!=0)
						clip_position=clip_position/clip_position.w;
					pickedPoint_z=clip_position.z;
					LOGCATE("单击,selectedDirectPoint_index=%d",selectedDirectPoint_index);
					hasSingle_taped=true;
                }
                else{//双击（添加删除新点）
                	if(directPoint_hasSelected)
                		return;

                	SelectPoint tempPoint=SelectPoint();
                    if(!getNearestPoint_AmongOriginVerts(touch_position_near, touch_position_far,objModel.getMesh(0),tempPoint,mat_Model,camera_translationX,camera_translationY,camera_translationZ))
                    	return;

                    int directIndex=0;
                    float directDistance_min=1000;
                    bool isDeleteDirectPointOperation=false;
                    for(int i=0;i<objModel.boundbox->directPoints.size();i++){
                    	glm::vec3 tempPos=objModel.boundbox->directPoints[i].pos;
                    	float tempDistance=pow(tempPoint.pos.x-tempPos.x,2)+pow(tempPoint.pos.y-tempPos.y,2)+pow(tempPoint.pos.z-tempPos.z,2);
                    	if(tempDistance<directDistance_min){
							directDistance_min=tempDistance;
							directIndex=i;
                    	}
                    }
                    if(directDistance_min<MIN_DIRECTPOINTS_SPACE)
						isDeleteDirectPointOperation=true;

					//LOGCATE("new dffd point pos=%f,%f,%f",tempPoint.pos.x,tempPoint.pos.y,tempPoint.pos.z);
					if (isDeleteDirectPointOperation){//已存在，则删除
						objModel.boundbox->deleteDffdSelectedPoint(directIndex);
						directPointsHasSetup=true;
					}
					else{
						if(objModel.boundbox->directPoints.size()<MAX_DIRECTPOINTS_NUMS){//尚未满，可以添加新的直接编辑点
							objModel.boundbox->addDffdSelectedPoint(tempPoint);
							directPointsHasSetup=true;
						}
					}
					LOGCATE("size=%d,origin size=%d",objModel.boundbox->directPoints.size(),objModel.boundbox->origin_directPoints.size());
                }
            }
            else{
            	if(scaleY==0){//已开始移动，停止添加删除直接操作
					if(hasSingle_taped){
						directPoint_hasSelected=true;
						glm::vec3 touch_position=Get3Dpos(offsetX,offsetY,pickedPoint_z,mat_Model, mat_View, mat_Projection,WINDOW_WIDTH,WINDOW_HEIGHT);
						glm::vec3 movement=touch_position-objModel.boundbox->directPoints[selectedDirectPoint_index].pos;
						objModel.boundbox->dffd_adjustControlPoints(selectedDirectPoint_index,movement.x,movement.y,movement.z);
						directPointsHasSetup=true;
						controlPoint_NeedReSet=true;
#ifndef DFFD_PERSIST_RENDING
						objModel.boundbox->bspinebody->controlPoints_needUpdate = true;
#endif
						//LOGCATE("dffd位移=%f,%f,%f",movement.x,movement.y,movement.z);
					}
            	}
            	else{//抬起手放开
					hasSingle_taped=false;
#ifdef DFFD_PERSIST_RENDING
					if(controlPoint_NeedReSet)
					    objModel.boundbox->bspinebody->controlPoints_needUpdate = true;
#endif
            	}
            }
        }

    }
}

void SFFD::loadFile_cutScheme() {
	CutScheme::init();
	std::ifstream infilestream;
	
	int index = 0;
	for (int i = 0; i < CutScheme::MAX_CUTLENGTH; i++) {
		for (int j = 0; j < CutScheme::MAX_CUTLENGTH; j++) {
			for (int k = 0; k < CutScheme::MAX_CUTLENGTH; k++) {
				//CutScheme::nums_trianglesAccumlation_cutScheme.push_back(total);
				if (!(i + j + 1<k || i + k + 1<j || k + j + 1<i)) {
					infilestream.open("/sdcard/cutOutputFile/" + std::to_string(i + 1) + "_" + std::to_string(j + 1) + "_" + std::to_string(k + 1) + ".txt");
					int nums_vertexs;
					infilestream >> nums_vertexs;
					DelaunayResult temp = DelaunayResult();

					for (int x = 0; x < nums_vertexs; x++) {
						float tempX, tempY, tempZ;
						infilestream >> tempX;
						infilestream >> tempY;
						infilestream >> tempZ;//一个质心坐标
						temp.vector_positions.push_back(glm::vec3(tempX, tempY, tempZ));
					}
					
					int tempInd;
					while (infilestream >> tempInd) {
						temp.vector_indices.push_back(tempInd);
					}
					CutScheme::cutScheme.push_back(temp);
					infilestream.close();
				}
				index++;
			}
		}
	}
}

/***
 *
 * @param systemMode 低位到高位
 * @param value0 是否处于变形过程,1代表是
 * @param value1 绘制模式，1代表三角形
 */
void SFFD::SetParamsInt(int systemMode, int value0, int value1){
	if(!has_inited)
	  return;

	int tempint=0;
    tempint=systemMode/1000;
    setLines_deformation=(tempint==1?true:false);
	systemMode=systemMode-1000*tempint;
    tempint=systemMode/100;
	ffd_dffd=(tempint==1?true:false);
    systemMode=systemMode-100*tempint;

	deformation_process=(systemMode%10==1? true:false);
	manipulation_rotate_translation=(systemMode/10==1? true:false);

	renderMode_lines_triangles=(value1==1);
	show_boundbox=(value0==1);
	objModel.changeMeshIndicesBuffer(renderMode_lines_triangles);

	//重置旋转面曲线相关数据
	std::vector<glm::vec2>().swap(rotateLines_pointVec);
	rotateLines_pointVec.push_back(glm::vec2(0,1));
	rotateLines_pointVec.push_back(glm::vec2(0,-1));
}

void SFFD::recoverConTrolPoints() {
	if(!setLines_deformation){//设置旋转线阶段
        //开始生成模型，替代原先loadmodel的工作，并进行remesh（CPU）
		objModel.generateModel(rotateLines_pointVec,(float)WINDOW_HEIGHT/(float)WINDOW_WIDTH);
        //开始SFFD
		//objModel.setComputeData();
		//释放资源
		std::vector<glm::vec2>().swap(rotateLines_pointVec);
		glDeleteBuffers(1,&this->rotateLines_Vbo);
		setLines_deformation=true;
	}
	else{
		objModel.boundbox->bspinebody->recoverControlPoints();
		controlPoint_NeedReSet=true;
		objModel.boundbox->bspinebody->controlPoints_needUpdate = true;

		for(int i=0;i<objModel.boundbox->directPoints.size();i++)
			objModel.boundbox->directPoints[i]=objModel.boundbox->origin_directPoints[i];
		directPoint_hasSelected= false;
		directPointsHasSetup=true;
	}

	LOGCATE("recoverConTrolPoints!");
}

