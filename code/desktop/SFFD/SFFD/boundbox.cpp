#include "boundbox.h"
#include "bspinebody.h"
#include "globalGPUTest.h"
#include "ssbo.h"
#include <fstream>
#include "cutScheme.h"
#include <math.h>
Boundbox::Boundbox() :hasInit(false) {}



///更新包围盒边界
void Boundbox::updateBoundary(Vertex &vertex) {
	if (hasInit) {
		if (vertex.position.x < min[0])
			min[0] = vertex.position.x;
		else if (vertex.position.x > max[0])
			max[0] = vertex.position.x;

		if (vertex.position.y < min[1])
			min[1] = vertex.position.y;
		else if (vertex.position.y > max[1])
			max[1] = vertex.position.y;


		if (vertex.position.z < min[2])
			min[2] = vertex.position.z;
		else if (vertex.position.z > max[2])
			max[2] = vertex.position.z;
	}
	else {
		min[0] = vertex.position.x;
		max[0] = vertex.position.x;
		min[1] = vertex.position.y;
		max[1] = vertex.position.y;
		min[2] = vertex.position.z;
		max[2] = vertex.position.z;
		hasInit = true;
	}
}
int Boundbox::generateBspinebody() {

	if (!hasInit)
		return 0;
	bspinebody = new Bspinebody();	
	int result = bspinebody->InitBspinebody(this);
	for (int i = 0; i < 3; i++)
		seg_nums[i] = bspinebody->controlPointNums[i] - bspinebody->degree[i];

	return result;
}

int Boundbox::generateBspinebody(unsigned int cx, unsigned int cy, unsigned int cz, unsigned int dx, unsigned int dy, unsigned int dz) {
	if (!hasInit)
		return 0;
	bspinebody = new Bspinebody(cx,cy,cz,dx,dy,dz);
	int result = bspinebody->InitBspinebody(this);
	for (int i = 0; i < 3; i++)
		seg_nums[i] = bspinebody->controlPointNums[i] - bspinebody->degree[i];

	return result;
}

void Boundbox::draw(const Shader& shader) const {

	if (hasInit) {
		bspinebody->draw(shader);
	}
}



