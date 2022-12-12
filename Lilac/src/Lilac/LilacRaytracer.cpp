#include <SFML3D/Window.hpp>
#include <SFML3D/Graphics.hpp>

#include <Lilac/OpenGL.h>

#include <Lilac/Shader.h>
#include <Lilac/Program.h>
#include <Lilac/File.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>


const std::string vertexShaderSource =
	"#version 400\n"
	"in vec3 vp;"
	"out vec2 v_tex_coord;"
	"void main() {"
	"  gl_Position = vec4(2.0 * vp - vec3(1.0, 1.0, 0.0), 1.0);"
	"  v_tex_coord = vp.xy;"
	"}";

const std::string fragmentShaderSource =
	"#version 400\n"
	"in vec2 v_tex_coord;"
	"out vec4 frag_colour;"
	"uniform sampler2D u_texture;"
	"void main() {"
	"  frag_colour = texture(u_texture, v_tex_coord);"
	"}";

using namespace Lilac;

void init_opengl()
{

}

// for some reason we're getting just a speck in the output
// regardless of the scaling of the image
// bug in `intersect_aabb`?
int main()
{
	sf3d::Window window(sf3d::VideoMode(512, 512), "OpenGL");
	window.setActive(true);

	GLenum err = glewInit(); // REQUIRED to use GLEW functions!

	if (err != GLEW_OK)
	{
		std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "" << std::endl;
	std::cout << "" << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "" << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "" << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "" << "OpenGL Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "" << std::endl;
	std::cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl << std::endl;


	// This assumes compute shaders are available (gl 4.3+)
	int tex_w = 512, tex_h = 512;
	GLuint tex_output = 0;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	int work_group_count[3] = { 0, 0, 0 };

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_group_count[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_group_count[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_group_count[2]);

	std::cout << "Max work group size <" <<
		work_group_count[0] << ", " <<
		work_group_count[1] << ", " <<
		work_group_count[2] << ">" << std::endl;

	int work_group_size[3] = { 0, 0, 0 };

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_group_size[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_group_size[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_group_size[2]);

	std::cout << "Max work group size <" <<
		work_group_size[0] << ", " <<
		work_group_size[1] << ", " <<
		work_group_size[2] << ">" << std::endl;

	int work_group_invocations;
	
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_group_invocations);

	std::cout << "Max work group invocations: " << work_group_invocations << std::endl;

	auto raytraceShaderSource = loadFileString("resources/shaders/raytrace.cs.glsl");
	ComputeShader raytraceShader{ raytraceShaderSource };
	ComputeProgram raytraceProgram{ raytraceShader };

	float quad_points[] = {
	   1.0f,  0.0f,  0.0f,
	   0.0f,  0.0f,  0.0f,
	   1.0f,  1.0f,  0.0f,
	   0.0f,  1.0f,  0.0f
	};

	GLuint quad_vbo = 0;
	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), quad_points, GL_STATIC_DRAW);

	GLuint quad_vao = 0;
	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	VertexShader vertexShader{ vertexShaderSource };
	FragmentShader fragmentShader{ fragmentShaderSource };
	RenderProgram quadProgram{ vertexShader, fragmentShader };


	auto running = true;
	while (running)
	{
		raytraceProgram.dispatch();

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		sf3d::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf3d::Event::Closed)
			{
				running = false;
			}
			else if (event.type == sf3d::Event::Resized)
			{
				glViewport(0, 0, event.size.width, event.size.height);
			}
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		quadProgram.use();
		glBindVertexArray(quad_vao);
		glBindTexture(GL_TEXTURE0, tex_output);
		// draw points 0-3 from the currently bound VAO with current in-use shader
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		window.display();
	}
}