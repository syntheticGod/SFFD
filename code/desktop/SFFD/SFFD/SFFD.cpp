// SFFD.cpp : �������̨Ӧ�ó������ڵ㡣
//

// ����GLEW�� ���徲̬����
#define GLEW_STATIC
#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"glew32s.lib")
#pragma comment(lib,"glfw3dll.lib")
#pragma comment(lib,"SOIL.lib")
#pragma comment(lib,"assimp.lib")


//#define TEST_DISPLAY_CYM


//+#define TEST_DISPLAY_ERROR

//#define OUTPUT_GROUNDTRUTH_COMPAREFILE

//#define OUTPUT_DATALIST_COMPAREFILE

//#define OUTPUT_TEAPOTFILE
#include "testCYM.h"
#ifdef OUTPUT_TEAPOTFILE
#include "teapotUtil.h"
#endif // OUTPUT_TEAPOTFILE

#include <GLEW/glew.h>
// ����GLFW��
#include <GLFW/glfw3.h>
// ����SOIL��
#include <SOIL/SOIL.h>
// ����GLM��
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cstdlib>
// ������ɫ�����ؿ�
#include "shader.h"
// ����������Ƹ�����
#include "camera.h"
// �������������
#include "texture.h"
// ����ģ�͵���
#include "model.h"
#include "mode.h"
#include <math.h>
#include <stdio.h>
#include "cutScheme.h"
// ���̻ص�����ԭ������
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
// ����ƶ��ص�����ԭ������
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_move_callback_blank(GLFWwindow* window, double xpos, double ypos);
// �����ֻص�����ԭ������
void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void loadFile_cutScheme();

//void GetWorldPos(glm::vec3* pp, Camera& cam);
// �������ƶ�
void do_movement();


bool has_test = false;
// ���������
const int WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 720;
// ���������������
GLfloat lastX = WINDOW_WIDTH / 2.0f, lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouseMove = true;
bool keyPressedStatus[1024]; // ���������¼
GLfloat deltaTime = 0.0f; // ��ǰ֡����һ֡��ʱ���
GLfloat lastFrame = 0.0f; // ��һ֡ʱ��
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
glm::vec3 lampPos(0.5f, 1.0f, 1.5f);
Draw_mode System_mode::draw_mode = normal_mode;
Deformation_mode System_mode::deformation_mode = boundbox_mode;

glm::mat4 model,view,projection;
Model objModel;
SelectPoint point_select;
bool has_select = false;
float point_size = 15;
float pickedPoint_z = 0;
const float point_size_max = 40;
const float point_size_min = 5;
bool cursor_press_leave = false;
bool show_boundbox = true;
bool selectedPointsCanChange = true;//�Ƿ���Ա��ѡ���ģ�͵�
bool selectedPointInit = false;

