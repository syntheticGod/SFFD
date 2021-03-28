#pragma once
#include <GLES3/gl31.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "bspinebody.h"
#include "../model/mesh.h"
#include <cmath>
#include <../util/LogUtil.h>
#define MAX_NEARESTTOUCHPOINT_DISTANCE_LIMIT 0.1
#define MAX_NEARESTDIRECTPOINT_DISTANCE_LIMIT 200
#include <mesh.h>
#include <algorithm>
#include <vector>


struct DistancePoint{
    float distance;//到光线的距离
    int index;
    DistancePoint(){}
    DistancePoint(float dis,int i):distance(dis),index(i){}
    bool operator<(const DistancePoint& v)  const{
       return distance<v.distance;
    }
};


//用作直接编辑点时，取X分量为原模型中的顶点下标号
struct SelectPoint
{
	SelectPoint(){}
	SelectPoint(int x,int y,int z, glm::vec3 pos):index_x(x), index_y(y), index_z(z), pos(pos){}
	SelectPoint(const SelectPoint& s) {
		index_x = s.index_x;
		index_y = s.index_y;
		index_z = s.index_z;
		pos = s.pos;
	}
	//仅对下标进行比较
	bool operator==(const SelectPoint& v)  const{
		if(index_x!=v.index_x||index_y!=v.index_y||index_z!=v.index_z)
			return false;
		return true;
	}

	int index_x = 0;
	int index_y = 0;
	int index_z = 0;
	glm::vec3 pos= glm::vec3(0,0,0);
};

/***
 *
 * @param p
 * @param l1
 * @param l2
 * @return 距离的平方（减少运算量）
 */
inline float pointToLineDistance3D(glm::vec3 p, glm::vec3 l1, glm::vec3 l2)
{
	glm::vec3 line_vec = l2 - l1; //AB
	glm::vec3 point_vec = p - l1; //AP
	float d = glm::dot(line_vec, point_vec) / glm::length(line_vec); //投影的长度 |AC|
	return pow(glm::length(point_vec), 2) - pow(d, 2); //勾股定理：|CP| = sqrt(AP^2-AC^2)
}


/***
 * GLES中无法直接读取深度缓存，故引入新的参数z人工设定深度，求出屏幕触点到远平面的光线
 * @param x
 * @param y
 * @param z 人工设定深度参数，范围在-1到1之间（含）
 * @param modelMat
 * @param viewMat
 * @param projectionMat
 * @param WINDOW_WIDTH
 * @param WINDOW_HEIGHT
 * @return
 */
inline glm::vec3 Get3Dpos(int x, int y, float z,glm::mat4& modelMat, glm::mat4& viewMat, glm::mat4& projectionMat, GLint WINDOW_WIDTH ,GLint WINDOW_HEIGHT) {
	
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
    winZ=z;
	glm::vec4 in;
	in.x = (2 * winX - WINDOW_WIDTH) / WINDOW_WIDTH;
	in.y = (2 * winY - WINDOW_HEIGHT) / WINDOW_HEIGHT;
	//in.z = 2.0 * winZ - 1.0;
	in.z = winZ;
	in.w = 1.0;

	in = glm::inverse(projectionMat*viewMat*modelMat)*in;
	if (in.w != 0)
		in = in / (in.w);

	return glm::vec3(in.x,in.y,in.z);
}

/***
 * 返回最近点（从控制顶点中查找）
 * @param ponit
 * @param ray
 * @param bsplinevolume
 * @return
 */
