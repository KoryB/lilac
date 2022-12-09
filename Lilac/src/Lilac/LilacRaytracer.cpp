#include <GL/glew.h>

#include <SFML3D/Window.hpp>
#include <SFML3D/OpenGL.hpp>

#include <iostream>

int main()
{
	sf3d::Window window(sf3d::VideoMode(800, 600), "OpenGL");
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

	std::cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;

	glGenTextures(1, &tex_output);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);


	auto running = true;
	while (running)
	{
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

		window.display();
	}

	return EXIT_SUCCESS;
}