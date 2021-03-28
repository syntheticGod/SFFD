#pragma once
#include <glm\glm.hpp>
#include <vector>
#include "teapotData.h"

struct Vertex;
typedef glm::vec3 Vec3f;

Vec3f evalBezierCurve(const Vec3f *P, const float &t);
Vec3f evalBezierPatch(const Vec3f *controlPoints, const float &u, const float &v);
Vec3f derivBezier(const Vec3f *P, const float &t);
Vec3f dUBezier(const Vec3f *controlPoints, const float &u, const float &v);
Vec3f dVBezier(const Vec3f *controlPoints, const float &u, const float &v);

class Teapot {
public:
	Teapot(){
		hasInited = false;
		divs = 5;
	}
	void init();
	void outputObjFile();

	std::vector<Vertex> vertexList_origin;
	std::vector<int> indicesList_origin;
	//vertex的顺序与BezierSurfaceIndex对应，即对应的原始贝塞尔曲面的下标，用于提取uv
	std::vector<int> BezierSurfaceIndex;
private:
	bool hasInited;
	int divs;//细化段数
};