void Boundbox::setupUniform_BsplineCompute(const Shader& shader) {
	glUseProgram(0);
	shader.use();
	int degree[3];
	int controlPonitNums[3];
	float controlPoints_localCordinate_singleDimention[6];
	for (int i = 0; i < 3; i++)
		degree[i] = bspinebody->degree[i];
	glUniform1iv(glGetUniformLocation(shader.programId, "degree"), 3, degree);

	
	for (int i = 0; i < 3; i++)
		controlPonitNums[i] = bspinebody->controlPointNums[i];
	glUniform1iv(glGetUniformLocation(shader.programId, "controlPonitNums"), 3, controlPonitNums);

	controlPoints_localCordinate_singleDimention[0] = min[0];
	controlPoints_localCordinate_singleDimention[1] = max[0];
	controlPoints_localCordinate_singleDimention[2] = min[1];
	controlPoints_localCordinate_singleDimention[3] = max[1];
	controlPoints_localCordinate_singleDimention[4] = min[2];
	controlPoints_localCordinate_singleDimention[5] = max[2];
	glUniform1fv(glGetUniformLocation(shader.programId, "controlPoints_localCordinate_singleDimention"), 6, controlPoints_localCordinate_singleDimention);
	glUseProgram(0);
}
void Boundbox::setupComputeData_deformation(const Shader& shader) {
	glUseProgram(0);
	shader.use();
	int controlPonitNums[3];
	for (int i = 0; i < 3; i++) {
		controlPonitNums[i] = bspinebody->controlPointNums[i];
		seg_nums[i] = controlPonitNums[i] - 3;
	}
	glUniform1iv(glGetUniformLocation(shader.programId, "controlPonitNums"), 3, controlPonitNums);
	int pointOffset = 2 * (controlPonitNums[0] + controlPonitNums[1] + controlPonitNums[2]) - 3;
	glUniform1ui(glGetUniformLocation(shader.programId, "pointOffset"), pointOffset);
	glUseProgram(0);
}
void Boundbox::test_ffd_calBase() {
	for (int i = 0; i < 3; i++) {
		glm::vec4 base_data[200];//实际用不到200个
		glm::vec4 base_data_low[200];//低一次的b样条基

		int degree = bspinebody->degree[i];
		int controPointNums = bspinebody->controlPointNums[i];


		int index = 0;
		for (int pointIndex = 0; pointIndex < controPointNums; pointIndex++) {
			for (int seg = 0; seg < controPointNums - degree; seg++) {
				Polynomial tempPoly = bspinebody->base[i].result[pointIndex][degree + 1][seg + degree];
				base_data[index] = glm::vec4(tempPoly.data[0], tempPoly.data[1], tempPoly.data[2], tempPoly.data[3]);
				base[i][index] = base_data[index];
				index++;
			}
		}
		setupSSBufferObjectList<glm::vec4>(baseSSbo[i], i + 3, base_data, index, false);
		index = 0;
		for (int pointIndex = 0; pointIndex < controPointNums - 1; pointIndex++) {
			for (int seg = 0; seg < controPointNums - degree; seg++) {
				Polynomial tempPoly = bspinebody->base[i].result[pointIndex + 1][degree][seg + degree];
				base_data_low[index] = glm::vec4(tempPoly.data[0], tempPoly.data[1], tempPoly.data[2], tempPoly.data[3]);
				base_low[i][index] = base_data_low[index];
				index++;
			}
		}		
	}
}
void Boundbox::setupComputeData_bsplineBase(const Shader& shader) {
	glUseProgram(0);
	shader.use();
	for (int i = 0; i < 3; i++) {
		glm::vec4 base_data[200];//实际用不到200个
		glm::vec4 base_data_low[200];//低一次的b样条基

		int degree = bspinebody->degree[i];
		int controPointNums = bspinebody->controlPointNums[i];


		int index = 0;
		for (int pointIndex = 0; pointIndex < controPointNums; pointIndex++) {
			for (int seg = 0; seg < controPointNums - degree; seg++) {
				Polynomial tempPoly = bspinebody->base[i].result[pointIndex][degree + 1][seg + degree];
				base_data[index] = glm::vec4(tempPoly.data[0], tempPoly.data[1], tempPoly.data[2], tempPoly.data[3]);
				base[i][index] = base_data[index];
				index++;
			}
		}
		setupSSBufferObjectList<glm::vec4>(baseSSbo[i], i + 3, base_data, index, false);
		index = 0;
		for (int pointIndex = 0; pointIndex < controPointNums-1; pointIndex++) {
			for (int seg = 0; seg < controPointNums - degree; seg++) {
				Polynomial tempPoly = bspinebody->base[i].result[pointIndex+1][degree][seg + degree];
				base_data_low[index] = glm::vec4(tempPoly.data[0], tempPoly.data[1], tempPoly.data[2], tempPoly.data[3]);
				base_low[i][index] = base_data_low[index];
				index++;
			}
		}
		setupSSBufferObjectList<glm::vec4>(baseSSBO_low[i], i + 6, base_data_low, index, false);
	}


	int controlPonitNums[3];
	int seg_nums[3];
	float controlPoints_localCordinate_singleDimention[6];

	for (int i = 0; i < 3; i++) {
		controlPonitNums[i] = bspinebody->controlPointNums[i];
		seg_nums[i] = controlPonitNums[i] - 3;
	}		
	glUniform1iv(glGetUniformLocation(shader.programId, "controlPonitNums"), 3, controlPonitNums);
	glUniform1iv(glGetUniformLocation(shader.programId, "seg_nums"), 3, seg_nums);

	controlPoints_localCordinate_singleDimention[0] = min[0];
	controlPoints_localCordinate_singleDimention[1] = max[0];
	controlPoints_localCordinate_singleDimention[2] = min[1];
	controlPoints_localCordinate_singleDimention[3] = max[1];
	controlPoints_localCordinate_singleDimention[4] = min[2];
	controlPoints_localCordinate_singleDimention[5] = max[2];
	glUniform1fv(glGetUniformLocation(shader.programId, "controlPoints_localCordinate_singleDimention"), 6, controlPoints_localCordinate_singleDimention);

	int pointOffset = 2 * (controlPonitNums[0] + controlPonitNums[1] + controlPonitNums[2]) - 3;
	glUniform1ui(glGetUniformLocation(shader.programId, "pointOffset"), pointOffset);
	glUseProgram(0);
}

