#pragma once
#include <GLEW/glew.h>
#include "mesh.h"

class Bspinebody;

///包围盒和B样条体
class Boundbox
{
	friend class Model;
public:
	Boundbox();

	void outputControlPoints();
	///更新包围盒边界
	void updateBoundary(Vertex &vertex); 
	int generateBspinebody();
	int generateBspinebody(unsigned int cx, unsigned int cy, unsigned int cz, unsigned int dx, unsigned int dy, unsigned int dz);//控制点数，次数
	void draw(const Shader& shader) const;
	GLfloat min[3], max[3];
	bool hasInit;
	Bspinebody* bspinebody;
	Vertex selectedPoint;
	Vertex selectedPoint_original;
	glm::vec3 selectedPointPos_afterDeformation;

	void setupComputeData_deformation(const Shader& shader);
	void setupComputeData_bsplineBase(const Shader& shader);
	void setupComputeData_bsplinebody();
    //void setupBsplineUniform_test(const Shader& shader);
	void setupUniform_BsplineCompute(const Shader& shader);
	//重新计算并向gpu传输控制顶点与差分控制顶点
	void updateControlPoints();
	void setupComputeData_precomputeMatrix_triangleControlPoints();
	void setupComputeData_precomputeMatrix_tessellation();
	int getTotalControlPointNums();
	//设置选择点并计算B样条基
	void setSelectedPoint(Vertex& v);
	void setupSeletedPointBuffer();
	void drawSeletedPoint(const Shader& shader);

	glm::vec4 base[3][100];
	glm::vec4 base_low[3][100];
	//测试用
	int seg_nums[3];
	int calSeg(int dimention, float t);
	float calBase_X(int point_index, float t);
	float calBase_Y(int point_index, float t);
	float calBase_Z(int point_index, float t);
	float calBase_X_derivative(int point_index, float t);
	float calBase_Y_derivative(int point_index, float t);
	float calBase_Z_derivative(int point_index, float t);
	float cal_determinant(float x1, float y1, float x2, float y2) {//依列排
		return x1*y2 - y1*x2;
	}
	//传统FFD求值
	glm::vec3 test_calPositionAfterDeformation_FFD(glm::vec3 position_pragma);
	glm::vec3 test_calNormalAfterDeformation(glm::vec3 position_pragma, glm::vec3 origin_normal);
	//控制点是否需要更新（重新计算）
	bool isControlPointsNeedUpdate();
	//根据给定的单点参数坐标和位移，计算调整控制顶点位置，同时计算选择顶点的新位置。位移实际为vec3便于padding调整为vec4
	void dffd_adjustControlPoints(float movement_x, float movement_y, float movement_z);
	void test_ffd_calBase();
private:
	GLuint controlPointSSbo, controlPoint_derivative_SSbo[3], baseSSbo[3],baseSSBO_low[3],knotVectorSSBO[3],precomputeMatrix_triangleControlPointsSSbo, precomputeMatrix_tessellationSSbo,
		selectedPointVao, selectedPointVbo;
	float parameter_x, parameter_y, parameter_z;
	float point_BsplineBase[3][10];
	float total_down;
};




