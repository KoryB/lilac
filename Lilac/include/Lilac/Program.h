#ifndef LILAC_PROGRAM_H
#define LILAC_PROGRAM_H

#include <Lilac/OpenGL.h>
#include <Lilac/Shader.h>

namespace Lilac
{
class Program
{
public:
	// TODO: Consider making these things virtual
	GLuint getRawHandle() const;
	void use() const;

	// TODO: make constructor private (I think?)
	// TODO: make destructor

protected:
	GLuint m_handle;

	void linkProgram();
};

class RenderProgram : public Program
{
public:
	RenderProgram(const VertexShader& vs, const FragmentShader& fs);
};

class ComputeProgram : public Program
{
public:
	ComputeProgram(const ComputeShader& cs);

	void dispatch(unsigned int x = 1, unsigned int y = 1, unsigned int z = 1);
};
}


#endif // LILAC_PROGRAM_H