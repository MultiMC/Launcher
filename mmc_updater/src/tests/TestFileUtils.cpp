#include "TestFileUtils.h"

#include "FileUtils.h"
#include "TestUtils.h"

void TestFileUtils::testDirName()
{
	QString dirName;
	QString fileName;

#ifdef Q_OS_WIN
	// absolute paths
	dirName = FileUtils::dirname("E:/Some Dir/App.exe");
	TEST_COMPARE(dirName, "E:/Some Dir");
	fileName = FileUtils::fileName("E:/Some Dir/App.exe");
	TEST_COMPARE(fileName, "App.exe");

	dirName =
		FileUtils::dirname("C:/Users/kitteh/AppData/Local/Temp/MultiMC5-yidaaa/MultiMC.exe");
	TEST_COMPARE(dirName, "C:/Users/kitteh/AppData/Local/Temp/MultiMC5-yidaaa");
	fileName =
		FileUtils::fileName("C:/Users/kitteh/AppData/Local/Temp/MultiMC5-yidaaa/MultiMC.exe");
	TEST_COMPARE(fileName, "MultiMC.exe");

#else
	// absolute paths
	dirName = FileUtils::dirname("/home/tester/foo bar/baz");
	TEST_COMPARE(dirName, "/home/tester/foo bar");
	fileName = FileUtils::fileName("/home/tester/foo bar/baz");
	TEST_COMPARE(fileName, "baz");
#endif
	// current directory
	dirName = FileUtils::dirname("App.exe");
	TEST_COMPARE(dirName, ".");
	fileName = FileUtils::fileName("App.exe");
	TEST_COMPARE(fileName, "App.exe");

	// relative paths
	dirName = FileUtils::dirname("Foo/App.exe");
	TEST_COMPARE(dirName, "Foo");
	fileName = FileUtils::fileName("Foo/App.exe");
	TEST_COMPARE(fileName, "App.exe");
}

int main(int, char **)
{
	TestList<TestFileUtils> tests;
	tests.addTest(&TestFileUtils::testDirName);
	return TestUtils::runTest(tests);
}
