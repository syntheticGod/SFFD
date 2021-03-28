#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <iostream>
#include <fstream>
#include <GLEW/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iomanip>      // std::setprecision
// �����ƶ�����
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};
// ����Ԥ�賣��
const GLfloat YAW = 0.0f;
const GLfloat PITCH = 0.0f;
const GLfloat SPEED = 3.0f;
const GLfloat MOUSE_SENSITIVTY = 0.1f;
const GLfloat MOUSE_SENSITIVTY_VERT = 3.0f;
const GLfloat MOUSE_SENSITIVTY_HER = 6.0f;
const GLfloat MOUSE_ZOOM = 45.0f;
const float  MAX_PITCH_ANGLE = 89.0f; // ��ֹ������
//�����
//const float scale_factor = 0.55f;//���ƶ���λ��0����
const float scale_factor = 0.45f*3.5f;//���������
//��ƿ��
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
		//model = glm::translate(model, glm::vec3(0.0f, -1.55f, 0.0f)); // �ʵ��µ�λ��
		//model = glm::rotate(model, glm::radians(-50.0f), glm::vec3(1.0, 0.0, 0.0));//���ƶ���λ��0����

		//������ӽǵ���
		//model = glm::rotate(model, glm::radians(-43.0f), glm::vec3(1.0, 0.0, 0.0));
		//model = glm::rotate(model, glm::radians(-10.0f), glm::vec3(0.0, 1.0, 0.0));
		//model = glm::scale(model, glm::vec3(scale_factor, scale_factor, scale_factor)); // �ʵ���Сģ��

		//��ƿ���ӽǵ���
		//model = glm::rotate(model, glm::radians(27.0f), glm::vec3(1.0, 0.0, 0.0));
		//model = glm::scale(model, glm::vec3(1.15f, 1.15f, 1.15f)); // �ʵ���Сģ��

		//���ĵ��ӽǵ���
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, glm::radians(-40.0f), glm::vec3(1.0, 0.0, 0.0));
		model = glm::scale(model, glm::vec3(1.15f, 1.15f, 1.15f));

		//����
		//model = glm::rotate(model, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
		//model = glm::scale(model, glm::vec3(1.05f, 1.05f, 1.05f));

		/*
		//����
		model = glm::translate(model, glm::vec3(0.0f, 0.35f, 0.0f)); // �ʵ��µ�λ��
		model = glm::rotate(model, glm::radians(-50.0f), glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(-1.0, 0.0, 1.0));
		model = glm::scale(model, glm::vec3(1.05f, 1.05f, 1.05f));
		*/
		//�������
		//model = glm::scale(model, glm::vec3(1.25f, 1.25f, 1.25f));
	}
public:
	// ��ȡ�ӱ任����
	glm::mat4 getViewMatrix()
	{
		return glm::lookAt(this->position, this->position + this->forward, this->viewUp);
	}
	glm::mat4 getModelMatrix() {
		return model;
	}
	// ������̰��������ƶ�
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
	// ��������ƶ�
	void handleMouseMove(GLfloat xoffset, GLfloat yoffset)
	{
		
		xoffset *= this->mouse_sensitivity; // ����������ȵ��ڽǶȱ任
		yoffset *= this->mouse_sensitivity;
		
		model = glm::translate(model, glm::vec3(0.0f, 1.55f, 0.0f)); // �ʵ��µ�λ��
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
	// �������������� ������[1.0, MOUSE_ZOOM]֮��
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
	// ʹpitch yaw�Ƕȱ����ں���Χ��
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
	// ����forward side up����
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
	glm::vec3 forward,up, side, viewUp, position; // �������
	GLfloat yawAngle, pitchAngle; // ŷ����
	GLfloat moveSpeed, mouse_sensitivity, mouse_zoom; // ���ѡ��
private:
	glm::mat4 model;//ģ�;���
};

#endif