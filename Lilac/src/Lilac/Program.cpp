#include <Lilac/Program.h>
#include <Lilac/Shader.h>

#include <iostream>
#include <vector>


GLuint Program::getRawHandle() const 
{
	return m_handle;
}

void Program::use() const
{
	glUseProgram(m_handle);
}

void Program::linkProgram()
{
	glLinkProgram(m_handle);
	// check for linking errors and validate program as per normal here
	GLint isLinked = 0;
	glGetProgramiv(m_handle, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		glGetProgramInfoLog(m_handle, maxLength, &maxLength, &infoLog[0]);
		std::string infoString(infoLog.begin(), infoLog.end());

		std::cerr << "Shader compilation error: " << std::endl << infoString << std::endl;

		// The program is useless now. So delete it.
		glDeleteProgram(m_handle);

		// Provide the infolog in whatever manner you deem best.
		// TODO: Error
	}
}

RenderProgram::RenderProgram(const VertexShader &vs, const FragmentShader &fs)
{
	m_handle = glCreateProgram();

	auto vsHandle = vs.getRawHandle();
	auto fsHandle = fs.getRawHandle();

	glAttachShader(m_handle, vsHandle);
	glAttachShader(m_handle, fsHandle);
	linkProgram();
}

ComputeProgram::ComputeProgram(const ComputeShader& cs)
{
	m_handle = glCreateProgram();

	auto csHandle = cs.getRawHandle();

	glAttachShader(m_handle, csHandle);
	linkProgram();
}

void ComputeProgram::dispatch(unsigned int x, unsigned int y, unsigned int z)
{
	use();
	glDispatchCompute((GLuint)x, (GLuint)y, (GLuint)z);
}