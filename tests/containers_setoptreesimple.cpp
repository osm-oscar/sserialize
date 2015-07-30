#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <vector>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/containers/SetOpTree.h>
#include <sserialize/containers/SetOpTreePrivateSimple.h>

using namespace sserialize;

class TestOpFilter: public SetOpTree::ExternalFunctoid {
	virtual ItemIndex operator()(const std::string& ) { return ItemIndex(); }
	virtual const std::string cmdString() const { return "TestOpFilter"; }
};

class TestOpFilter2: public SetOpTree::ExternalFunctoid {
	virtual ItemIndex operator()(const std::string& ) { return ItemIndex(); }
	virtual const std::string cmdString() const { return "TestOpFilter2"; }
};

class SetOpTreeSimpleTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( SetOpTreeSimpleTest );
// CPPUNIT_TEST( testWithoutEf );
CPPUNIT_TEST( testWithEF );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	
	void testWithEF() {
		std::vector< std::pair<std::string, std::string> > qsStrucStr;
		qsStrucStr.push_back(std::pair<std::string, std::string>("def", "*def*"));
		qsStrucStr.push_back(std::pair<std::string, std::string>("*sfx", "*sfx"));
		qsStrucStr.push_back(std::pair<std::string, std::string>("pfx*", "pfx*"));
		qsStrucStr.push_back(std::pair<std::string, std::string>("*substr*", "*substr*"));
		qsStrucStr.push_back(std::pair<std::string, std::string>("\"exact\"", "\"exact\""));
		qsStrucStr.push_back(std::pair<std::string, std::string>("abc\\", ""));
		qsStrucStr.push_back(std::pair<std::string, std::string>("abc\\ de", "*abc de*"));
		qsStrucStr.push_back(std::pair<std::string, std::string>("-abc -cde", "-*abc* -*cde*"));
		qsStrucStr.push_back(std::pair<std::string, std::string>("-abc \\-cde", "*-cde* -*abc*"));
		qsStrucStr.push_back(std::pair<std::string, std::string>("$TestOpFilter[abc]", "$TestOpFilter[abc]"));
		qsStrucStr.push_back(std::pair<std::string, std::string>("-$TestOpFilter[abc]", "-$TestOpFilter[abc]"));
		qsStrucStr.push_back(std::pair<std::string, std::string>("abc - $TestOpFilter[hallo] $TestOpFilter2[du]", "*abc* $TestOpFilter2[du] -$TestOpFilter[hallo]"));
		qsStrucStr.push_back(std::pair<std::string, std::string>("abc -\\$TestOpFilter[hallo]* $TestOpFilter2[du]", "*abc* $TestOpFilter2[du] -$TestOpFilter[hallo]*"));
		
		sserialize::SetOpTree sop(sserialize::SetOpTree::SOT_SIMPLE);
		sop.registerSelectableOpFilter(new TestOpFilter());
		sop.registerExternalFunction(new TestOpFilter2());
		
		for(std::vector< std::pair<std::string, std::string> >::iterator it(qsStrucStr.begin()); it != qsStrucStr.end(); ++it) {
			sop.buildTree(it->first);
			std::stringstream ss;
			sop.printStructure(ss);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(it->first, it->second, ss.str());
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  SetOpTreeSimpleTest::suite() );
	runner.run();
	return 0;
}