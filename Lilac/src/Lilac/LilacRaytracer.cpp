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
	"#version 430\n"
	"in vec3 vp;\n"
	"out vec2 v_tex_coord;\n"
	"void main() {\n"
	"  gl_Position = vec4(2.0 * vp - vec3(1.0, 1.0, 0.0), 1.0);\n"
	"  v_tex_coord = vp.xy;\n"
	"}\n";

// TODO: Change to version 430?
const std::string fragmentShaderSource =
"#version 430\n"
"layout(std430, binding = 0) buffer Pixels\n"
"{\n"
"	vec4 colors[]; // Should match compute shader\n"
"};\n"
"const ivec2 image_size = ivec2(512, 512);\n"
"in vec2 v_tex_coord;\n"
"out vec4 frag_color;\n"
"\n"
"void main() {\n"
    //"  vec2 frag_coord = floor(gl_FragCoord.xy * image_size / vec2(512, 512));\n"
	"  vec2 frag_coord = floor((image_size - ivec2(1)) * v_tex_coord)\n; // Subtract 1 because we go from 0..1 inclusive\n"
	"  int index = int(frag_coord.x + frag_coord.y * image_size.x);\n"
	"  frag_color = vec4(0.0, 0.0, 0.0, 1.0);\n"
	//"  frag_color.r = index / float(image_size.x * image_size.y);\n"
	//"  frag_color.rg = frag_coord.xy / vec2(image_size);\n"
	"  frag_color = colors[index]; // 0.25 * (colors[index] + colors[index+1] + colors[index+2] + colors[index+3]);\n"
	"}\n";

using namespace Lilac;

bool initOpenGl()
{
	GLenum err = glewInit(); // REQUIRED to use GLEW functions!
	
	if (err != GLEW_OK)
	{
		std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
		return false;
	}

	std::cout << "" << std::endl;
	std::cout << "" << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "" << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "" << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "" << "OpenGL Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "" << std::endl;
	std::cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl << std::endl;

	int work_group_count[3] = { 0, 0, 0 };

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_group_count[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_group_count[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_group_count[2]);

	std::cout << "Max work group count <" <<
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

	return true;
}

// camera logic might be broken here actually

int main()
{
	sf3d::Window window(sf3d::VideoMode(512, 512), "OpenGL");
	window.setActive(true);

	if (!initOpenGl())
	{
		return EXIT_FAILURE;
	}

	// This assumes compute shaders are available (gl 4.3+)
	int tex_w = 512, tex_h = 512;
	GLuint outputTexture = 0;

	glGenTextures(1, &outputTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, outputTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	auto raytraceShaderSource = loadFileString("resources/shaders/raytrace.cs.glsl");
	ComputeShader raytraceShader{ raytraceShaderSource };
	ComputeProgram raytraceProgram{ raytraceShader };

	float quad_points[] = {
	   1.0f,  0.0f,  0.0f,
	   0.0f,  0.0f,  0.0f,
	   1.0f,  1.0f,  0.0f,
	   0.0f,  1.0f,  0.0f
	};

	GLuint buffer = 0;
	const int raysPerPixel = 1;
	const int channelsPerPixel = 4;
	const int imageSize = tex_w * tex_h;
	const int bytesPerFloat = 4;
	const int bufferSize = bytesPerFloat * raysPerPixel * channelsPerPixel * imageSize;

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, NULL, GL_DYNAMIC_COPY); // TODO: Look more into `usage` parameter, it's a bit unclear which I should pick here

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

	auto sleepTime = sf3d::milliseconds(1000);

	std::vector<float> bufferPrePopulated;
	int i = 0;
	bool isColor = false;
	float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for (int y = 0; y < tex_h; y++)
	{
		for (int x = 0; x < tex_w; x++)
		{
			if (i++ % 8 == 0)
			{
				isColor = !isColor;
			}

			if (isColor)
			{
				color[0] = ((float)x) / tex_w;
				color[1] = ((float)y) / tex_h;
			}
			else
			{
				color[0] = color[1] = 0.0f;
			}

			bufferPrePopulated.push_back(color[0]);
			bufferPrePopulated.push_back(color[1]);
			bufferPrePopulated.push_back(color[2]);
			bufferPrePopulated.push_back(color[3]);
		}
	}

	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	// glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, bufferPrePopulated.data(), GL_DYNAMIC_COPY);

	auto running = true;
	while (running)
	{
		raytraceProgram.use();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
		raytraceProgram.dispatch(tex_w, tex_h);

		// make sure writing to image has finished before read
		// TODO: Add this into the compute program with a configurable bitset
		// But research to see if that is bad performance wise, technical details, matters which image we are reading from etc
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

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
		glBindTexture(GL_TEXTURE0, outputTexture);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
		// draw points 0-3 from the currently bound VAO with current in-use shader
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		window.display();
		sf3d::sleep(sleepTime);

		/*std::vector<float> bufferVector(bufferSize);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufferVector.data());

		std::ofstream bufferFile;
		bufferFile.open("buffer.csv");

		for (int y = 0; y < tex_h; y++)
		{
			for (int x = 0; x < tex_w; x++)
			{
				bufferFile << "<";
				for (int i = 0; i < 4; i++)
				{
					auto f = bufferVector[y * tex_w * 4 + x * 4 + i];

					bufferFile << f;

					if (i != 3)
					{
						bufferFile << " ";
					}
				}
				bufferFile << ">, ";
			}
			bufferFile << std::endl;
		}
		bufferFile.close();

		return 0;*/
	}
}