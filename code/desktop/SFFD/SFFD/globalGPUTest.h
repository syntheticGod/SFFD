#pragma once
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLEW/glew.h>
 
glm::vec3 testBsplineUniform(glm::vec3 positon);//�������ã��ж�����������ȷ��
int calSeg(int dimention, float t);//���¾�Ϊ�������Ӻ���
float calBase_X(int point_index, float t);
float calBase_Y(int point_index, float t);
float calBase_Z(int point_index, float t);


//uniform��
extern GLint degree_global[3];
extern GLint controlPonitNums_global[3];
extern GLfloat controlPoints_global[375];
extern GLfloat base_x_global[60];
extern GLfloat base_y_global[60];
extern GLfloat base_z_global[60];
extern GLfloat controlPoints_localCordinate_singleDimention_global[6];