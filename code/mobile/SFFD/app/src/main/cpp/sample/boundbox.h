#pragma once
#include <GLES3/gl31.h>
#include "../model/mesh.h"
#include "mouse.h"
class Bspinebody;


struct DffdSelectedPoint_Data{
    float point_BsplineBase[3][10];
    float total_down;
};

///��Χ�к�B������
class Boundbox
{
	friend class Model;
public:
	Boundbox();

	void outputControlPoints();
	///���°�Χ�б߽�
	void updateBoundary(Vertex &vertex); 
	int generateBspinebody();
	int generateBspinebody(unsigned int cx, unsigned int cy, unsigned int cz, unsigned int dx, unsigned int dy, unsigned int dz);//���Ƶ���������
	void draw(const Shader& shader) const;
	GLfloat min[3], max[3];
	bool hasInit;
	Bspinebody* bspinebody;
	glm::vec3 selectedPointPos_afterDeformation;

	void setupComputeData_deformation(const Shader& shader);
	void setupComputeData_bsplineBase(const Shader& shader);
	void setupComputeData_bsplinebody();
    //void setupBsplineUniform_test(const Shader& shader);
	void setupUniform_BsplineCompute(const Shader& shader);
	//���¼��㲢��gpu������ƶ������ֿ��ƶ���
	void updateControlPoints();
	void setupComputeData_precomputeMatrix_triangleControlPoints();
	void setupComputeData_precomputeMatrix_tessellation();
	int getTotalControlPointNums();

	glm::vec4 base[3][100];
	glm::vec4 base_low[3][100];
	//������
	int seg_nums[3];
	int calSeg(int dimention, float t);
	float calBase_X(int point_index, float t);
	float calBase_Y(int point_index, float t);
	float calBase_Z(int point_index, float t);
	float calBase_X_derivative(int point_index, float t);
	float calBase_Y_derivative(int point_index, float t);
	float calBase_Z_derivative(int point_index, float t);
	float cal_determinant(float x1, float y1, float x2, float y2) {//������
		return x1*y2 - y1*x2;
	}
	glm::vec3 test_calNormalAfterDeformation(glm::vec3 position_pragma, glm::vec3 origin_normal);
	//���Ƶ��Ƿ���Ҫ���£����¼��㣩
	bool isControlPointsNeedUpdate();
	//���ݸ����ĵ�����������λ�ƣ�����������ƶ���λ�ã�ͬʱ����ѡ�񶥵����λ�á�λ��ʵ��Ϊvec3����padding����Ϊvec4

	//�µ�dffdʹ�����
	std::vector<SelectPoint> directPoints;//�������н�ȡ��SelectPoint�ṹ���X��������������ԭmesh��vertData���±�
    std::vector<SelectPoint> origin_directPoints;

    void addDffdSelectedPoint(SelectPoint);
    void deleteDffdSelectedPoint(int index);
    void dffd_adjustControlPoints(int index,float movement_x, float movement_y, float movement_z);
    void setupSeletedPointBuffer();
    void reSetupSeletedPointBuffer();
    void drawSeletedPoint(const Shader& shader);

private:
	GLuint controlPointSSbo[1];
    GLuint controlPoint_derivative_SSbo[3][1];
    GLuint baseSSbo[3][1];
    GLuint baseSSBO_low[3][1];
    GLuint knotVectorSSBO[3][1];
    GLuint precomputeMatrix_triangleControlPointsSSbo[1];
    GLuint precomputeMatrix_tessellationSSbo[1];
	GLuint selectedPointVao, selectedPointVbo;
	std::vector<DffdSelectedPoint_Data> directPointData;
	//float parameter_x, parameter_y, parameter_z;
	//float point_BsplineBase[3][10];
	//float total_down;
};




