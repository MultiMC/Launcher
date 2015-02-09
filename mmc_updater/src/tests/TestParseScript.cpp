#include "TestParseScript.h"

#include "TestUtils.h"
#include "UpdateScript.h"

void TestParseScript::testParse()
{
	UpdateScript script;

	script.parse("file_list.xml");

	TEST_COMPARE(script.isValid(), true);
	TEST_COMPARE(script.filesToInstall(),
				 std::vector<UpdateScriptFile>({UpdateScriptFile{"SourceA", "DestA", 0755},
												UpdateScriptFile{"SourceB", "DestB", 0644},
												UpdateScriptFile{"SourceC", "DestC", 0644}}));
	TEST_COMPARE(
		script.filesToUninstall(),
		std::vector<QString>({"file-to-uninstall.txt", "other-file-to-uninstall.txt"}));
}

int main(int, char **)
{
	TestList<TestParseScript> tests;
	tests.addTest(&TestParseScript::testParse);
	return TestUtils::runTest(tests);
}