int main(int argc, char** argv)
{
#ifdef  OUTPUT_TEAPOTFILE
	Teapot teapot = Teapot();
	teapot.init();
	teapot.outputObjFile();
	return 1;
#endif //  OUTPUT_TEAPOTFILE

#ifdef OUTPUT_DATALIST_COMPAREFILE
	//OutputFile_CompareTimeList(0.01f,0.161f,0.003f,120);
	//OutputFile_CompareDataList(true, 0.01f, 0.16f, 0.003f, 120);
	OutputFile_CompareDataList(false, 5, 91, 5, 0.06f);
	return 1;
#endif

	loadFile_cutScheme();

	if (!glfwInit())	// ��ʼ��glfw��
	{
		std::cout << "Error::GLFW could not initialize GLFW!" << std::endl;
		return -1;
	}
	
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// ��������
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
		"SFFD", NULL, NULL);
	if (!window)
	{
		std::cout << "Error::GLFW could not create winddow!" << std::endl;
		glfwTerminate();
		std::system("pause");
		return -1;
	}
	// �����Ĵ��ڵ�contextָ��Ϊ��ǰcontext
	glfwMakeContextCurrent(window);

	// ע�ᴰ�ڼ����¼��ص�����
	glfwSetKeyCallback(window, key_callback);
	// ע������¼��ص�����
	glfwSetCursorPosCallback(window, mouse_move_callback);

	// ע���������¼��ص�����
	glfwSetScrollCallback(window, mouse_scroll_callback);
	// ��겶�� ͣ���ڳ�����
	glfwSetInputMode(window, GLFW_CURSOR,GLFW_CURSOR_DISABLED);

	// ��ʼ��GLEW ��ȡOpenGL����
	glewExperimental = GL_TRUE; // ��glew��ȡ������չ����
	GLenum status = glewInit();
	if (status != GLEW_OK)
	{
		std::cout << "Error::GLEW glew version:" << glewGetString(GLEW_VERSION)
			<< " error string:" << glewGetErrorString(status) << std::endl;
		glfwTerminate();
		std::system("pause");
		return -1;
	}

	const GLubyte* name = glGetString(GL_VENDOR); //���ظ���ǰOpenGLʵ�ֳ��̵�����
	const GLubyte* biaoshifu = glGetString(GL_RENDERER); //����һ����Ⱦ����ʶ����ͨ���Ǹ�Ӳ��ƽ̨
	const GLubyte* OpenGLVersion = glGetString(GL_VERSION); //���ص�ǰOpenGLʵ�ֵİ汾��
															//const GLubyte* Extensions  =glGetString(GL_EXTENSIONS);
	const GLubyte* gluVersion = gluGetString(GLU_VERSION); //���ص�ǰGLU���߿�汾
	printf("OpenGLʵ�ֳ��̵����֣�%s\n", name);
	printf("��Ⱦ����ʶ����%s\n", biaoshifu);
	printf("OpenGLʵ�ֵİ汾�ţ�%s\n", OpenGLVersion);
	//printf("OpenGL֧�ֵ���չ��%s\n",Extensions );
	printf("OGLU���߿�汾��%s\n", gluVersion);
	
	// �����ӿڲ���
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	

#ifdef TEST_DISPLAY_ERROR
	Error er= Error(CutScheme::cut_space, CutScheme::smoothSharpAngle);
	er.init();

	Shader error_shader("shader/error.vertex", "shader/error.frag");
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	error_shader.use();
	
	GLint beginErrorLoc = glGetUniformLocation(error_shader.programId, "beginError");
	GLint middleErrorLoc = glGetUniformLocation(error_shader.programId, "middleError");
	GLint endErrorLoc = glGetUniformLocation(error_shader.programId, "endError");

	GLint beginColorLoc = glGetUniformLocation(error_shader.programId, "beginColor");
	GLint middleColorLoc = glGetUniformLocation(error_shader.programId, "middleColor");
	GLint endColorLoc = glGetUniformLocation(error_shader.programId, "endColor");

	glUniform3f(beginColorLoc, 0.0f, 0.0f, 1.0f);
	glUniform3f(middleColorLoc, 0.0f, 1.0f, 0.0f);
	glUniform3f(endColorLoc, 1.0f, 0.0f, 0.0f);

#if defined(TEST_DISPLAY_GC_NORMAL_ERROR)||defined(TEST_DISPLAY_CYM_NORMAL_ERROR)||defined(TEST_DISPLAY_BOTH_NORMAL_ERROR)
	//��normalError��ʽ����
	glUniform1f(beginErrorLoc, 0.0f);
	glUniform1f(middleErrorLoc, 5.0f);
	glUniform1f(endErrorLoc, 10.0f);
#else
	glUniform1f(beginErrorLoc, 0.0f);
	glUniform1f(middleErrorLoc, 0.004f);
	glUniform1f(endErrorLoc, 0.008f);
