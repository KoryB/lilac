#ifndef LILAC_FILE_H
#define LILAC_FILE_H

#include <string>

#include <iostream>
#include <fstream>
#include <sstream>


// TODO: Handle errors
std::string loadFileString(const std::string& filepath)
{
	std::ifstream fp{ filepath, std::ios_base::in };
	std::stringstream buffer;
	buffer << fp.rdbuf();

	fp.close();

	return buffer.str();
}

#endif //LILAC_FILE_H