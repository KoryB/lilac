#include <GL/glew.h>

#include <SFML3D/Window.hpp>
#include <SFML3D/Graphics.hpp>
#include <SFML3D/OpenGL.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

void init_opengl()
{

}

int log_shader_compile_error(GLuint shader)
{
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
		std::string errorString(errorLog.begin(), errorLog.end());

		std::cerr << "Shader compilation error: " << std::endl << errorString << std::endl;

		// Exit with failure.
		glDeleteShader(shader); // Don't leak the shader.
		return 1;
	}

	return 0;
}

// for some reason we're getting just a speck in the output
// regardless of the scaling of the image
// bug in `intersect_aabb`?
int main()
{
	sf3d::Window window(sf3d::VideoMode(512, 512), "OpenGL");
	window.setActive(true);

	// This assumes compute shaders are available (gl 4.3+)
	int tex_w = 512, tex_h = 512;
	GLuint tex_output;

	GLenum err = glewInit(); // REQUIRED to use GLEW functions!

	if (err != GLEW_OK)
	{
		std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
		return EXIT_FAILURE;
	}

	std::ifstream fp{ "resources/shaders/raytrace.cs.glsl" };
	std::stringstream buffer;
	buffer << fp.rdbuf();

	fp.close();

	auto raytracer_shader_source = buffer.str();

	std::cout << "" << std::endl;
	std::cout << "" << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "" << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "" << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "" << "OpenGL Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "" << std::endl;
	std::cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl << std::endl;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	int work_group_count[3];

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_group_count[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_group_count[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_group_count[2]);

	std::cout << "Max work group size <" <<
		work_group_count[0] << ", " <<
		work_group_count[1] << ", " <<
		work_group_count[2] << ">" << std::endl;

	int work_group_size[3];

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

	GLuint raytracer_shader = glCreateShader(GL_COMPUTE_SHADER);
	auto raytracer_shader_cstr = raytracer_shader_source.c_str();

	std::cout << "Compiling shader: " << raytracer_shader_source << std::endl;
	glShaderSource(raytracer_shader, 1, &raytracer_shader_cstr, NULL);
	glCompileShader(raytracer_shader);
	if (log_shader_compile_error(raytracer_shader))
	{
		return EXIT_FAILURE;
	}

	GLuint raytracer_program = glCreateProgram();
	glAttachShader(raytracer_program, raytracer_shader);
	glLinkProgram(raytracer_program);
	// check for linking errors and validate program as per normal here
	GLint isLinked = 0;
	glGetProgramiv(raytracer_program, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(raytracer_program, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		glGetProgramInfoLog(raytracer_program, maxLength, &maxLength, &infoLog[0]);
		std::string infoString(infoLog.begin(), infoLog.end());

		std::cerr << "Shader compilation error: " << std::endl << infoString << std::endl;

		// The program is useless now. So delete it.
		glDeleteProgram(raytracer_program);

		// Provide the infolog in whatever manner you deem best.
		// Exit with failure.
		return EXIT_FAILURE;
	}

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

	const char* vertex_shader =
		"#version 400\n"
		"in vec3 vp;"
		"out vec2 v_tex_coord;"
		"void main() {"
		"  gl_Position = vec4(2.0 * vp - vec3(1.0, 1.0, 0.0), 1.0);"
		"  v_tex_coord = vp.xy;"
		"}";

	const char* fragment_shader =
		"#version 400\n"
		"in vec2 v_tex_coord;"
		"out vec4 frag_colour;"
		"uniform sampler2D u_texture;"
		"void main() {"
		"  frag_colour = texture(u_texture, v_tex_coord);"
		"}";

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, NULL);
	glCompileShader(vs);

	if (log_shader_compile_error(vs))
	{
		return EXIT_FAILURE;
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragment_shader, NULL);
	glCompileShader(fs);

	if (log_shader_compile_error(fs))
	{
		return EXIT_FAILURE;
	}

	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);


	auto running = true;
	while (running)
	{
		{ // launch compute shaders!
			glUseProgram(raytracer_program);
			glDispatchCompute((GLuint)tex_w, (GLuint)tex_h, 1);
		}

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

		glUseProgram(shader_programme);
		glBindVertexArray(quad_vao);
		glBindTexture(GL_TEXTURE0, tex_output);
		// draw points 0-3 from the currently bound VAO with current in-use shader
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		window.display();
	}
}