//
// Created by ByteFlow on 2019/7/30.
//

#ifndef NDK_OPENGLES_3_0_MODEL3DSample_H
#define NDK_OPENGLES_3_0_MODEL3DSample_H
#define MAX_DIRECTPOINTS_NUMS 4
//直接编辑点最小的间距，用于判断是否为删除点操作
#define MIN_DIRECTPOINTS_SPACE 0.1
//dffd模式下持续绘制
#define DFFD_PERSIST_RENDING
#define PERSIST_RENDING

#include <../glm/detail/type_mat.hpp>
#include <../glm/detail/type_mat4x4.hpp>
#include "../model/shader.h"
#include "../model/model.h"
#include "GLSampleBase.h"
#include "mouse.h"
#include <vector>



class SFFD : public GLSampleBase
{
public:
	SFFD();

	virtual ~SFFD();

	virtual void LoadImage(NativeImage *pImage);

	virtual void Init();
	virtual void Draw(int screenW, int screenH);

	virtual void Destroy();

	virtual void UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY);

	virtual void SetParamsInt(int systemMode, int value0, int value1);

	void UpdateMVPMatrix_new(int angleX, int angleY, float ratio);//不继承原有的方法

    virtual void recoverConTrolPoints();

private:


	int m_AngleX;
	int m_AngleY;
	float camera_translationZ;
	float camera_translationX;
	float camera_translationY;
	bool has_inited;
	bool controlPoint_hasSelected;
    bool directPointsHasSetup=false;
    bool needResetdirectPointsBuffer=false;

	//直接编辑点的保存及状态（总个数，已选择的下标等）
	bool directPoint_hasSelected;//已选择直接编辑点开始变形（点开始移动），停止添加删除直接编辑点
    int selectedDirectPoint_index;//当前选择的直接编辑点下标，范围在0`MAX_DIRECTPOINTS_NUMS-1之间
    bool hasSingle_taped=false;//为了避免操作过快导致未单击直接位移的情况发生

	bool controlPoint_NeedReSet;
    //采用的变形模式,左false右true
	bool ffd_dffd;
	bool setLines_deformation=false;
	glm::mat4 mat_Model, mat_View, mat_Projection;
	Model objModel;
	Shader rotateLineShader,shader,boundboxShader,selectedPointShader;
	void loadFile_cutScheme();

	//为true则在调整视图（自动强制关闭变形体），反之在变形过程（自动强制显示变形体）
    bool deformation_process,renderMode_lines_triangles,show_boundbox,manipulation_rotate_translation;
    //为false代表旋转调整视图，反之平移调整视图
    GLint WINDOW_WIDTH,WINDOW_HEIGHT;
    SelectPoint point_select;
    float pickedPoint_z;

    //旋转线相关数据
    std::vector<glm::vec2> rotateLines_pointVec;
	GLuint rotateLines_Vao,rotateLines_Vbo;
	bool testDraw_hassetup=false;

	//绘制标识符
	bool originMesh_remesh_hasSetup=false;
	bool mesh_hasSetup=false;
	bool model_hascompute=false;
	bool bsplinebody_hasSetup=false;
};


#endif //NDK_OPENGLES_3_0_MODEL3DSample_H
