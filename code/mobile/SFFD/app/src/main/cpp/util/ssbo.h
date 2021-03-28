#pragma once
#include <GLES3/gl31.h>
#include <../glm/glm.hpp>
#include <vector>
#include "LogUtil.h"



template <typename T>
void setupSSBufferObjectVec(GLuint* ssbo, GLuint index, std::vector<T>& data, bool is_dynamic)
{
    glGenBuffers(1, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(T), data.data(), is_dynamic == true ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, *ssbo);
    //int errorInt=(int)glGetError();
    //LOGCATE("ssbo get error=%d,glIndex=%d",glGetError(),index);

}

template<typename T>
void setupSSBufferObjectList(GLuint* ssbo, GLuint index, T* pIn, GLuint count,bool is_dynamic) {
    glGenBuffers(1, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(T), pIn, is_dynamic == true ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, *ssbo);
    //int errorInt=(int)glGetError();
    //LOGCATE("ssbo get error=%d,glIndex=%d",glGetError(),index);
}


//template<typename T>
//void setupSSBufferObjectListTemp(GLuint& ssbo, GLuint index, bool is_dynamic);//‘⁄∂—…œ…Í«Îø’º‰£¨∑¿÷π’ª“Á≥ˆ
