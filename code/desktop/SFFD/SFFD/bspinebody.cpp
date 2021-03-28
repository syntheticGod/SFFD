#include "boundbox.h"
#include "bspinebody.h"
#include "globalGPUTest.h"

Bspinebody::Bspinebody() :is_valid(true), has_init(false) {
	//memset(controlPointNums, 5, sizeof(controlPointNums));
	//memset(degree, 3, sizeof(degree));
	controlPointNums[0] = 5;
	controlPointNums[1] = 5;
	controlPointNums[2] = 5;
	degree[0] = 3;
	degree[1] = 3;
	degree[2] = 3;
	indices = std::vector<GLuint>();
	indices_vectex = std::vector<GLuint>();
	base[0] = BsplineBase();
	base[1] = BsplineBase();
	base[2] = BsplineBase();
	
}
Bspinebody::Bspinebody(unsigned int cx, unsigned int cy, unsigned int cz, unsigned int dx, unsigned int dy, unsigned int dz) :has_init(false) {
	controlPointNums[0] = cx;
	controlPointNums[1] = cy;
	controlPointNums[2] = cz;
	degree[0] = dx;
	degree[1] = dy;
	degree[2] = dz;
	if (controlPointNums[0] < degree[0] + 1 || controlPointNums[1] < degree[1] + 1 || controlPointNums[2] < degree[2] + 1)
		is_valid = false;
	indices = std::vector<GLuint>();
	indices_vectex = std::vector<GLuint>();
	base[0] = BsplineBase();
	base[1] = BsplineBase();
	base[2] = BsplineBase();
}

//长方体
int Bspinebody::InitBspinebody(Boundbox* box) {

	if (!is_valid || !box->hasInit)
		return 0;

	controlPoints_needUpdate = false;
	std::cout << "三维度最小值"<<box->min[0] << "," << box->min[1] << "," << box->min[2]  << std::endl;
	std::cout << "三维度最大值"<<box->max[0] << "," << box->max[1] << "," << box->max[2] << std::endl;
	for (int d = 0; d < 3; d++) {
		//单维度贝塞尔曲线的情况，均匀分布
		if (controlPointNums[d] == degree[d] + 1) {
			float space = (box->max[d] - box->min[d]) / degree[d];
			for (int i = 0; i < controlPointNums[d]; i++)
				controlPoints_singleDimention[d][i] = box->min[d] + space*i;
			//初始化节点向量
			for (int i = 0; i < degree[d]; i++) {
				knotVector[d][i] = controlPoints_singleDimention[d][0];
			}
			for (int i = degree[d]; i <controlPointNums[d]; i++) {
				knotVector[d][i] = controlPoints_singleDimention[d][0] + (i - degree[d])*(controlPoints_singleDimention[d][controlPointNums[d] - 1] - controlPoints_singleDimention[d][0]) / (controlPointNums[d] - degree[d]);
			}
			for (int i = controlPointNums[d]; i <= controlPointNums[d] + degree[d]; i++) {
				knotVector[d][i] = controlPoints_singleDimention[d][controlPointNums[d] - 1];
			}
		}
		else {
			int space_nums = 0;//分段k的和
			if ((controlPointNums[d] - 1) % 2 == 0) {
				int half_nums = (controlPointNums[d] - 1) / 2;
				for (int i = 1; i <= half_nums; i++) {
					if (i < degree[d])
						space_nums += i;
					else
						space_nums += degree[d];
				}
				space_nums *= 2;
			}
			else {
				int half_nums = (controlPointNums[d] - 2) / 2;
				for (int i = 1; i <= half_nums; i++) {
					if (i < degree[d])
						space_nums += i;
					else
						space_nums += degree[d];
				}
				space_nums *= 2;
				if (half_nums < degree[d])
					space_nums += half_nums;
				else
					space_nums += degree[d];

			}

			float space = (box->max[d] - box->min[d]) / space_nums;
			int left_index = 1;
			int right_index = controlPointNums[d] - 2;
			controlPoints_singleDimention[d][0] = box->min[d];
			controlPoints_singleDimention[d][controlPointNums[d] - 1] = box->max[d];
			int tempIndex = 1;
			while (left_index <= right_index) {
				controlPoints_singleDimention[d][left_index] = controlPoints_singleDimention[d][left_index-1] + space*tempIndex;
				controlPoints_singleDimention[d][right_index] = controlPoints_singleDimention[d][right_index+1] - space*tempIndex;
				if (tempIndex<degree[d])
					tempIndex++;
				left_index++;
				right_index--;
			}
			//初始化节点向量
			for (int i = 0; i < degree[d]; i++) {
				knotVector[d][i] = controlPoints_singleDimention[d][0];
			}
			for (int i = degree[d]; i <controlPointNums[d]; i++) {
				knotVector[d][i] = controlPoints_singleDimention[d][0] + (i - degree[d])*(controlPoints_singleDimention[d][controlPointNums[d] - 1] - controlPoints_singleDimention[d][0]) / (controlPointNums[d] - degree[d]);
			}
			for (int i = controlPointNums[d]; i <= controlPointNums[d] + degree[d]; i++) {
				knotVector[d][i] = controlPoints_singleDimention[d][controlPointNums[d] - 1];
			}

		}
	}

	//将各维度单维度坐标赋予所有控制点，以及初始化局部坐标（参数域）
	for (int x = 0; x < controlPointNums[0]; x++) {
		for (int y = 0; y < controlPointNums[1]; y++) {
			for (int z = 0; z < controlPointNums[2]; z++) {
				controlPoints[x][y][z] = glm::vec4(controlPoints_singleDimention[0][x], controlPoints_singleDimention[1][y], controlPoints_singleDimention[2][z],0);
				controlPoints_original[x][y][z] = controlPoints[x][y][z];
			}
		}
	}
	

	//建立索引表（线框）
	for (int y = 0; y < controlPointNums[1]; y++) {
		for (int z = 0; z < controlPointNums[2]; z++) {
			for (int x = 0; x < controlPointNums[0]; x++) {
				int tempIndex = x*10 * 10 + y*10 + z;//表的实际大小是10*10*10

				indices.push_back(tempIndex);
				if(x!=0&&x!= controlPointNums[0]-1)
					indices.push_back(tempIndex);
			}
		}
	}

	for (int x = 0; x < controlPointNums[0]; x++) {
		for (int z = 0; z < controlPointNums[2]; z++) {
			for (int y = 0; y < controlPointNums[1]; y++) {
				int tempIndex = x*10 * 10 + y*10 + z;
				indices.push_back(tempIndex);
				if (y != 0 && y != controlPointNums[1] - 1)
					indices.push_back(tempIndex);
			}
		}
	}

	for (int x = 0; x < controlPointNums[0]; x++) {
		for (int y = 0; y < controlPointNums[1]; y++) {
			for (int z = 0; z < controlPointNums[2]; z++) {
				int tempIndex = x*10 * 10 + y*10 + z;
				indices.push_back(tempIndex);
				if (z != 0 && z != controlPointNums[2] - 1)
					indices.push_back(tempIndex);
			}
		}
	}

	//建立索引表（线框的顶点）
	for (int x = 0; x < controlPointNums[0]; x++) {
		for (int y = 0; y < controlPointNums[1]; y++) {
			for (int z = 0; z < controlPointNums[2]; z++) {
				int tempIndex = x * 10 * 10 + y * 10 + z;
				indices_vectex.push_back(tempIndex);
			}
		}
	}
	//计算B样条基

	for (int i = 0; i < 3; i++) {
		base[i].setKnotVec(knotVector[i]);
	}
	
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < controlPointNums[i]; j++) {
			for (int k = degree[i]; k <= controlPointNums[i]; k++) {
				base[i].calBase(j, degree[i]+1, k);
			}			
		}	
	}

	//adjust();

	//更新差分控制顶点
	cal_controlPoints_derivative();

	setupBuffer();
	has_init = true;
	return 1;
}

