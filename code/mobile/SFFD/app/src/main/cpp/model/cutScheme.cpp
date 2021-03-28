#include "cutScheme.h"
#include "math.h"
//std::vector<int> CutScheme::nums_triangles_cutScheme;//�и����������
//std::vector<int> CutScheme::nums_trianglesAccumlation_cutScheme;//�и���������ۼ��������ڲ��Ҷ�Ӧ���и����
std::vector<DelaunayResult> CutScheme::cutScheme;//�и��
int CutScheme::scheme_index[10][10][10];//0��
float CutScheme::cut_space;
int CutScheme::MAX_CUTLENGTH;
int CutScheme::tessellation_times;
float CutScheme::smoothSharpLimit;
int CutScheme::smoothSharpAngle;

void CutScheme::init() {
		//nums_triangles_cutScheme = std::vector<int>();
		//nums_trianglesAccumlation_cutScheme = std::vector<int>();
		cutScheme = std::vector<DelaunayResult>();
		MAX_CUTLENGTH = 10;
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
                //�����������ת�������ϸ�Ⱥ��ʷּ��
		cut_space = 0.3f;
		//cut_space = 100.3f;
		tessellation_times = 5;
		smoothSharpAngle = 90;
		smoothSharpLimit = std::cos(smoothSharpAngle*3.141592f/180.0f);//������ֵ
}