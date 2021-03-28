#pragma once
#include<string.h>
#include <cmath>
#include <iostream>
#define MAX_DEGREE 5 //����������Ϊ5�������ƶ���������ά�ȣ�Ϊ16

using namespace std;
struct Polynomial
{
	int maxDegree;//��ߴ���t^max����ʹ���Ĭ��Ϊ0����1
	float data[MAX_DEGREE+1];//����������Ϊ5����λ��Ӧ�ʹ���,��0λ��Ӧ0�η�����1
	Polynomial():maxDegree(0){
		for (int i = 0; i <= MAX_DEGREE; i++)
			data[i] = 0;
	}
	Polynomial(int maxD, float dat[MAX_DEGREE+1]) :maxDegree(maxD) {
		//memcpy(data, dat, sizeof(dat));
		for (int i = 0; i <= MAX_DEGREE; i++)
			data[i] = dat[i];
	}
	Polynomial(const Polynomial & p)
	{
		maxDegree = p.maxDegree;
		memcpy(data, p.data, sizeof(p.data));
	}


	//����һ��t����һ�ף���ֻ��С�����ʱ����
	void plusT() {
		if (maxDegree < MAX_DEGREE) {
			for (int i = maxDegree; i >= 0; i--) {
				data[i + 1] = data[i];
			}
			data[0] = 0;
			maxDegree++;
		}		
	}

	Polynomial operator+(const Polynomial& right) {
		for (int i = 0; i <= MAX_DEGREE; i++) 
			this->data[i] += right.data[i];	
		this->maxDegree = (this->maxDegree > right.maxDegree ? this->maxDegree : right.maxDegree);
		return *this;
	}
	Polynomial operator-(const Polynomial& right) {
		for (int i = 0; i <= MAX_DEGREE; i++)
			this->data[i] -= right.data[i];
		this->maxDegree = (this->maxDegree > right.maxDegree ? this->maxDegree : right.maxDegree);
		return *this;
	}
	Polynomial operator*(const float num) {
		for (int i = 0; i <=this->maxDegree; i++)
			this->data[i] *= num;
		return *this;
	}
	Polynomial operator/(const float num) {
		for (int i = 0; i <=this->maxDegree; i++)
			if(num!=0)
			    this->data[i] /= num;
		return *this;
	}

	//�������ʽ��ֵ
	float cal_Polynomial(float t)const {
		float result=0;
		for (int i = 0; i <= this->maxDegree; i++) {
			result += this->data[i]*pow(t, i);
		}
		return result;
	}
};



class BsplineBase{
public:
	float* knot;
	bool has_cal[16][7][16];//i,k,seg����ΧΪk+n-1,k,k+n-1�����ޣ�0����
	Polynomial result[16][7][16];

	BsplineBase() {
		for (int i = 0; i < 16; i++)
			for (int j = 0; j < 7; j++)
				for (int k = 0; k < 16; k++) {
					has_cal[i][j][k] = false;
					result[i][j][k] = Polynomial();
				}				
	}
	void setKnotVec(float* k) {
		knot = k;
	}
	//segment���ο�ʼ�Ľڵ��±�����,k>=1
	Polynomial calBase(int i,int k,int segment) {//ֻ��k����0��,k�ǽ�����������+1
		if (has_cal[i][k][segment])
			return result[i][k][segment];
		if (k == 1) {
			if (segment == i) {
				float temp[MAX_DEGREE+1] = { 1,0,0,0,0,0 };
				result[i][1][segment] =  Polynomial(0, temp);
				has_cal[i][1][segment] = true;
			}
			else {
				float temp[MAX_DEGREE+1] = { 0,0,0,0,0,0 };
				result[i][1][segment] = Polynomial(0, temp);
				has_cal[i][1][segment] = true;
			}
			//cout << "C" << i << 1 << segment<< "="<< result[i][1][segment].data[0] << endl;
			return result[i][k][segment];
		}
		else {//����һ��
			//���ĸ
			float left_Denominator = knot[i + k-1] - knot[i];
			//�����(���ֵ����
			float left_numerator_num = -knot[i];
			float right_Denominator= knot[i + k] - knot[i+1];
			float right_numerator_num = knot[i + k ];

			Polynomial left_reslut= Polynomial();
			Polynomial right_reslut = Polynomial();
			if (left_Denominator != 0) {
				Polynomial temp_1 = calBase(i, k - 1, segment);
				Polynomial temp_2 = temp_1;
				temp_1 = temp_1 / left_Denominator;
				temp_1.plusT();
				temp_2 = temp_2 / left_Denominator*left_numerator_num;
				left_reslut = temp_1 + temp_2;
			}

			if (right_Denominator != 0) {
				Polynomial temp_1 = calBase(i+1, k - 1, segment);
				Polynomial temp_2 = temp_1;
				temp_1 = temp_1 / right_Denominator;
				temp_1.plusT();
				temp_1 = temp_1*(-1);
				temp_2 = temp_2 / right_Denominator*right_numerator_num;
				right_reslut = temp_1 + temp_2;
			}

			result[i][k][segment] = left_reslut + right_reslut;

			/*
			 cout << "C" << i << k << segment<<"=" ;
			 for (int j = 0; j <= MAX_DEGREE; j++) {
				 cout << result[i][k][segment].data[j] << "," ;
			 }
			 cout << endl;
			 */
			has_cal[i][k][segment] = true;
			return result[i][k][segment];
		}
	}
	
};