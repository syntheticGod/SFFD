#include "globalGPUTest.h"
#include <iostream>

 GLint degree_global[3];
 GLint controlPonitNums_global[3];
 GLfloat controlPoints_global[375];
 GLfloat base_x_global[60];
 GLfloat base_y_global[60];
 GLfloat base_z_global[60];
 GLfloat controlPoints_localCordinate_singleDimention_global[6];

 using namespace std;
 glm::vec3 testBsplineUniform(glm::vec3 positon) {
	float total = 0;
	glm::vec3  totalvec = glm::vec3(0, 0, 0);
	for (int i = 0; i < controlPonitNums_global[0]; i++) {
		for (int j = 0; j < controlPonitNums_global[1]; j++) {
			for (int k = 0; k < controlPonitNums_global[2]; k++) {
				int offset = i * 75 + j * 15 + k * 3;
				glm::vec3 controlPonit = glm::vec3(controlPoints_global[offset], controlPoints_global[offset + 1], controlPoints_global[offset + 2]);				
				float tempx = calBase_X(i, positon.x);
				float tempy = calBase_Y(j, positon.y);
				float tempz = calBase_Z(k, positon.z);
				totalvec += controlPonit*tempx*tempy*tempz;
			}
		}
	}
	/*
	if (abs(positon.x- totalvec.x)>0.0001|| abs(positon.y - totalvec.y)>0.0001|| abs(positon.z - totalvec.z)>0.0001) {
		cout << "原坐标：" << positon.x << "  ,  " << positon.y << "  ,  " << positon.z;
		cout << "新坐标：" << totalvec.x << "  ,  " << totalvec.y << "  ,  " << totalvec.z << endl;
	}
	*/
	return totalvec;
}


int calSeg(int dimention, float t) {
	float range = controlPoints_localCordinate_singleDimention_global[dimention * 2 + 1] - controlPoints_localCordinate_singleDimention_global[dimention * 2];
	float space = range / (controlPonitNums_global[dimention] - degree_global[dimention]);
	int result= int((t - controlPoints_localCordinate_singleDimention_global[dimention * 2]) / space);
	if (result > 1)
		result = 1;
	return result;
}


float calBase_X(int point_index, float t) {//二者均0基
	int seg = calSeg(0, t);
	int offset = point_index * 12 + seg * 6;
	float result = 0;
	for (int i = 0; i <= degree_global[0]; i++) {
		float temp = base_x_global[i + offset];
		result = result + temp * pow(t, i);
	}

	return result;
}
float calBase_Y(int point_index, float t) {//二者均0基
	int seg = calSeg(1, t);
	int offset = point_index * 12 + seg * 6;
	float result = 0;
	for (int i = 0; i <= degree_global[1]; i++) {
		float temp = base_y_global[i + offset];
		result = result + temp * pow(t, i);
	}
		

	return result;
}
float calBase_Z(int point_index, float t) {//二者均0基
	int seg = calSeg(2, t);
	int offset = point_index * 12 + seg * 6;
	float result = 0;
	for (int i = 0; i <= degree_global[2]; i++) {
		float temp = base_z_global[i + offset];
		result = result + temp * pow(t, i);
	}
		

	return result;
}