int Boundbox::getTotalControlPointNums() {
	return bspinebody->controlPointNums[0]+ bspinebody->controlPointNums[1]+ bspinebody->controlPointNums[2];
}
void Boundbox::setupComputeData_bsplinebody() {
	updateControlPoints();

	glm::vec4 base_data[200];//实际用不到200个
	glm::vec4 base_data_low[200];//低一次的b样条基

	for (int i = 0; i < 3; i++) {
		int degree = bspinebody->degree[i];
		int controPointNums = bspinebody->controlPointNums[i];
		

		int index = 0;
		for (int pointIndex = 0; pointIndex < controPointNums; pointIndex++) {
			for (int seg = 0; seg < controPointNums- degree; seg++) {
				Polynomial tempPoly=bspinebody->base[i].result[pointIndex][degree + 1][seg + degree];
				base_data[index] = glm::vec4(tempPoly.data[0], tempPoly.data[1], tempPoly.data[2], tempPoly.data[3]);
				base[i][index] = base_data[index];
				index++;
			}
		}
		setupSSBufferObjectList<glm::vec4>(baseSSbo[i], i+1, base_data, index,false);	
		index = 0;
		for (int pointIndex = 0; pointIndex <= controPointNums; pointIndex++) {
			for (int seg = 0; seg < controPointNums - degree; seg++) {
				Polynomial tempPoly = bspinebody->base[i].result[pointIndex][degree][seg + degree];
				base_data_low[index] = glm::vec4(tempPoly.data[0], tempPoly.data[1], tempPoly.data[2], tempPoly.data[3]);
				base_low[i][index] = base_data_low[index];
				index++;
			}
		}
		setupSSBufferObjectList<glm::vec4>(baseSSBO_low[i], i + 12, base_data_low, index, false);
	}	
	//传输节点向量
	for (int i = 0; i < 3; i++) {

		setupSSBufferObjectList<GLfloat>(knotVectorSSBO[i], i + 9, &(bspinebody->knotVector[i][0]), bspinebody->controlPointNums[i]+ bspinebody->degree[i]+1, false);
	}
}

void Boundbox::updateControlPoints() {
	//计算差分控制顶点的ssbo
	bspinebody->cal_controlPoints_derivative();
	std::vector<glm::vec4> controlPointsVec = std::vector<glm::vec4>();
	for (int i = 0; i < bspinebody->controlPointNums[0]; i++)
		for (int j = 0; j < bspinebody->controlPointNums[1]; j++)
			for (int k = 0; k < bspinebody->controlPointNums[2]; k++)
				controlPointsVec.push_back(bspinebody->controlPoints[i][j][k]);
	glDeleteBuffers(1, &controlPointSSbo);
	glDeleteBuffers(1, &controlPoint_derivative_SSbo[0]);
	glDeleteBuffers(1, &controlPoint_derivative_SSbo[1]);
	glDeleteBuffers(1, &controlPoint_derivative_SSbo[2]);
	setupSSBufferObjectVec<glm::vec4>(controlPointSSbo, 3, controlPointsVec, true);
	setupSSBufferObjectVec<glm::vec4>(controlPoint_derivative_SSbo[0], 4, bspinebody->controlPoints_derivative_x, true);
	setupSSBufferObjectVec<glm::vec4>(controlPoint_derivative_SSbo[1], 5, bspinebody->controlPoints_derivative_y, true);
	setupSSBufferObjectVec<glm::vec4>(controlPoint_derivative_SSbo[2], 6, bspinebody->controlPoints_derivative_z, true);
	bspinebody->controlPoints_needUpdate = false;
}

