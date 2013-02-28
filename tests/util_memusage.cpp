#include <sserialize/utility/memusage.h>
#include <deque>
#include <set>
#include <vector>
#include <list>
#include <iostream>


int main() {
	sserialize::MemUsage().print();
	std::deque< std::deque<uint32_t>* > ab;
	for(size_t i = 0; i < 1024; i++) {
		std::deque<uint32_t> * p = new std::deque<uint32_t>();
		for(size_t j = 0; j < 16*1024; j++) {
			p->push_back(1024*i+j);
		}
		ab.push_back(p);
	}
	sserialize::MemUsage().print();
	
	for(size_t i = 0; i < 1024; i++) {
		delete ab[i];
	}
	ab.clear();
	
	sserialize::MemUsage().print();
	
	std::cout << std::endl << "Empty deque:" << sizeof(std::deque<uint32_t>) << std::endl;
	sserialize::MemUsage().print();
	std::deque<uint32_t> * emptyDeques = new std::deque<uint32_t>[1024*1024];
	sserialize::MemUsage().print();
	delete[] emptyDeques;
	
	std::cout << std::endl << "Empty set:" << sizeof(std::set<uint32_t>) << std::endl;
	sserialize::MemUsage().print();
	std::set<uint32_t> * emptySets = new std::set<uint32_t>[1024*1024];
	sserialize::MemUsage().print();
	delete[] emptySets;
	
	
	std::cout << std::endl << "Empty vector:" << sizeof(std::vector<uint32_t>) << std::endl;
	sserialize::MemUsage().print();
	std::vector<uint32_t> * emptyVectors = new std::vector<uint32_t>[1024*1024];
	sserialize::MemUsage().print();
	delete[] emptyVectors;
	
	
	std::cout << std::endl << "Empty list:" << sizeof(std::list<uint32_t>) << std::endl;
	sserialize::MemUsage().print();
	std::list<uint32_t> * emptyLists= new std::list<uint32_t>[1024*1024];
	sserialize::MemUsage().print();
	delete[] emptyLists;
	
	return 0;
}