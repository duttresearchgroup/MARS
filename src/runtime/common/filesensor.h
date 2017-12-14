#ifndef __arm_rt_filesensor_h
#define __arm_rt_filesensor_h

#include <fstream>

class FileSensor
{
	std::ifstream _file;

public:
	FileSensor(const std::string &path)
		:_file(path)
		 { }

	double read()
	{
		double val;
		_file.seekg(0);
		_file >> val;
		return val;
	}

};


#endif