void Boundbox::setupComputeData_precomputeMatrix_triangleControlPoints() {
	string fileStr = "precompute_matrix/restraintAndFittingMatrix" + to_string(bspinebody->degree[0]) + "_" + to_string(bspinebody->degree[1]) + "_" + to_string(bspinebody->degree[2]) + ".txt";
	int totalD = bspinebody->degree[0] + bspinebody->degree[1] + bspinebody->degree[2];
	const int fittingPointNums = (1 + totalD)*(2 + totalD) / 2;
	ifstream fs(fileStr);
	float data[10 * (55 + 9)];//最大所需空间
	int index = 0;
	for (int i = 0; i < 10; i++) {//只读前十行
		for (int j = 0; j < 9+ fittingPointNums; j++) {
			fs >> data[index];
			index++;
		}	
	}		
	fs.close();
	setupSSBufferObjectList<float>(precomputeMatrix_triangleControlPointsSSbo, 6, data, 10 * (55 + 9), false);
}

void Boundbox::setupComputeData_precomputeMatrix_tessellation() {
	int tessellationTimes = CutScheme::tessellation_times;
	string fileStr = "precompute_matrix/tessellationMatrix_"+to_string(tessellationTimes)+".txt";
	const int totalPointNums = (tessellationTimes + 1)*(tessellationTimes + 2) / 2;
	ifstream fs(fileStr);
	float data[10 * 21];//最大支持细分段数暂定为5，即totalPointNums=21
	int index = 0;
	for (int i = 0; i < totalPointNums; i++) {
		for (int j = 0; j < 10; j++) {
			fs >> data[index];
			index++;
		}
	}
	fs.close();
	setupSSBufferObjectList<float>(precomputeMatrix_tessellationSSbo, 11, data, 10 * 21, false);
}


//一些测试要用到的函数
int Boundbox::calSeg(int dimention, float t) {
	float range = max[dimention] - min[dimention];
	float space = range / seg_nums[dimention];

	int result = int((t - min[dimention]) / space);
	if (result >= seg_nums[dimention])
		result = seg_nums[dimention] - 1;
	return result;
}


float Boundbox::calBase_X(int point_index, float t) {//二者均0基
	int seg = calSeg(0, t);
	int offset = point_index * seg_nums[0] + seg;
	glm::vec4 polynomial = base[0][offset];
	float result = polynomial.x + polynomial.y*t + polynomial.z*t*t + polynomial.w*t*t*t;
	return result;
}
float Boundbox::calBase_Y(int point_index, float t) {//二者均0基
	int seg = calSeg(1, t);
	int offset = point_index * seg_nums[1] + seg;
	glm::vec4 polynomial = base[1][offset];
	float result = polynomial.x + polynomial.y*t + polynomial.z*t*t + polynomial.w*t*t*t;
	return result;
}
float Boundbox::calBase_Z(int point_index, float t) {//二者均0基
	int seg = calSeg(2, t);
	int offset = point_index * seg_nums[2] + seg;
	glm::vec4 polynomial = base[2][offset];
	float result = polynomial.x + polynomial.y*t + polynomial.z*t*t + polynomial.w*t*t*t;
	return result;
}