#endif
	glUseProgram(0);
	

	while (!glfwWindowShouldClose(window)) {
		error_shader.use();
		GLfloat currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		glfwPollEvents(); // ����������� ���̵��¼�
		do_movement(); // �����û�������� �����������

					   // �����ɫ������ ����Ϊָ����ɫ
					   //glClearColor(0.18f, 0.04f, 0.14f, 1.0f);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		projection = glm::perspective(camera.mouse_zoom, (GLfloat)(WINDOW_WIDTH) / WINDOW_HEIGHT, 1.0f, 100.0f); // ͶӰ����
		glUniformMatrix4fv(glGetUniformLocation(error_shader.programId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		view = camera.getViewMatrix(); // �ӱ任����
		glUniformMatrix4fv(glGetUniformLocation(error_shader.programId, "view"), 1, GL_FALSE, glm::value_ptr(view));
		model = camera.getModelMatrix();//ģ�;���
		glUniformMatrix4fv(glGetUniformLocation(error_shader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glUseProgram(0);

		er.display(error_shader);
		glfwSwapBuffers(window);
	}
	// �ͷ���Դ
	glfwTerminate();
	
	return 0;
#endif // TEST_DISPLAY_ERROR

	//resources/models/nanosuit/nanosuit.obj
	//resources/newmodels/star.obj
	//resources/newmodels/heart.obj
	//resources/newmodels/diamond2.obj
	//resources/newmodels/cube.obj
	//resources/newmodels/ship.obj
	//resources/newmodels/heart.obj
	//resources/newmodels/vase.obj
	//resources/newmodels/utahTeapotObjFile.obj
	//resources/newmodels/truck/mini-truck-scene.obj
	//resources/newmodels/chair-obj/chair.obj
	//resources/newmodels/table-obj/table.obj
	//resources/newmodels/untiene-rabbit-obj/untiene-rabbit.obj

#ifdef  TEST_DISPLAY_CYM
	CYMResult cym_result = CYMResult();
	cym_result.loadModel_CYMResult();
#else
	if (!objModel.loadModel("resources/newmodels/heart.obj"))
	{
		glfwTerminate();
		std::system("pause");
		return -1;
	}
#endif //  TEST_DISPLAY_CYM


	// Section2 ׼����ɫ������
	//Shader shader("shader/modelBspline.vertex", "shader/modelBspline.frag");
	//Shader shader("shader/modelBspline.vertex", "shader/triangleTest.frag", "shader/triangleTest.geo");
#ifdef  TEST_DISPLAY_CYM
#ifdef TEST_DISPLAY_CYM_WITHTEX
	Shader shader("shader/model.vertex", "shader/model.frag");
#else
	Shader shader("shader/model.vertex", "shader/model_noTex.frag");
#endif 
#else
	Shader shader("shader/model.vertex", "shader/model.frag");
#endif 
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	//������������
	shader.use();
	// ���ù�Դ���� ���Դ
	GLint lightAmbientLoc = glGetUniformLocation(shader.programId, "light.ambient");
	GLint lightDiffuseLoc = glGetUniformLocation(shader.programId, "light.diffuse");
	GLint lightSpecularLoc = glGetUniformLocation(shader.programId, "light.specular");
	GLint lightPosLoc = glGetUniformLocation(shader.programId, "light.position");
	GLint attConstant = glGetUniformLocation(shader.programId, "light.constant");
	GLint attLinear = glGetUniformLocation(shader.programId, "light.linear");
	GLint attQuadratic = glGetUniformLocation(shader.programId, "light.quadratic");
	glUniform3f(lightAmbientLoc, 0.5f, 0.5f, 0.5f);
	glUniform3f(lightDiffuseLoc, 0.8f, 0.8f, 0.8f);
	glUniform3f(lightSpecularLoc, 1.0f, 1.0f, 1.0f);
	//lampPos = objModel.light_Pos;
	//lampPos = camera.position;
	//glUniform3f(lightPosLoc, lampPos.x, lampPos.y, lampPos.z);
	// ����˥��ϵ��
	glUniform1f(attConstant, 1.0f);
	glUniform1f(attLinear, 0.0f);
	glUniform1f(attQuadratic, 0.0f);


	glUseProgram(0);

	Shader boundboxShader("shader/boundbox.vertex", "shader/boundbox.frag");
	Shader selectedPointShader("shader/selectedPoint.vertex", "shader/selectedPoint.frag");
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glPointSize(point_size);

	//Shader bsplineComputeShader("shader/bsplineModel.comp");
	//Shader tessellationComputeShader("shader/tessellation.comp");
	/*
	std::vector<Shader> computeShaders = std::vector<Shader>();
	Shader cpComputeShader("shader/centroid_coordinates_interpolation.comp");
	Shader bsplineBaseComputeShader("shader/bsplineBase.comp");
	computeShaders.push_back(cpComputeShader);
	computeShaders.push_back(bsplineBaseComputeShader);
	*/

#ifndef TEST_DISPLAY_CYM
	objModel.setComputeData();
#endif // !TEST_DISPLAY_CYM

	

	
#ifdef  OUTPUT_GROUNDTRUTH_COMPAREFILE
	CYMResult cymResult = CYMResult();
	cymResult.output_groundTruthCompareFile(CutScheme::cut_space, CutScheme::smoothSharpAngle);
	return 1;
#endif //  OUTPUT_GROUNDTRUTH_COMPAREFILE

	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		glfwPollEvents(); // ����������� ���̵��¼�
		do_movement(); // �����û�������� �����������

					   // �����ɫ������ ����Ϊָ����ɫ
		//glClearColor(0.18f, 0.04f, 0.14f, 1.0f);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		shader.use();
		// ���ù۲���λ��
		GLint viewPosLoc = glGetUniformLocation(shader.programId, "viewPos");
		glUniform3f(viewPosLoc, camera.position.x, camera.position.y, camera.position.z);

		lampPos = camera.position;
		glUniform3f(lightPosLoc, lampPos.x, lampPos.y, lampPos.z);

		 projection = glm::perspective(camera.mouse_zoom, (GLfloat)(WINDOW_WIDTH) / WINDOW_HEIGHT, 1.0f, 100.0f); // ͶӰ����
		glUniformMatrix4fv(glGetUniformLocation(shader.programId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		 view = camera.getViewMatrix(); // �ӱ任����
		glUniformMatrix4fv(glGetUniformLocation(shader.programId, "view"),1, GL_FALSE, glm::value_ptr(view));
		 model=camera.getModelMatrix();//ģ�;���
		glUniformMatrix4fv(glGetUniformLocation(shader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glUseProgram(0);
#ifdef TEST_DISPLAY_CYM
		cym_result.draw(shader);
#else
		objModel.drawModel(shader);
#endif // TEST_DISPLAY_CYM

		

		glBindVertexArray(0);
		glUseProgram(0);
#ifndef TEST_DISPLAY_CYM
		if (show_boundbox) {
			boundboxShader.use();
			glUniformMatrix4fv(glGetUniformLocation(boundboxShader.programId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(boundboxShader.programId, "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(boundboxShader.programId, "model"),
				1, GL_FALSE, glm::value_ptr(model));
			GLint colorLoc = glGetUniformLocation(boundboxShader.programId, "colorInput");
			glUniform3f(colorLoc, 65, 105, 225);
			objModel.drawBoundbox(boundboxShader);
			glBindVertexArray(0);
			glUseProgram(0);
	}
		if (System_mode::deformation_mode == Deformation_mode::dffd_mode&&selectedPointInit) {
			selectedPointShader.use();
			glUniformMatrix4fv(glGetUniformLocation(selectedPointShader.programId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(selectedPointShader.programId, "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(selectedPointShader.programId, "model"),
				1, GL_FALSE, glm::value_ptr(model));
			objModel.drawSelectedPoint(selectedPointShader);
			glBindVertexArray(0);
			glUseProgram(0);
		}
#endif // !TEST_DISPLAY_CYM

		
		glfwSwapBuffers(window); 
	}
	// �ͷ���Դ
	glfwTerminate();
	
	return 0;
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keyPressedStatus[key] = true;
		else if (action == GLFW_RELEASE)
			keyPressedStatus[key] = false;
	}
	
	if(action== GLFW_PRESS)
	switch (key)
	{
	case GLFW_KEY_1:
		if (System_mode::draw_mode == Draw_mode::normal_mode) {
			System_mode::draw_mode = mesh_mode;
			std::cout << "mode switch to mesh_mode" << std::endl;
		}
		else {
			System_mode::draw_mode = normal_mode;
			std::cout << "mode switch to normal_mode" << std::endl;
		}
		break;

	case GLFW_KEY_CAPS_LOCK:
		if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPosCallback(window, mouse_move_callback_blank);
		}		
		else {
			glfwSetCursorPos(window, lastX, lastY);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetCursorPosCallback(window, mouse_move_callback);
		}			
		break;

	case GLFW_KEY_2:
		point_size += 5;
		point_size = (point_size > point_size_max ? point_size_max : point_size);
		glPointSize(point_size);
		break;

	case GLFW_KEY_3:
		point_size -= 5;
		point_size = (point_size < point_size_min ? point_size_min : point_size);
		glPointSize(point_size);
		break;
	case GLFW_KEY_4:		 
		if (System_mode::deformation_mode == Deformation_mode::boundbox_mode) {
			System_mode::deformation_mode = Deformation_mode::dffd_mode;
			std::cout << "deformation mode switch to dffd_mode" << std::endl;
		}
		else {
			System_mode::deformation_mode = Deformation_mode::boundbox_mode;
			std::cout << "deformation mode switch to boundbox_mode" << std::endl;
		}
		break;
	case GLFW_KEY_5:
		objModel.boundbox->bspinebody->recoverControlPoints();
		objModel.boundbox->setSelectedPoint(objModel.boundbox->selectedPoint_original);
		objModel.boundbox->setupSeletedPointBuffer();
		selectedPointsCanChange = true;
		objModel.boundbox->bspinebody->setupBuffer();
		objModel.boundbox->bspinebody->controlPoints_needUpdate = true;
		break;
	case GLFW_KEY_6:
		show_boundbox = !show_boundbox;
		break;
	case GLFW_KEY_7:
		objModel.boundbox->outputControlPoints();
		std::cout << "�����ļ��������" << std::endl;
		break;
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, GL_TRUE);
		break;

	
	default:
		break;
	}

}
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	//std::cout << xpos << "," << ypos << std::endl;
	if (firstMouseMove) // �״�����ƶ�
	{
		lastX = xpos;
		lastY = ypos;
		firstMouseMove = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;
	camera.handleMouseMove(xoffset, yoffset);
	lastX = xpos;
	lastY = ypos;
	/*
	//һ��ֻ����һ�������ת	
	float xoffset_abs = abs(xoffset);
	float yoffset_abs = abs(yoffset);
	if (xoffset_abs == yoffset_abs) {}
	else if (xoffset_abs > yoffset_abs) {
		camera.handleMouseMove(xoffset, 0);
		lastX = xpos;
	}		
	else {
		camera.handleMouseMove(0, yoffset);
		lastY = ypos;
	}		
	*/
}
void mouse_move_callback_blank(GLFWwindow* window, double xpos, double ypos) {
	if (System_mode::deformation_mode == Deformation_mode::boundbox_mode) {
		int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		if (state == GLFW_PRESS) {
			glm::vec3 mouse_pos = Get3Dpos(xpos, ypos, model, view, projection, WINDOW_WIDTH, WINDOW_HEIGHT);
			glPointSize(point_size_max);
			cursor_press_leave = true;
			if (!has_select) {
				glm::vec4 clip_position = projection*view*model*glm::vec4(mouse_pos, 1.0f);
				if (clip_position.w != 0)
					clip_position = clip_position / clip_position.w;
				pickedPoint_z = clip_position.z;

				point_select = getNearestPoint(mouse_pos, objModel.boundbox->bspinebody);
				objModel.boundbox->bspinebody->controlPoints[point_select.index_x][point_select.index_y][point_select.index_z].x = mouse_pos.x;
				objModel.boundbox->bspinebody->controlPoints[point_select.index_x][point_select.index_y][point_select.index_z].y = mouse_pos.y;
				objModel.boundbox->bspinebody->controlPoints[point_select.index_x][point_select.index_y][point_select.index_z].z = mouse_pos.z;

				objModel.boundbox->bspinebody->setupBuffer();
				objModel.boundbox->bspinebody->controlPoints_needUpdate = true;
				has_select = true;
			}
			else {
				mouse_pos = Get3Dpos_withZ(xpos, ypos, pickedPoint_z, model, view, projection, WINDOW_WIDTH, WINDOW_HEIGHT);
				objModel.boundbox->bspinebody->controlPoints[point_select.index_x][point_select.index_y][point_select.index_z].x = mouse_pos.x;
				objModel.boundbox->bspinebody->controlPoints[point_select.index_x][point_select.index_y][point_select.index_z].y = mouse_pos.y;
				objModel.boundbox->bspinebody->controlPoints[point_select.index_x][point_select.index_y][point_select.index_z].z = mouse_pos.z;
				objModel.boundbox->bspinebody->setupBuffer();
				objModel.boundbox->bspinebody->controlPoints_needUpdate = true;
			}
		}
		else {
			if (cursor_press_leave) {
				glPointSize(point_size_min + 6.0f);
				cursor_press_leave = false;
			}
			has_select = false;
		}
	}//��Χ�п���ģʽ
	else if (System_mode::deformation_mode == Deformation_mode::dffd_mode) {
		if (selectedPointsCanChange) {//ѡ��Լ����
			int left_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
			int right_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
			if (right_state == GLFW_PRESS) {
				glm::vec3 mouse_pos = Get3Dpos(xpos, ypos, model, view, projection, WINDOW_WIDTH, WINDOW_HEIGHT);
				/*
				glm::vec3 touch_position_near = Get3Dpos_withZ(xpos, ypos, 1, model, view, projection, WINDOW_WIDTH, WINDOW_HEIGHT);
				glm::vec3 touch_position_far = Get3Dpos_withZ(xpos, ypos, -1, model, view, projection, WINDOW_WIDTH, WINDOW_HEIGHT);
				
				//���������ƽ�й���(z��)
				if (touch_position_near.x == touch_position_far.x&&touch_position_near.y == touch_position_far.y)
					return;
				else {
					if (objModel.getNearestPoint_AmongControPoints(touch_position_near, touch_position_far, objModel.boundbox->bspinebody, point_select)) {
						//����ü��ռ����ȣ������������ȡλ��ʹ��
						glm::vec4 clip_position = projection*view*model*glm::vec4(point_select.pos, 1.0f);
						if (clip_position.w != 0)
							clip_position = clip_position / clip_position.w;
						pickedPoint_z = clip_position.z;

						Vertex v = Vertex();
						v.position = glm::vec4(point_select.pos, 0);
						objModel.boundbox->selectedPoint_original = Vertex();
						objModel.boundbox->setSelectedPoint(v);
						objModel.boundbox->setupSeletedPointBuffer();
						selectedPointInit = true;
					}

					else//��������������̫Զ���Ҳ����ʺϵ�Ŀ���
						return;
				}
				*/

				glm::vec4 clip_position = projection*view*model*glm::vec4(mouse_pos, 1.0f);
				if (clip_position.w != 0)
					clip_position = clip_position / clip_position.w;
				pickedPoint_z = clip_position.z;
				Vertex v = objModel.getNearestPoint(mouse_pos.x, mouse_pos.y, mouse_pos.z);
				//std::cout << v.position.x << "," << v.position.y << "," << v.position.z << std::endl;
				objModel.boundbox->selectedPoint_original = v;
				objModel.boundbox->setSelectedPoint(v);
				objModel.boundbox->setupSeletedPointBuffer();
				selectedPointInit = true;
				
			}
			if (left_state == GLFW_PRESS) {//˵����ѡ���
				selectedPointsCanChange = false;
				std::cout << "Լ����ѡ�����" << std::endl;
			}
		}
		else {//��ѡ��Լ����
			int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
			if (state == GLFW_PRESS) {
				//glm::vec3 mouse_pos = Get3Dpos(xpos, ypos, model, view, projection, WINDOW_WIDTH, WINDOW_HEIGHT);
				glm::vec3 mouse_pos = Get3Dpos_withZ(xpos, ypos, pickedPoint_z, model, view, projection, WINDOW_WIDTH, WINDOW_HEIGHT);

				float dx = (mouse_pos.x - objModel.boundbox->selectedPoint.position.x) ;
				float dy = (mouse_pos.y - objModel.boundbox->selectedPoint.position.y) ;
				float dz = (mouse_pos.z - objModel.boundbox->selectedPoint.position.z) ;				
				objModel.boundbox->dffd_adjustControlPoints(dx,dy,dz);
				objModel.boundbox->setupSeletedPointBuffer();
				objModel.boundbox->bspinebody->controlPoints_needUpdate = true;
			}
		}
	}//dffdģʽ
	
		
}
// ����������ദ�������ֿ���
void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.handleMouseScroll(yoffset);
}
// ����������ദ����̿���
void do_movement()
{
	if (keyPressedStatus[GLFW_KEY_W])
		camera.handleKeyPress(FORWARD, deltaTime);
	if (keyPressedStatus[GLFW_KEY_S])
		camera.handleKeyPress(BACKWARD, deltaTime);
	if (keyPressedStatus[GLFW_KEY_A])
		camera.handleKeyPress(LEFT, deltaTime);
	if (keyPressedStatus[GLFW_KEY_D])
		camera.handleKeyPress(RIGHT, deltaTime);
	if (keyPressedStatus[GLFW_KEY_E])
		camera.handleKeyPress(UP, deltaTime);
	if (keyPressedStatus[GLFW_KEY_Q])
		camera.handleKeyPress(DOWN, deltaTime);
}

void loadFile_cutScheme() {
	CutScheme::init();
	ifstream infilestream;
	/*
	vector<int> nums_triangles_cutScheme= vector<int>();
	infilestream.open("cutOutputFile/nums_triangle.txt");
	for (int i = 0; i < CutScheme::MAX_CUTLENGTH*CutScheme::MAX_CUTLENGTH*CutScheme::MAX_CUTLENGTH; i++) {
		int temp;
		infilestream >> temp;
		nums_triangles_cutScheme.push_back(temp);
	}
	infilestream.close();
	*/
	int index = 0;
	for (int i = 0; i < CutScheme::MAX_CUTLENGTH; i++) {
		for (int j = 0; j < CutScheme::MAX_CUTLENGTH; j++) {
			for (int k = 0; k < CutScheme::MAX_CUTLENGTH; k++) {
				//CutScheme::nums_trianglesAccumlation_cutScheme.push_back(total);
				if (!(i + j + 1<k || i + k + 1<j || k + j + 1<i)) {
					infilestream.open("cutOutputFile/" + std::to_string(i+1) + "_" + std::to_string(j+1) + "_" + std::to_string(k+1) + ".txt");
					int nums_vertexs;
					infilestream >> nums_vertexs;
					DelaunayResult temp= DelaunayResult();
					
					for (int x = 0; x < nums_vertexs; x++) {
						float tempX, tempY, tempZ;
						infilestream >> tempX;
						infilestream >> tempY;
						infilestream >> tempZ;//һ����������
						temp.vector_positions.push_back(glm::vec3(tempX, tempY, tempZ));
					}
					/*
					for (int x = 0; x < nums_triangles_cutScheme[index]; x++) {
						int tempX, tempY, tempZ;
						infilestream >> tempX;
						infilestream >> tempY;
						infilestream >> tempZ;//һ�������εĶ����±�
						temp.vector_indices.push_back(tempX);
						temp.vector_indices.push_back(tempY);
						temp.vector_indices.push_back(tempZ);
					}
					*/
					int tempInd;
					while (infilestream >> tempInd) {
						temp.vector_indices.push_back(tempInd);
					}
					CutScheme::cutScheme.push_back(temp);
					infilestream.close();
				}
				index++;
			}
		}
	}

}

