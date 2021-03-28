#pragma once
#include "BsplineBase.h"
class Boundbox;

///各维度最大支持10个控制点
//最大次数5次
class Bspinebody
{
	friend class Boundbox;
public:

	bool controlPoints_needUpdate;
	unsigned int controlPointNums[3];
	glm::vec4 controlPoints[10][10][10];//x,y,z
	glm::vec4 controlPoints_original[10][10][10];//原始的控制顶点，用于复位
	std::vector<glm::vec4> controlPoints_derivative_x;//x维度少一个，属于控制顶点配合节点向量的组合运算结果,排放的顺序按照对应的维度成面扫过去
	std::vector<glm::vec4> controlPoints_derivative_y;//y维度少一个，属于控制顶点配合节点向量的组合运算结果
	std::vector<glm::vec4> controlPoints_derivative_z;//z维度少一个，属于控制顶点配合节点向量的组合运算结果
	void setupBuffer();
	void reSetupBuffer();
	int num_of_patches;//分割后的面片数
	void recoverControlPoints();//复位控制顶点至原始状态
	
private:
	Bspinebody(); 
	Bspinebody(unsigned int cx, unsigned int cy, unsigned int cz, unsigned int dx, unsigned int dy, unsigned int dz); 

	void draw(const Shader& shader) const;
	int InitBspinebody(Boundbox* box); //长方体
	void cal_controlPoints_derivative();//根据已有的控制顶点和节点向量计算组合控制顶点（雅可比矩阵用）

	BsplineBase base[3];
	unsigned int degree[3];
	float controlPoints_singleDimention[3][10];//0基，存放对应维度下下标对应坐标
	
	//glm::vec3 controlPoints_localCordinate[10][10][10];//x,y,z
	bool is_valid;//是否合法
	bool has_init;
	float knotVector[3][16];//10+5+1

	GLuint VAOId[2];
	GLuint VBOId;
	GLuint EBOId[2];
	std::vector<GLuint> indices;
	std::vector<GLuint> indices_vectex;//顶点的索引表
	void adjust();//调整控制顶点位置
	
};

