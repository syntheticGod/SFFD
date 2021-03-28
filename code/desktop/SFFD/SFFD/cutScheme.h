#pragma once
#include <vector>
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

struct DelaunayResult {
	std::vector<glm::vec3> vector_positions;
	std::vector<int> vector_indices;//3��intΪһ�飬��Ӧһ��������
};

class CutScheme {
public:
	//static std::vector<int> nums_triangles_cutScheme;//�и����������
	//static std::vector<int> nums_trianglesAccumlation_cutScheme;//�и���������ۼ��������ڲ��Ҷ�Ӧ���и����
	//static std::vector<glm::vec3> cutScheme;//�и��
	static int scheme_index[20][20][20];//0��
	static std::vector<DelaunayResult> cutScheme;
	static float cut_space;
	static float smoothSharpLimit;
	static int smoothSharpAngle;
	static int MAX_CUTLENGTH;
	static void init();
	static int tessellation_times;	
};