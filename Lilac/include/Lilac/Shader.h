#ifndef LILAC_SHADER_H
#define LILAC_SHADER_H

#include <Lilac/OpenGL.h>

#include <string>
#include <map>

namespace Lilac
{
class Shader
{
public:
	// TODO: Consider making these things virtual
	GLuint getRawHandle() const;

	// TODO: Get hot recompilation working
	void compileShader(const std::string& source, const std::map<std::string, std::string>& macros);
	static std::string preprocessShader(const std::string& source, int maxDepth, const std::map<std::string, std::string>& macros);

	// TODO: make constructor private (I think?)
	// TODO: make destructor

protected:
	GLuint m_handle;

	static const int s_maxDepth = 16;
};

class ComputeShader : public Shader
{
public:
	ComputeShader(const std::string& source);
	ComputeShader(const std::string& source, std::map<std::string, std::string> macros);
};

class VertexShader : public Shader
{
public:
	VertexShader(const std::string& source);
	VertexShader(const std::string& source, std::map<std::string, std::string> macros);
};

class FragmentShader : public Shader
{
public:
	FragmentShader(const std::string& source);
	FragmentShader(const std::string& source, std::map<std::string, std::string> macros);
};
}


#endif // LILAC_SHADER_H