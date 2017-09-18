#include <sserialize/stats/histogram2d.h>
#include <cmath>


namespace sserialize {

void Histogram2D::inc(uint32_t x, uint32_t y) {
	if (m_smallestX > x) m_smallestX = x;
	if (m_smallestY > y) m_smallestY = y;
	if (m_largestX < x) m_largestX = x;
	if (m_largestY < y) m_largestY = y; 
	uint64_t bin = ( (static_cast<uint64_t>(x) << 32) | y);
	if (m_map.count(bin) > 0) {
		m_map[bin] += 1;
	}
	else {
		m_map.insert( std::pair<uint64_t, uint32_t>(bin, 1) );
	}
}

uint32_t Histogram2D::count(uint32_t x, uint32_t y) {
	uint64_t bin = (static_cast<uint64_t>(x) << 32 | static_cast<uint64_t>(y));
	if (m_map.count(bin) > 0) {
		return m_map[bin];
	}
	return 0;
}

void Histogram2D::toFile(std::string fileName) {
	uint32_t width = m_largestX+1;
	uint32_t height = m_largestY+1;
	uint32_t largest = 0;
	uint32_t * pic = new uint32_t[width*height];
	delete[] pic;
	pic = new uint32_t[width*height];
	memset(pic, 0, sizeof(uint32_t)*width*height);
	uint32_t * colMax = new uint32_t[width];
	memset(colMax, 0, sizeof(uint32_t)*width);
	uint32_t sum = 0;
	for(std::map<uint64_t, uint32_t>::iterator it = m_map.begin(); it != m_map.end(); it++) {
		uint64_t val = it->first;
		uint32_t x = val >> 32;
		uint32_t y = val & (0x00000000FFFFFFFF);
		if (largest <  it->second) {
			largest =  it->second;
		}
		if (colMax[x] < it->second) colMax[x] = it->second;
		if (y*width+x > width*height) {
			std::cout << "error" << std::endl;
		}
		pic[y*width+x] = it->second;
		sum += it->second;
	}
	std::ofstream rawfile;
	std::ofstream imgfile;
	std::ofstream imgGlobalScaleFile;
	std::ofstream imgGlobalScaleGammaFile;
	std::ofstream imgGlobalScaleColorCodedFile;
	std::ofstream imgGlobalScaleColorCodedGammaFile;
	rawfile.open(std::string(fileName + ".csv").c_str());
	imgfile.open(std::string(fileName + ".pnm").c_str());
	imgGlobalScaleFile.open(std::string(fileName + "_global.pnm").c_str());
	imgGlobalScaleGammaFile.open(std::string(fileName + "_globalgamma.pnm").c_str());
	imgGlobalScaleColorCodedFile.open(std::string(fileName + "_globalcolorcoded.pnm").c_str());
	imgGlobalScaleColorCodedGammaFile.open(std::string(fileName + "_globalcolorcodedgamma.pnm").c_str());
	
// 	rawfile << "P2" << std::endl << width << " " << height << std::endl << largest << std::endl;
	imgfile << "P2" << std::endl << width << " " << height << std::endl << 255 << std::endl;
	imgGlobalScaleFile << "P2" << std::endl << width << " " << height << std::endl << 255 << std::endl;
	imgGlobalScaleGammaFile << "P2" << std::endl << width << " " << height << std::endl << 255 << std::endl;
	imgGlobalScaleColorCodedFile << "P3" << std::endl << width << " " << height << std::endl << 255 << std::endl;
	imgGlobalScaleColorCodedGammaFile << "P3" << std::endl << width << " " << height << std::endl << 255 << std::endl;
	sum = 0;
	for(uint32_t y=0; y < height; y++) {
		for(uint32_t x=0; x < width; x++) {
			uint32_t val = pic[y*width+x];
			rawfile << val << " ";
			sum += val;
			if (colMax[x] > 0) {
				imgfile << (val*255)/colMax[x] << " ";
			}
			else {
				imgfile << (val) << " ";
			}
			imgGlobalScaleFile << (val*255)/largest << " ";
			imgGlobalScaleGammaFile << static_cast<int>(pow(((double)val)/largest, 1.0/3.2)*255) << " ";
			{
				uint16_t compRange = 255 + 100 + 100;
				uint16_t midCol = (val*compRange) / largest;
				int r, g, b;
				r = 0;
				g = 0;
				b = 0;
				if (midCol < 256) {
					b = midCol;
				}
				else if (midCol < 356) {
					r = midCol-256+100;
				}
				else {
					g = midCol - 356 + 100;
				}
				imgGlobalScaleColorCodedFile << r << " " << g << " " << b << " ";
			}
			{
				uint16_t compRange = 255 + 100 + 100;
				int midCol = static_cast<int>(pow(((double)val)/largest, 1.0/3.2)*compRange);
				int r, g, b;
				r = 0;
				g = 0;
				b = 0;
				if (midCol < 256) {
					g = midCol;
				}
				else if (midCol < 356) {
					r = midCol-256+100;
				}
				else {
					b = midCol - 356 + 100;
				}
				imgGlobalScaleColorCodedGammaFile << r << " " << g << " " << b << " ";
			}
		}
		rawfile << std::endl;
		imgfile << std::endl;
		imgGlobalScaleFile << std::endl;
		imgGlobalScaleGammaFile << std::endl;
		imgGlobalScaleColorCodedFile << std::endl;
		imgGlobalScaleColorCodedGammaFile << std::endl;
	}
	rawfile.close();
	imgfile.close();
	imgGlobalScaleFile.close();
	imgGlobalScaleGammaFile.close();
	imgGlobalScaleColorCodedFile.close();
	delete[] pic;
}

}//end namespace