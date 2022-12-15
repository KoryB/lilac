#ifndef LILAC_SHADER_H
#define LILAC_SHADER_H

#include <Lilac/OpenGL.h>

#include <string>

namespace Lilac
{
class Shader
{
public:
	// TODO: Consider making these things virtual
	GLuint getRawHandle() const;

	// TODO: Get hot recompilation working
	void compileShader(const std::string& source);

	// TODO: make constructor private (I think?)
	// TODO: make destructor

protected:
	GLuint m_handle;
};

class ComputeShader : public Shader
{
public:
	ComputeShader(const std::string& source);
};

class VertexShader : public Shader
{
public:
	VertexShader(const std::string& source);
};

class FragmentShader : public Shader
{
public:
	FragmentShader(const std::string& source);
};
}


#endif // LILAC_SHADER_H