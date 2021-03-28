//
// Created by ByteFlow on 2019/7/30.
//

#ifndef NDK_OPENGLES_3_0_MODEL3DSample_H
#define NDK_OPENGLES_3_0_MODEL3DSample_H
#define MAX_DIRECTPOINTS_NUMS 4
//ֱ�ӱ༭����С�ļ�࣬�����ж��Ƿ�Ϊɾ�������
#define MIN_DIRECTPOINTS_SPACE 0.1
//dffdģʽ�³�������
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

	void UpdateMVPMatrix_new(int angleX, int angleY, float ratio);//���̳�ԭ�еķ���

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

	//ֱ�ӱ༭��ı��漰״̬���ܸ�������ѡ����±�ȣ�
	bool directPoint_hasSelected;//��ѡ��ֱ�ӱ༭�㿪ʼ���Σ��㿪ʼ�ƶ�����ֹͣ���ɾ��ֱ�ӱ༭��
    int selectedDirectPoint_index;//��ǰѡ���ֱ�ӱ༭���±꣬��Χ��0`MAX_DIRECTPOINTS_NUMS-1֮��
    bool hasSingle_taped=false;//Ϊ�˱���������쵼��δ����ֱ��λ�Ƶ��������

	bool controlPoint_NeedReSet;
    //���õı���ģʽ,��false��true
	bool ffd_dffd;
	bool setLines_deformation=false;
	glm::mat4 mat_Model, mat_View, mat_Projection;
	Model objModel;
	Shader rotateLineShader,shader,boundboxShader,selectedPointShader;
	void loadFile_cutScheme();

	//Ϊtrue���ڵ�����ͼ���Զ�ǿ�ƹرձ����壩����֮�ڱ��ι��̣��Զ�ǿ����ʾ�����壩
    bool deformation_process,renderMode_lines_triangles,show_boundbox,manipulation_rotate_translation;
    //Ϊfalse������ת������ͼ����֮ƽ�Ƶ�����ͼ
    GLint WINDOW_WIDTH,WINDOW_HEIGHT;
    SelectPoint point_select;
    float pickedPoint_z;

    //��ת���������
    std::vector<glm::vec2> rotateLines_pointVec;
	GLuint rotateLines_Vao,rotateLines_Vbo;
	bool testDraw_hassetup=false;

	//���Ʊ�ʶ��
	bool originMesh_remesh_hasSetup=false;
	bool mesh_hasSetup=false;
	bool model_hascompute=false;
	bool bsplinebody_hasSetup=false;
};


#endif //NDK_OPENGLES_3_0_MODEL3DSample_H
