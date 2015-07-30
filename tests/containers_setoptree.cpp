#include <iostream>
#include <sserialize/search/SetOpTree.h>

using namespace sserialize;

class TestOpFilter: public SetOpTree::ExternalFunctoid {
	virtual ItemIndex operator()(const std::string& ) { return ItemIndex(); }
	virtual const std::string cmdString() const { return "TestOpFilter"; }
};

class TestOpFilter2: public SetOpTree::ExternalFunctoid {
	virtual ItemIndex operator()(const std::string& ) { return ItemIndex(); }
	virtual const std::string cmdString() const { return "TestOpFilter2"; }
};

int main() {
	SetOpTree opTree(SetOpTree::SOT_COMPLEX);
	opTree.registerSelectableOpFilter(new TestOpFilter());
	opTree.registerExternalFunction(new TestOpFilter2());
	std::vector<std::string> qstrs;
	qstrs.push_back("Triberg / nnenwaldtunnel\\");
	qstrs.push_back("Triberg / nnenwaldtunnel\\ ");
	qstrs.push_back("Triberg / nnenwaldtunnel\\ (16");
	qstrs.push_back("Triberg / nnenwaldtunnel\\ (166");
	qstrs.push_back("Triberg / nnenwaldtunnel\\ (166\\");
	qstrs.push_back("Triberg / nnenwaldtunnel\\ (166\\ ");
	qstrs.push_back("abc\\");
	qstrs.push_back("abc\\-cdf");
	qstrs.push_back("($TestOpFilter[abc]");
	qstrs.push_back("(abc / $TestOpFilter[hallo]) / $TestOpFilter2[du]");
	qstrs.push_back("(abc / ()) / efg");
	qstrs.push_back("(abc / **) / efg");
	qstrs.push_back("(abc / ) / efg");
	qstrs.push_back("abc / cde /");
	qstrs.push_back("abc -");
	qstrs.push_back("abc cde");
	qstrs.push_back(")))((abc +-+ sdf");
	qstrs.push_back(")))((abc");
	qstrs.push_back("(((abc");
	qstrs.push_back("(((abc)))");
	qstrs.push_back("(abc + cdf) - (fgh + hij)");
	qstrs.push_back("abc");
	qstrs.push_back("abc / cdf");
	qstrs.push_back("abc / cde / efg");
	qstrs.push_back("abc / cde / efg / ghi");
	qstrs.push_back("abc - cdf");
	qstrs.push_back("abc + cdf");
	qstrs.push_back("abc ^ cdf");
	for(std::vector<std::string>::iterator it = qstrs.begin(); it != qstrs.end(); ++it) {
		opTree.buildTree(*it);
		std::cout << "Parsed querystring: " << *it << "; to: ";
		opTree.printStructure(std::cout) << std::endl;
	}
	return 0;
}