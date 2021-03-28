#include "cutScheme.h"

//std::vector<int> CutScheme::nums_triangles_cutScheme;//切割方案三角形数
//std::vector<int> CutScheme::nums_trianglesAccumlation_cutScheme;//切割方案三角形累加数（用于查找对应的切割方案）
std::vector<DelaunayResult> CutScheme::cutScheme;//切割方案
int CutScheme::scheme_index[20][20][20];//0基
float CutScheme::cut_space;
float CutScheme::smoothSharpLimit;
int CutScheme::smoothSharpAngle;
int CutScheme::MAX_CUTLENGTH;
int CutScheme::tessellation_times;

void CutScheme::init() {
		//nums_triangles_cutScheme = std::vector<int>();
		//nums_trianglesAccumlation_cutScheme = std::vector<int>();
		cutScheme = std::vector<DelaunayResult>();
		MAX_CUTLENGTH = 12;
		int temp_index = 0;
		for (int i = 0; i < MAX_CUTLENGTH; i++) {
			for (int j = 0; j < MAX_CUTLENGTH; j++) {
				for (int k = 0; k < MAX_CUTLENGTH; k++) {
					scheme_index[i][j][k] = temp_index;
					temp_index++;
					if (i + j+1 < k || i + k+1 < j || k + j+1 < i)
						temp_index--;
				}
			}
		}
	
		cut_space = 0.035f;
		//cut_space = 1000.1f;

		tessellation_times = 5;

		smoothSharpAngle = 90;

		smoothSharpLimit = cos(smoothSharpAngle*3.141592f/180.0f);//尖锐阈值 

}