inline bool getNearestPoint_AmongControPoints(glm::vec3 point1,glm::vec3 point2,const Bspinebody* bsplinevolume,SelectPoint& selectP) {
	int index_x = 0;
	int index_y = 0;
	int index_z = 0;
	
	float min_distance = 100000;
	for (int i = 0; i < bsplinevolume->controlPointNums[0]; i++) 
		for (int j = 0; j < bsplinevolume->controlPointNums[1]; j++) 
			for (int k = 0; k < bsplinevolume->controlPointNums[2]; k++) {
				glm::vec4 temp_point = bsplinevolume->controlPoints[i][j][k];
				float distance = pointToLineDistance3D(glm::vec3(temp_point),point1,point2);
				if (distance <= min_distance) {
					index_x = i;
					index_y = j;
					index_z = k;
					min_distance = distance;
				}
			}
	         LOGCATE("P1=%f,%f,%f",point1.x,point1.y,point1.z);
	         LOGCATE("P2=%f,%f,%f",point2.x,point2.y,point2.z);
	         LOGCATE("mindistance=%f",min_distance);
			LOGCATE("selecedPoint position=%f,%f,%f",bsplinevolume->controlPoints[index_x][index_y][index_z].x,bsplinevolume->controlPoints[index_x][index_y][index_z].y,bsplinevolume->controlPoints[index_x][index_y][index_z].z);
	if(min_distance>MAX_NEARESTTOUCHPOINT_DISTANCE_LIMIT)
		return false;
	else{
		glm::vec3 tempVec = glm::vec3(bsplinevolume->controlPoints[index_x][index_y][index_z]);
		selectP= SelectPoint(index_x, index_y, index_z, tempVec);
	}

	return true;
}
/***
 * 从模型的原始顶点获取最新的选择点
 * @param point1
 * @param point2
 * @param mesh
 * @param selectP
 * @return
 */
 //添加model矩阵和视点位置，在一个距离阈值内的所有点，取离视点最近的点作为结果
inline bool getNearestPoint_AmongOriginVerts(glm::vec3 point1,glm::vec3 point2,const Mesh* mesh,SelectPoint& selectP,
        glm::mat4 modelMatrix,float camera_x,float camera_y,float camera_z){
     int index_vert=0;
     const int vecSize_limit=20;
     std::vector<DistancePoint> distance_vec;

    for (int i = 0; i < mesh->origin_vertData.size(); i++) {
                glm::vec4 temp_point = mesh->origin_vertData[i].position;
                float distance = pointToLineDistance3D(glm::vec3(temp_point),point1,point2);
                if(distance_vec.size()<vecSize_limit){
                    distance_vec.push_back(DistancePoint(distance,i));
                    if(distance_vec.size()==vecSize_limit)
                        std::sort(distance_vec.begin(),distance_vec.end());
                }
                else{
                    if(distance<distance_vec[vecSize_limit-1].distance){
                        distance_vec.push_back(DistancePoint(distance,i));
                        std::sort(distance_vec.begin(),distance_vec.end());
                        distance_vec.pop_back();
                    }
                }
            }

    //获取待选距离点中离视点最近的那个点
    float min_distance=100000;
    for(int i=0;i<distance_vec.size();i++){
        int index=distance_vec[i].index;
        glm::vec4 origin_pos=glm::vec4(glm::vec3(mesh->origin_vertData[index].position),1.0f);
        glm::vec4 pos_world=modelMatrix*origin_pos;
        pos_world=pos_world/pos_world.w;
        float temp_distance=pow(pos_world.x-camera_x,2)+pow(pos_world.y-camera_y,2)+pow(pos_world.z-camera_z,2);
        if(temp_distance<min_distance){
            index_vert=index;
            min_distance=temp_distance;
        }
    }

	if(min_distance>MAX_NEARESTDIRECTPOINT_DISTANCE_LIMIT)
		return false;
	else{
		glm::vec3 tempVec = glm::vec3(mesh->origin_vertData[index_vert].position);
		selectP= SelectPoint(index_vert, 0, 0, tempVec);
	}
	return true;
}

inline bool getNearestPoint_AmongDirectPoints(glm::vec3 point1,glm::vec3 point2,const std::vector<SelectPoint>& directPoints,SelectPoint& selectP){
	int index_vert=0;

	float min_distance = 100000;
	for (int i = 0; i < directPoints.size(); i++) {
		float distance = pointToLineDistance3D(directPoints[i].pos,point1,point2);
		if (distance <= min_distance) {
			index_vert=i;
			min_distance = distance;
		}
	}
	if(min_distance>MAX_NEARESTTOUCHPOINT_DISTANCE_LIMIT)
		return false;
	else
		selectP= directPoints[index_vert];

	return true;
}

