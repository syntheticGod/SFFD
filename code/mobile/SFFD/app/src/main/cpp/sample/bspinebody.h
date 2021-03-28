#pragma once
#include "BsplineBase.h"
class Boundbox;

///��ά�����֧��10�����Ƶ�
//������5��
class Bspinebody
{
	friend class Boundbox;
public:

	bool controlPoints_needUpdate;
	unsigned int controlPointNums[3];
	glm::vec4 controlPoints[10][10][10];//x,y,z
	glm::vec4 controlPoints_original[10][10][10];//ԭʼ�Ŀ��ƶ��㣬���ڸ�λ
	std::vector<glm::vec4> controlPoints_derivative_x;//xά����һ�������ڿ��ƶ�����Ͻڵ����������������,�ŷŵ�˳���ն�Ӧ��ά�ȳ���ɨ��ȥ
	std::vector<glm::vec4> controlPoints_derivative_y;//yά����һ�������ڿ��ƶ�����Ͻڵ����������������
	std::vector<glm::vec4> controlPoints_derivative_z;//zά����һ�������ڿ��ƶ�����Ͻڵ����������������
	void setupBuffer();
	void reSetupBuffer();
	int num_of_patches;//�ָ�����Ƭ��
	void recoverControlPoints();//��λ���ƶ�����ԭʼ״̬
	
private:
	Bspinebody(); 
	Bspinebody(unsigned int cx, unsigned int cy, unsigned int cz, unsigned int dx, unsigned int dy, unsigned int dz); 

	void draw(const Shader& shader) const;
	int InitBspinebody(Boundbox* box); //������
	void cal_controlPoints_derivative();//�������еĿ��ƶ���ͽڵ�����������Ͽ��ƶ��㣨�ſɱȾ����ã�

	BsplineBase base[3];
	unsigned int degree[3];
	float controlPoints_singleDimention[3][10];//0������Ŷ�Ӧά�����±��Ӧ����
	
	//glm::vec3 controlPoints_localCordinate[10][10][10];//x,y,z
	bool is_valid;//�Ƿ�Ϸ�
	bool has_init;
	float knotVector[3][16];//10+5+1

	GLuint VAOId[2];
	GLuint VBOId;
	GLuint EBOId[2];
	std::vector<GLuint> indices;
	std::vector<GLuint> indices_vectex;//�����������
	void adjust();//�������ƶ���λ��
	
};

