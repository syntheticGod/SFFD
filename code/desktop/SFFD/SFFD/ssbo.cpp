#include <GLEW/glew.h>
#include <glm/glm.hpp>
#include <vector>

template <typename T>
void setupSSBufferObjectVec(GLuint& ssbo, GLuint index, std::vector<T>& data, bool is_dynamic)
{
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(T), data.data(), is_dynamic == true ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, ssbo);
}

template<typename T>
void setupSSBufferObjectList(GLuint& ssbo, GLuint index, T* pIn, GLuint count,bool is_dynamic) {
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(T), pIn, is_dynamic == true ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, ssbo);
}

/*
template<typename T>
void setupSSBufferObjectListTemp(GLuint& ssbo, GLuint index, bool is_dynamic) {

	T* data = malloc(sizeof(T) * 30000);
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 30000 * sizeof(T), data, is_dynamic == true ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, ssbo);
	free(data);
}
*/