float Boundbox::calBase_X_derivative(int point_index, float t) {//二者均0基,seg的段数在求导前后均不变，
	int seg = calSeg(0, t);
	int offset = point_index * seg_nums[0] + seg;
	glm::vec4 polynomial = base_low[0][offset];
	float result = polynomial.x + polynomial.y*t + polynomial.z*t*t + polynomial.w*t*t*t;
	return result;
}
float Boundbox::calBase_Y_derivative(int point_index, float t) {//二者均0基
	int seg = calSeg(1, t);
	int offset = point_index * seg_nums[1] + seg;
	glm::vec4 polynomial = base_low[1][offset];
	float result = polynomial.x + polynomial.y*t + polynomial.z*t*t + polynomial.w*t*t*t;
	return result;
}
float Boundbox::calBase_Z_derivative(int point_index, float t) {//二者均0基
	int seg = calSeg(2, t);
	int offset = point_index * seg_nums[2] + seg;
	glm::vec4 polynomial = base_low[2][offset];
	float result = polynomial.x + polynomial.y*t + polynomial.z*t*t + polynomial.w*t*t*t;
	return result;
}

glm::vec3 Boundbox::test_calPositionAfterDeformation_FFD(glm::vec3 position_pragma) {
	int controlPonitNums[3] = { bspinebody->controlPointNums[0],bspinebody->controlPointNums[1],bspinebody->controlPointNums[2] };
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i<controlPonitNums[0]; i++) {
		for (int j = 0; j<controlPonitNums[1]; j++) {
			for (int k = 0; k<controlPonitNums[2]; k++) {
				glm::vec3 controlPonit = glm::vec3(bspinebody->controlPoints[i][j][k]);
				float tempx = calBase_X(i, position_pragma.x);
				float tempy = calBase_Y(j, position_pragma.y);
				float tempz = calBase_Z(k, position_pragma.z);
				position = position + controlPonit*tempx*tempy*tempz;
			}
		}
	}
	return position;
}

glm::vec3 Boundbox::test_calNormalAfterDeformation(glm::vec3 position_pragma, glm::vec3 origin_normal) {
	int controlPonitNums[3] = { bspinebody->controlPointNums[0],bspinebody->controlPointNums[1],bspinebody->controlPointNums[2] };
	
	glm::vec3 Jmat[3];//列主矩阵，按列排三个vec3
	glm::vec3 normal = glm::vec3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i<controlPonitNums[0]-1; i++) {
		for (int j = 0; j<controlPonitNums[1]; j++) {
			for (int k = 0; k<controlPonitNums[2]; k++) {
				int index = i*controlPonitNums[1] * controlPonitNums[2] + j*controlPonitNums[2] + k;
				glm::vec4 temp = bspinebody->controlPoints_derivative_x[index];
				glm::vec3 controlPonit = glm::vec3(temp.x, temp.y, temp.z);
				float tempx = calBase_X_derivative(i, position_pragma.x);
				float tempy = calBase_Y(j, position_pragma.y);
				float tempz = calBase_Z(k, position_pragma.z);
				float temptotal = tempx*tempy*tempz;
				normal = normal + controlPonit*temptotal;
			}
		}
	}
	Jmat[0] = normal;
	normal = glm::vec3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i<controlPonitNums[0]; i++) {
		for (int j = 0; j<controlPonitNums[1]-1; j++) {
			for (int k = 0; k<controlPonitNums[2]; k++) {
				int index = j*controlPonitNums[0] * controlPonitNums[2] + i*controlPonitNums[2] + k;
				glm::vec4 temp = bspinebody->controlPoints_derivative_y[index];
				glm::vec3 controlPonit = glm::vec3(temp.x, temp.y, temp.z);
				float tempx = calBase_X(i, position_pragma.x);
				float tempy = calBase_Y_derivative(j, position_pragma.y);
				float tempz = calBase_Z(k, position_pragma.z);
				float temptotal = tempx*tempy*tempz;
				normal = normal + controlPonit*temptotal;
			}
		}
	}
	Jmat[1] = normal;
	normal = glm::vec3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i<controlPonitNums[0]; i++) {
		for (int j = 0; j<controlPonitNums[1]; j++) {
			for (int k = 0; k<controlPonitNums[2]-1; k++) {
				int index = k*controlPonitNums[0] * controlPonitNums[1] + i*controlPonitNums[1] + j;
				glm::vec4 temp = bspinebody->controlPoints_derivative_z[index];
				glm::vec3 controlPonit = glm::vec3(temp.x, temp.y, temp.z);
				float tempx = calBase_X(i, position_pragma.x);
				float tempy = calBase_Y(j, position_pragma.y);
				float tempz = calBase_Z(k, position_pragma.z);
				float temptotal = tempx*tempy*tempz;
				normal = normal + controlPonit*temptotal;
			}
		}
	}
	Jmat[2] = normal;

	//计算伴随矩阵（x123即第一行的123元素）
	float x1 = cal_determinant(Jmat[1].y, Jmat[1].z, Jmat[2].y, Jmat[2].z);
	float x2 = -1 * cal_determinant(Jmat[1].x, Jmat[1].z, Jmat[2].x, Jmat[2].z);
	float x3 = cal_determinant(Jmat[1].x, Jmat[1].y, Jmat[2].x, Jmat[2].y);
	float y1 = -1 * cal_determinant(Jmat[0].y, Jmat[0].z, Jmat[2].y, Jmat[2].z);
	float y2 = cal_determinant(Jmat[0].x, Jmat[0].z, Jmat[2].x, Jmat[2].z);
	float y3 = -1 * cal_determinant(Jmat[0].x, Jmat[0].y, Jmat[2].x, Jmat[2].y);
	float z1 = cal_determinant(Jmat[0].y, Jmat[0].z, Jmat[1].y, Jmat[1].z);
	float z2 = -1 * cal_determinant(Jmat[0].x, Jmat[0].z, Jmat[1].x, Jmat[1].z);
	float z3 = cal_determinant(Jmat[0].x, Jmat[0].y, Jmat[1].x, Jmat[1].y);
	//转置
	float temp = x2;
	x2 = y1;
	y1 = temp;

	temp = x3;
	x3 = z1;
	z1 = temp;

	temp = y3;
	y3 = z2;
	z2 = temp;
	//矩阵向量乘法
	float result_x = x1*origin_normal.x + x2*origin_normal.y + x3*origin_normal.z;
	float result_y = y1*origin_normal.x + y2*origin_normal.y + y3*origin_normal.z;
	float result_z = z1*origin_normal.x + z2*origin_normal.y + z3*origin_normal.z;
	return glm::vec3(result_x, result_y, result_z);

}