void Bspinebody::adjust() {

	/*茶壶用
	for (int i = 0; i < controlPointNums[0]; i++)
		for (int j = 0; j < controlPointNums[1]; j++)
			for (int k = 0; k < controlPointNums[2]; k++) {
				
				if (i<2)
					controlPoints[i][j][k].z += 1.1f/3.2f*(3 - i);
				else if(i==2)
					controlPoints[i][j][k].z += 0.5/3.2f * (3 - i);
				else if (i>3)
					controlPoints[i][j][k].z += 1/3.2f*(i - 3);
			}	
	*/
				

	/*
	//花瓶用
	for (int i = 0; i < controlPointNums[0]; i++)
		for (int j = 0; j < controlPointNums[1]; j++)
			for (int k = 0; k < controlPointNums[2]; k++) {

				if (j == 4) {
					controlPoints[i][j][k].x *= 1.5;			
					if (i == 3) 
						controlPoints[i][j][k].z *= 2.1;
					
				}						
			}
	*/
	/*
	//爱心用
	for (int i = 0; i < controlPointNums[0]; i++)
		for (int j = 0; j < controlPointNums[1]; j++)
			for (int k = 0; k < controlPointNums[2]; k++) {

				if (i<2)
					controlPoints[i][j][k].z -= 1.1f / 3.2f*(3 - i);
				else if (i == 2)
					controlPoints[i][j][k].z -= 0.5 / 3.2f * (3 - i);
				else if (i>3)
					controlPoints[i][j][k].z -= 1 / 3.2f*(i - 3);
			}
	*/
	/*
	//桌子
	for (int i = 0; i < controlPointNums[0]; i++)
		for (int j = 0; j < controlPointNums[1]; j++)
			for (int k = 0; k < controlPointNums[2]; k++) {

				if (j<2)
					controlPoints[i][j][k].x += 0.2f / 3.2f*(3 - i);
				else if (j == 2)
					controlPoints[i][j][k].x += 1.5f / 3.2f * (3 - i);
				else if (j>3)
					controlPoints[i][j][k].x += 0.2f / 3.2f*(i - 3);

				if(i==2)
					controlPoints[i][j][k].y += 0.7f ;
			}
	*/
	
	//椅子
	for (int i = 0; i < controlPointNums[0]; i++)
		for (int j = 0; j < controlPointNums[1]; j++)
			for (int k = 0; k < controlPointNums[2]; k++) {

				if (k == 0 && j != 2) {
					if(j<2)
					    controlPoints[i][j][k].z -= 0.4f*(2-j);
					else
						controlPoints[i][j][k].z -= 0.4f*(j-2);
				}					
				else if(k>=3&&j<2){
					controlPoints[i][j][k].z += 0.3f*(2 - j);
				}					
			}
      
	/*
	//玩具兔子
	for (int i = 0; i < controlPointNums[0]; i++)
		for (int j = 0; j < controlPointNums[1]; j++)
			for (int k = 0; k < controlPointNums[2]; k++) {

				if (j == 2 && i != 2) {
					if(i>2)
						controlPoints[i][j][k].y -= 0.3f*(i-2);
					else if (i<2)
						controlPoints[i][j][k].y -= 0.3f*(2-i);
				}
				else if (j >2) {							
						controlPoints[i][j][k].x += 0.15f*i;			
				}
			}
	*/
}
void Bspinebody::draw(const Shader& shader) const {
	if (has_init) {
		shader.use();
		glBindVertexArray(VAOId[0]);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(2.0f);
		glDrawElements(GL_LINES, this->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glUseProgram(0);


		//接着画点
		shader.use();
		GLint colorLoc = glGetUniformLocation(shader.programId, "colorInput");
		glUniform3f(colorLoc, 255, 0, 0);
		glBindVertexArray(VAOId[1]);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		
		glDrawElements(GL_POINTS, this->indices_vectex.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glUseProgram(0);
		//glPointSize(11.0f);
	}
}

void Bspinebody::setupBuffer()  // 建立VAO,VBO等缓冲区
{
	glGenVertexArrays(2, VAOId);
	glGenBuffers(1, &this->VBOId);
	glGenBuffers(2, EBOId);

	glBindVertexArray(this->VAOId[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * 1000,
		controlPoints, GL_STATIC_DRAW);
	// 顶点位置属性
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
		0, (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// 索引数据
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOId[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* this->indices.size(),
		&this->indices[0], GL_STATIC_DRAW);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	/***********************************************************/
	//点的数据
	glBindVertexArray(this->VAOId[1]);
	// 顶点位置属性
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
		0, (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// 索引数据
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOId[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* this->indices_vectex.size(),
		&this->indices_vectex[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
}

void Bspinebody::cal_controlPoints_derivative() {
	controlPoints_derivative_x = std::vector<glm::vec4>();
	controlPoints_derivative_y = std::vector<glm::vec4>();
	controlPoints_derivative_z = std::vector<glm::vec4>();

	for (int i = 0; i < controlPointNums[0]-1; i++) {
		float left = (float)degree[0] / (knotVector[0][i + degree[0]+ 1]-knotVector[0][i+1]);

		for (int j = 0; j < controlPointNums[1]; j++) {
			for (int k = 0; k < controlPointNums[2]; k++) {
				glm::vec4 temp_result = left*(controlPoints[i + 1][j][k] - controlPoints[i][j][k]);
				controlPoints_derivative_x.push_back(temp_result);
			}
		}
	}
	for (int i = 0; i < controlPointNums[1] - 1; i++) {
		float left = (float)degree[1] / (knotVector[1][i + degree[1] + 1] - knotVector[1][i + 1]);

		for (int j = 0; j < controlPointNums[0]; j++) {
			for (int k = 0; k < controlPointNums[2]; k++) {
				glm::vec4 temp_result = left*(controlPoints[j][i + 1][k] - controlPoints[j][i][k]);
				controlPoints_derivative_y.push_back(temp_result);
			}
		}
	}
	for (int i = 0; i < controlPointNums[2] - 1; i++) {
		float left = (float)degree[2] / (knotVector[2][i + degree[2] + 1] - knotVector[2][i + 1]);

		for (int j = 0; j < controlPointNums[0]; j++) {
			for (int k = 0; k < controlPointNums[1]; k++) {
				glm::vec4 temp_result = left*(controlPoints[j][k][i + 1] - controlPoints[j][k][i]);
				controlPoints_derivative_z.push_back(temp_result);
			}
		}
	}
}

void Bspinebody::recoverControlPoints() {
	if (has_init) {
		for (int i = 0; i < controlPointNums[0]; i++)
			for (int j = 0; j < controlPointNums[1]; j++)
				for (int k = 0; k < controlPointNums[2]; k++)
					controlPoints[i][j][k] = controlPoints_original[i][j][k];
	}

}

