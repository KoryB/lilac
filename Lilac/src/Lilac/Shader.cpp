#include <Lilac/Shader.h>

#include <Lilac/OpenGL.h>

#include <strutil/strutil.h>

#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <map>

GLuint Lilac::Shader::getRawHandle() const
{
	return m_handle;
}

// TODO: Add logging, error?
void Lilac::Shader::compileShader(const std::string& source, const std::map<std::string, std::string>& macros)
{
	std::cout << "Processing Shader " << m_handle << ":" << std::endl << source << std::endl;

	auto processed = preprocessShader(source, s_maxDepth, macros);
	auto c_str = processed.c_str();

	std::cout << "Compiling Processed Shader " << m_handle << ":" << std::endl << processed << std::endl;

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
		std::cerr << "!!!!!!!!!! Shader compilation error: " << std::endl << errorString << std::endl;

		// Exit with failure.
		glDeleteShader(m_handle); // Don't leak the shader.

		// TODO: error
	}
}

// TODO: Eventually we'd like this to be its own class, but for now it's so simple that it's here
// TODO: Error handling
std::string Lilac::Shader::preprocessShader(const std::string& source, int maxDepth, const std::map<std::string, std::string>& macros)
{
	const std::string defineMacro = "#define";

	auto lines = strutil::split(source, "\n");
	std::map<std::string, std::string> sourceMacros(macros);

	std::string sourceWithoutDefines;

	for (auto const& line : lines)
	{
		if (strutil::starts_with(line, defineMacro))
		{
			auto nameValue = strutil::split(line, " ", 2);

			std::string name = nameValue[1];
			std::string value = nameValue[2];

			strutil::trim(name);
			strutil::trim(value);

			sourceMacros[name] = value;
		}
		else
		{
			sourceWithoutDefines += (line + "\n");
		}
	}

	std::cout << "Found macros: " << std::endl;

	for (auto const& [name, value] : sourceMacros)
	{
		std::cout << "\t" << name << "->" << value << std::endl;
	}

	for (int depth = 0; depth < maxDepth; depth++)
	{
		bool isReplaced = false;

		for (auto const& [name, value] : sourceMacros)
		{
			isReplaced |= strutil::replace_all(sourceWithoutDefines, name, value);
		}

		if (!isReplaced)
		{
			break;
		}
	}

	return sourceWithoutDefines;
}


Lilac::ComputeShader::ComputeShader(const std::string& source) : ComputeShader(source, {})
{

}

Lilac::ComputeShader::ComputeShader(const std::string& source, std::map<std::string, std::string> macros)
{
	this->m_handle = glCreateShader(GL_COMPUTE_SHADER);
	this->compileShader(source, macros);
}


Lilac::VertexShader::VertexShader(const std::string& source) : VertexShader(source, {})
{

}

Lilac::VertexShader::VertexShader(const std::string& source, std::map<std::string, std::string> macros)
{
	this->m_handle = glCreateShader(GL_VERTEX_SHADER);
	this->compileShader(source, macros);
}


Lilac::FragmentShader::FragmentShader(const std::string& source) : FragmentShader(source, {})
{

}

Lilac::FragmentShader::FragmentShader(const std::string& source, std::map<std::string, std::string> macros)
{
	this->m_handle = glCreateShader(GL_FRAGMENT_SHADER);
	this->compileShader(source, macros);
}