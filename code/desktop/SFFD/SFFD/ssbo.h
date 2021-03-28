#pragma once
#include <GLEW/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "ssbo.cpp"



template<typename T>
void setupSSBufferObjectVec(GLuint& ssbo, GLuint index, const std::vector<T>& data, bool is_dynamic);

template<typename T>
void setupSSBufferObjectList(GLuint& ssbo, GLuint index, T* pIn, GLuint count,bool is_dynamic);


//template<typename T>
//void setupSSBufferObjectListTemp(GLuint& ssbo, GLuint index, bool is_dynamic);//在堆上申请空间，防止栈溢出