bool Boundbox::isControlPointsNeedUpdate() {
	return bspinebody->controlPoints_needUpdate;
}

void Boundbox::dffd_adjustControlPoints( float movement_x, float movement_y, float movement_z) {

	glm::vec4 temp_pos = glm::vec4(0, 0, 0, 0);
	//计算最终调整方案并调整
	for (int i = 0; i < bspinebody->controlPointNums[0]; i++) {
		for (int j = 0; j < bspinebody->controlPointNums[1]; j++) {
			for (int k = 0; k < bspinebody->controlPointNums[2]; k++) {
				float temp_n = point_BsplineBase[0][i] * point_BsplineBase[1][j] * point_BsplineBase[2][k];
				glm::vec4 eachControlPointAdjustment = temp_n / total_down*glm::vec4(movement_x, movement_y, movement_z,0);
				bspinebody->controlPoints[i][j][k] += eachControlPointAdjustment;
				temp_pos += temp_n*bspinebody->controlPoints[i][j][k];
			}
		}
	}
	//重新设置包围盒的缓冲区
	bspinebody->setupBuffer();
	selectedPoint.position = temp_pos;
}

void Boundbox::setSelectedPoint(Vertex& v) {
	selectedPoint = v;
	parameter_x = v.position.x;
	parameter_y = v.position.y;
	parameter_z = v.position.z;
	int controlPonitNums[3];
	for (int i = 0; i < 3; i++) {
		controlPonitNums[i] = bspinebody->controlPointNums[i];
		seg_nums[i] = controlPonitNums[i] - 3;
	}


	for (int i = 0; i<bspinebody->controlPointNums[0]; i++)
		point_BsplineBase[0][i] = calBase_X(i, parameter_x);
	for (int i = 0; i<bspinebody->controlPointNums[1]; i++)
		point_BsplineBase[1][i] = calBase_X(i, parameter_y);
	for (int i = 0; i<bspinebody->controlPointNums[2]; i++)
		point_BsplineBase[2][i] = calBase_X(i, parameter_z);

	total_down = 0;
	//计算所有调整显示解的分母
	for (int i = 0; i < bspinebody->controlPointNums[0]; i++) {
		for (int j = 0; j < bspinebody->controlPointNums[1]; j++) {
			for (int k = 0; k < bspinebody->controlPointNums[2]; k++) {
				float t = (point_BsplineBase[0][i] * point_BsplineBase[1][j] * point_BsplineBase[2][k]);
				total_down += t*t;
			}
		}
	}
}

