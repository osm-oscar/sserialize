#ifndef SSERIALIZE_MEMUSAGE_H
#define SSERIALIZE_MEMUSAGE_H
#include <stdio.h>
#include <iostream>

//from http://stackoverflow.com/questions/1558402/memory-usage-of-current-process-in-c

namespace sserialize {

class MemUsage {
public:
	MemUsage() {
		const char* statm_path = "/proc/self/statm";

		FILE * f = fopen(statm_path,"r");
		if(!f){
			perror(statm_path);
			return;
		}
		if(7 != fscanf(f,"%ld %ld %ld %ld %ld %ld %ld", &size,&resident,&share,&text,&lib,&data,&dt)) {
			perror(statm_path);
		}
		fclose(f);
	}
	unsigned long size,resident,share,text,lib,data,dt;
	inline void print() const {
		std::cout << "Memory usage: " << std::endl;
		std::cout << "Size: " << size << std::endl;
		std::cout << "Resident: " << resident << std::endl;
		std::cout << "Share: " << share << std::endl;
		std::cout << "Text: " << text << std::endl;
		std::cout << "Lib: " << lib << std::endl;
		std::cout << "Data: " << data << std::endl;
		std::cout << "End Memory usage" << std::endl;
	}
};

}//end namespace

#endif