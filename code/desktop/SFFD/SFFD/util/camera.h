#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <iostream>
#include <fstream>
#include <GLEW/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iomanip>      // std::setprecision
// 定义移动方向
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};
// 定义预设常量
const GLfloat YAW = 0.0f;
const GLfloat PITCH = 0.0f;
const GLfloat SPEED = 3.0f;
const GLfloat MOUSE_SENSITIVTY = 0.1f;
const GLfloat MOUSE_SENSITIVTY_VERT = 3.0f;
const GLfloat MOUSE_SENSITIVTY_HER = 6.0f;
const GLfloat MOUSE_ZOOM = 45.0f;
const float  MAX_PITCH_ANGLE = 89.0f; // 防止万向锁
//茶壶用
//const float scale_factor = 0.55f;//控制顶点位移0适用
const float scale_factor = 0.45f*3.5f;//大变形适用
//花瓶用
//const float scale_factor = 0.0036f;
class Camera
{
public:
	Camera(glm::vec3 pos = glm::vec3(0.0, 0.0, 2.0),
		glm::vec3 up = glm::vec3(0.0, 1.0, 0.0),
		GLfloat yaw = YAW, GLfloat pitch = PITCH) 
		:position(pos), forward(0.0, 0.0, -1.0), up(up), viewUp(up),
		moveSpeed(SPEED), mouse_zoom(MOUSE_ZOOM), mouse_sensitivity(MOUSE_SENSITIVTY),
		yawAngle(yaw), pitchAngle(pitch)
	{
		this->updateCameraVectors();
		//model = glm::translate(model, glm::vec3(0.0f, -1.55f, 0.0f)); // 适当下调位置
		//model = glm::rotate(model, glm::radians(-50.0f), glm::vec3(1.0, 0.0, 0.0));//控制顶点位移0适用

		//茶壶的视角调整
		//model = glm::rotate(model, glm::radians(-43.0f), glm::vec3(1.0, 0.0, 0.0));
		//model = glm::rotate(model, glm::radians(-10.0f), glm::vec3(0.0, 1.0, 0.0));
		//model = glm::scale(model, glm::vec3(scale_factor, scale_factor, scale_factor)); // 适当缩小模型

		//花瓶的视角调整
		//model = glm::rotate(model, glm::radians(27.0f), glm::vec3(1.0, 0.0, 0.0));
		//model = glm::scale(model, glm::vec3(1.15f, 1.15f, 1.15f)); // 适当缩小模型

		//爱心的视角调整
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, glm::radians(-40.0f), glm::vec3(1.0, 0.0, 0.0));
		model = glm::scale(model, glm::vec3(1.15f, 1.15f, 1.15f));

		//桌子
		//model = glm::rotate(model, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
		//model = glm::scale(model, glm::vec3(1.05f, 1.05f, 1.05f));

		/*
		//椅子
		model = glm::translate(model, glm::vec3(0.0f, 0.35f, 0.0f)); // 适当下调位置
		model = glm::rotate(model, glm::radians(-50.0f), glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(-1.0, 0.0, 1.0));
		model = glm::scale(model, glm::vec3(1.05f, 1.05f, 1.05f));
		*/
		//玩具兔子
		//model = glm::scale(model, glm::vec3(1.25f, 1.25f, 1.25f));
	}
public:
	// 获取视变换矩阵
	glm::mat4 getViewMatrix()
	{
		return glm::lookAt(this->position, this->position + this->forward, this->viewUp);
	}
	glm::mat4 getModelMatrix() {
		return model;
	}
	// 处理键盘按键后方向移动
	void handleKeyPress(Camera_Movement direction, GLfloat deltaTime)
	{
		GLfloat velocity = this->moveSpeed * deltaTime;
		switch (direction)
		{
		case FORWARD:
			this->position += this->forward * velocity;
			break;
		case BACKWARD:
			this->position -= this->forward * velocity;
			break;
		case LEFT:
			this->position -= this->side * velocity;
			break;
		case RIGHT:
			this->position += this->side * velocity;
			break;
		case UP:
			this->position += this->up * velocity;
			break;
		case DOWN:
			this->position -= this->up * velocity;
			break;
		default:
			break;
		}
	}
	// 处理鼠标移动
	void handleMouseMove(GLfloat xoffset, GLfloat yoffset)
	{
		
		xoffset *= this->mouse_sensitivity; // 用鼠标灵敏度调节角度变换
		yoffset *= this->mouse_sensitivity;
		
		model = glm::translate(model, glm::vec3(0.0f, 1.55f, 0.0f)); // 适当下调位置
		model = glm::rotate(model, glm::radians(xoffset*MOUSE_SENSITIVTY_HER), glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, glm::radians(yoffset*MOUSE_SENSITIVTY_VERT), glm::vec3(1.0, 0.0, 0.0));
		model = glm::translate(model, glm::vec3(0.0f, -1.55f, 0.0f)); 
		/*
		this->pitchAngle += yoffset*MOUSE_SENSITIVTY_VERT;
		this->yawAngle += xoffset*MOUSE_SENSITIVTY_HER;
		this->normalizeAngle();
		this->updateCameraVectors();
		*/
		
	}
	// 处理鼠标滚轮缩放 保持在[1.0, MOUSE_ZOOM]之间
	void handleMouseScroll(GLfloat yoffset)
	{
		if (this->mouse_zoom >= 1.0f && this->mouse_zoom <= MOUSE_ZOOM)
			this->mouse_zoom -= this->mouse_sensitivity * yoffset;
		if (this->mouse_zoom <= 1.0f)
			this->mouse_zoom = 1.0f;
		if (this->mouse_zoom >= 45.0f)
			this->mouse_zoom = 45.0f;
	}
	/*
	// 使pitch yaw角度保持在合理范围内
	void normalizeAngle()
	{
		if (this->pitchAngle > MAX_PITCH_ANGLE)
			this->pitchAngle = MAX_PITCH_ANGLE;
		if (this->pitchAngle < -MAX_PITCH_ANGLE)
			this->pitchAngle = -MAX_PITCH_ANGLE;
		if (this->yawAngle < 0.0f)
			this->yawAngle += 360.0f;
	}
	*/
	// 计算forward side up向量
	void updateCameraVectors()
	{
		glm::vec3 forward;
		forward.x = -sin(glm::radians(this->yawAngle)) * cos(glm::radians(this->pitchAngle));
		forward.y = sin(glm::radians(this->pitchAngle));
		forward.z = -cos(glm::radians(this->yawAngle)) * cos(glm::radians(this->pitchAngle));
		this->forward = glm::normalize(forward);
		
		glm::vec3 side;
		side.x = cos(glm::radians(this->yawAngle));
		side.y = 0;
		side.z = -sin(glm::radians(this->yawAngle));
		this->side = glm::normalize(side);
		std::cout<<this->side.x<< this->side.y<< this->side.z << std::endl;

	}
public:
	glm::vec3 forward,up, side, viewUp, position; // 相机属性
	GLfloat yawAngle, pitchAngle; // 欧拉角
	GLfloat moveSpeed, mouse_sensitivity, mouse_zoom; // 相机选项
private:
	glm::mat4 model;//模型矩阵
};

#endif