void Boundbox::setupSeletedPointBuffer() {
	glm::vec3 position = glm::vec3(selectedPoint.position.x, selectedPoint.position.y, selectedPoint.position.z);
	glGenVertexArrays(1, &selectedPointVao);
	glGenBuffers(1, &selectedPointVbo);
	
	glBindVertexArray(selectedPointVao);
	glBindBuffer(GL_ARRAY_BUFFER, selectedPointVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) ,
		&position, GL_STATIC_DRAW);
	// 顶点位置属性
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		0, (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);	
}

void Boundbox::drawSeletedPoint(const Shader& shader){
	shader.use();
	
	glPointSize(26.0f);
	glBindVertexArray(selectedPointVao);
	glDrawArrays(GL_POINTS, 0, 1);
	glBindVertexArray(0);
	glUseProgram(0);
	glPointSize(11.0f);
}

void Boundbox::outputControlPoints() {
	//为了和cym代码适配作出的归一化转换
	if (this->bspinebody->has_init) {
		ofstream file;
		file.open("outputControlPoints.txt");
		for (int i = 0; i < bspinebody->controlPointNums[0]; i++)
			for (int j = 0; j < bspinebody->controlPointNums[1]; j++)
				for (int k = 0; k < bspinebody->controlPointNums[2]; k++) {
					glm::vec4 tempVec = bspinebody->controlPoints[i][j][k];
					file << tempVec.x << " " << tempVec.y << " " << tempVec.z << std::endl;
				}

		/*
		double m_fMin[3], m_fMax[3];
		
		double delta[3];
		for (int i = 0; i < 3; ++i)
		{
			m_fMin[i] = min[i];
			m_fMax[i] = max[i];
			delta[i] = -0.5 * (m_fMax[i] + m_fMin[i]);
			m_fMin[i] += delta[i];
			m_fMax[i] += delta[i];
		}

		double maxXYZ = m_fMax[0];
		if (m_fMax[1] > maxXYZ)
			maxXYZ = m_fMax[1];
		if (m_fMax[2] > maxXYZ)
			maxXYZ = m_fMax[2];

		for (int i = 0; i < bspinebody->controlPointNums[0]; i++)
			for (int j = 0; j < bspinebody->controlPointNums[1]; j++)
				for (int k = 0; k < bspinebody->controlPointNums[2]; k++) {
					glm::vec4 tempVec= bspinebody->controlPoints[i][j][k] ;
					const double zeroLimit = 0.000001;
					double resultx = (tempVec.x + delta[0]) / maxXYZ;
					if (abs(resultx) < zeroLimit)
						resultx = 0;
					double resulty = (tempVec.y + delta[1]) / maxXYZ;
					if (abs(resulty) < zeroLimit)
						resulty = 0;
					double resultz = (tempVec.z + delta[2]) / maxXYZ;
					if (abs(resultz) < zeroLimit)
						resultz = 0;
					file << resultx << " " << resulty << " " << resultz << std::endl;
				}
		*/
		file.close();	
	}
	
	
}