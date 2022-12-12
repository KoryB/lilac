#include <Lilac/Shader.h>

#include <Lilac/OpenGL.h>

#include <string>
#include <memory>
#include <vector>
#include <iostream>


GLuint Lilac::Shader::getRawHandle() const
{
	return m_handle;
}


// TODO: Add logging, error?
void Lilac::Shader::compileShader(const std::string& source)
{
	std::cout << "Compiling Shader " << m_handle << ":" << std::endl << source << std::endl;

	auto c_str = source.c_str();

	glShaderSource(m_handle, 1, &c_str, NULL);
	glCompileShader(m_handle);

	GLint isCompiled = 0;
	glGetShaderiv(m_handle, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(m_handle, maxLength, &maxLength, &errorLog[0]);

		std::string errorString(errorLog.begin(), errorLog.end());
		std::cerr << "Shader compilation error: " << std::endl << errorString << std::endl;

		// Exit with failure.
		glDeleteShader(m_handle); // Don't leak the shader.

		// TODO: error
	}
}


Lilac::ComputeShader::ComputeShader(const std::string& source)
{
	this->m_handle = glCreateShader(GL_COMPUTE_SHADER);
	this->compileShader(source);
}

Lilac::VertexShader::VertexShader(const std::string& source)
{
	this->m_handle = glCreateShader(GL_VERTEX_SHADER);
	this->compileShader(source);
}

Lilac::FragmentShader::FragmentShader(const std::string& source)
{
	this->m_handle = glCreateShader(GL_FRAGMENT_SHADER);
	this->compileShader(source);
}