#include <Lilac/File.h>

#include <iostream>
#include <fstream>
#include <sstream>


std::string Lilac::loadFileString(const std::string& filepath)
{
	std::ifstream fp{ filepath, std::ios_base::in };
	std::stringstream buffer;
	buffer << fp.rdbuf();

	fp.close();

	return buffer.str();
}