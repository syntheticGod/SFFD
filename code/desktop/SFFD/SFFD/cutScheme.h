#pragma once
#include <vector>
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

struct DelaunayResult {
	std::vector<glm::vec3> vector_positions;
	std::vector<int> vector_indices;//3个int为一组，对应一个三角形
};

class CutScheme {
public:
	//static std::vector<int> nums_triangles_cutScheme;//切割方案三角形数
	//static std::vector<int> nums_trianglesAccumlation_cutScheme;//切割方案三角形累加数（用于查找对应的切割方案）
	//static std::vector<glm::vec3> cutScheme;//切割方案
	static int scheme_index[20][20][20];//0基
	static std::vector<DelaunayResult> cutScheme;
	static float cut_space;
	static float smoothSharpLimit;
	static int smoothSharpAngle;
	static int MAX_CUTLENGTH;
	static void init();
	static int tessellation_times;	
};