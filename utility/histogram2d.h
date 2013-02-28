#ifndef COMMON_HISTOGRAM2D_H
#define COMMON_HISTOGRAM2D_H
#include <map>
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <string.h>

namespace sserialize {

class Histogram2D {
private:
	uint32_t m_smallestX;
	uint32_t m_smallestY;
	uint32_t m_largestX;
	uint32_t m_largestY;
	
	std::map<uint64_t, uint32_t> m_map;
public:
	Histogram2D() : 
	m_smallestX(0xFFFFFFFF),
	m_smallestY(0xFFFFFFFF),
	m_largestX(0),
	m_largestY(0)
	{
	}
	
	void inc(uint32_t x, uint32_t y);
	
	uint32_t count(uint32_t x, uint32_t y);

	void toFile(std::string fileName);
};

}//end namespace

#endif 