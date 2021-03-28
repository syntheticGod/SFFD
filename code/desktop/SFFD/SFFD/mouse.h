#pragma once
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include "bspinebody.h"
#include <cmath>

struct SelectPoint
{
	SelectPoint(){}
	SelectPoint(int x,int y,int z, glm::vec3 pos):index_x(x), index_y(y), index_z(z), pos(pos){}
	SelectPoint(SelectPoint& s) {
		index_x = s.index_x;
		index_y = s.index_y;
		index_z = s.index_z;
		pos = s.pos;
	}
	int index_x = 0;
	int index_y = 0;
	int index_z = 0;
	glm::vec3 pos= glm::vec3(0,0,0);
};

glm::vec3 Get3Dpos_withZ(int x, int y, float z, glm::mat4& modelMat, glm::mat4& viewMat, glm::mat4& projectionMat, GLint WINDOW_WIDTH, GLint WINDOW_HEIGHT) {

	GLint viewport[4] = { 0,0,WINDOW_WIDTH ,WINDOW_HEIGHT };
	glm::mat4 modelviewMat = viewMat * modelMat;

	GLfloat winX, winY, winZ;
	GLfloat object_x, object_y, object_z;
	int mouse_x = x;
	int mouse_y = y;
	//glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	//glGetDoublev(GL_PROJECTION_MATRIX, projection);
	//glGetIntegerv(GL_VIEWPORT, viewport);

	winX = (float)mouse_x;
	winY = (float)viewport[3] - (float)mouse_y;//相对于左下角的窗口y坐标
											   //glReadBuffer(GL_BACK);
											   //glReadPixels(mouse_x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
	winZ = z;
	glm::vec4 in;
	in.x = (2 * winX - WINDOW_WIDTH) / WINDOW_WIDTH;
	in.y = (2 * winY - WINDOW_HEIGHT) / WINDOW_HEIGHT;
	//in.z = 2.0 * winZ - 1.0;
	in.z = winZ;
	in.w = 1.0;

	in = glm::inverse(projectionMat*viewMat*modelMat)*in;
	if (in.w != 0)
		in = in / (in.w);

	return glm::vec3(in.x, in.y, in.z);
}


//根据屏幕坐标得到局部空间坐标（即读取物体时获得的坐标）
glm::vec3 Get3Dpos(int x, int y, glm::mat4& modelMat, glm::mat4& viewMat, glm::mat4& projectionMat, const int WINDOW_WIDTH ,const int WINDOW_HEIGHT) {
	
	GLint viewport[4] = { 0,0,WINDOW_WIDTH ,WINDOW_HEIGHT };
	glm::mat4 modelviewMat = viewMat * modelMat;

	GLdouble modelview[16];
	GLdouble projection[16];
	for (int i = 0; i < 4; i++) {
		for (int k = 0; k < 4; k++) {
			modelview[i * 4 + k] = modelviewMat[i][k];
		}
	}
	for (int i = 0; i < 4; i++) {
		for (int k = 0; k < 4; k++) {
			projection[i * 4 + k] = projectionMat[i][k];
		}
	}
	GLfloat winX, winY, winZ;
	GLdouble object_x, object_y, object_z;
	int mouse_x = x;
	int mouse_y = y;
	//glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	//glGetDoublev(GL_PROJECTION_MATRIX, projection);
	//glGetIntegerv(GL_VIEWPORT, viewport);

	winX = (float)mouse_x;
	winY = (float)viewport[3] - (float)mouse_y;//相对于左下角的窗口y坐标
	glReadBuffer(GL_BACK);
	glReadPixels(mouse_x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

	glm::vec4 in;
	in.x = (2 * winX - WINDOW_WIDTH) / WINDOW_WIDTH;
	in.y = (2 * winY - WINDOW_HEIGHT) / WINDOW_HEIGHT;
	in.z = 2.0 * winZ - 1.0;
	in.w = 1.0;

	in = glm::inverse(projectionMat*viewMat*modelMat)*in;
	if (in.w != 0)
		in = in / (in.w);
	

	//std::cout << in.x << "," << in.y << "," << in.z << "," << in.w << std::endl;
	return glm::vec3(in.x,in.y,in.z);
}

//返回最近点
SelectPoint getNearestPoint(glm::vec3 ponit,const Bspinebody* bsplinevolume) {
	int index_x = 0;
	int index_y = 0;
	int index_z = 0;
	
	float min_distance = 100000;
	for (int i = 0; i < bsplinevolume->controlPointNums[0]; i++) 
		for (int j = 0; j < bsplinevolume->controlPointNums[1]; j++) 
			for (int k = 0; k < bsplinevolume->controlPointNums[2]; k++) {
				glm::vec4 temp_point = bsplinevolume->controlPoints[i][j][k];
				float distance = pow(temp_point.x - ponit.x, 2) + pow(temp_point.y - ponit.y, 2) + pow(temp_point.z - ponit.z, 2);
				if (distance <= min_distance) {
					index_x = i;
					index_y = j;
					index_z = k;
					min_distance = distance;
				}
			}
	glm::vec3 tempVec = glm::vec3(bsplinevolume->controlPoints[index_x][index_y][index_z].x, bsplinevolume->controlPoints[index_x][index_y][index_z].y, bsplinevolume->controlPoints[index_x][index_y][index_z].z);
	SelectPoint result = SelectPoint(index_x, index_y, index_z, tempVec);
	return result;
}




