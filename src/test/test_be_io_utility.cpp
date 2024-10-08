/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */

#include <cstdlib>
#include <iostream>
#include <sstream>

#include <be_error.h>
#include <be_io_utility.h>
#include <be_memory_autoarray.h>
#include <be_memory_autoarrayutility.h>
#include <be_sysdeps.h>

using namespace BiometricEvaluation;
using namespace std;

static int
testPipe()
{
#ifdef _WIN32
	throw BiometricEvaluation::Error::NotImplemented{};
#else /* _WIN32 */
	cout << "Testing pipe functions.\n";
	std::string msg("Test Message; pay attention!");
	int msgSize = msg.length();

	int pipeFD[2];
	if (pipe(pipeFD) != 0) {
		cerr << "Could not create pipe: " << Error::errorStr() << ".\n";
		return (EXIT_FAILURE);
	}
	Memory::uint8Array cMsgArr(msgSize + 1);
	Memory::uint8Array pMsgArr(msgSize + 1);
	string rMsg;
	pid_t pid = fork();
	switch(pid) {
	case 0:		/* child */
		close(pipeFD[1]);
		try {
			cout << "Child read: ";
			IO::Utility::readPipe(cMsgArr, msgSize, pipeFD[0]);
		} catch (const Error::Exception &e) {
			cout << "Failed : " << e.whatString() << ".\n";
			return (EXIT_FAILURE);
		}
		rMsg = to_string(cMsgArr);
		cout << "Got '" << rMsg << "': ";
		if (rMsg != msg) {
			cout << "Messages do not match; failure.\n";
			return (EXIT_FAILURE);
		}
		cout << "Success.\n";
		try {
			cout << "Child read of non-existant message: ";
			IO::Utility::readPipe(cMsgArr, 2, pipeFD[0]);
			cout << "Failed; something was read.\n";
			return (EXIT_FAILURE);
		} catch (const Error::Exception &e) {
			cout << "'" << e.whatString() << "': Success.\n";
		}
		return (EXIT_SUCCESS);
	case -1:	/* error */
		cerr << "Could not fork: " << Error::errorStr() << ".\n";
		return (EXIT_FAILURE);
	default:	/* parent */
		close(pipeFD[0]);
		Memory::AutoArrayUtility::setString(pMsgArr, msg);
		try {
			IO::Utility::writePipe(pMsgArr, msgSize, pipeFD[1]);
		} catch (const Error::Exception &e) {
			cerr << "Parent failed to write message: "
			    << e.whatString() << ".\n";
			return (EXIT_FAILURE);
		}
		close(pipeFD[1]);
		int status;
		wait(&status);
		return (EXIT_SUCCESS);
	}
#endif /* _WIN32 */
}

int
main(int argc, char* argv[])
{
	/* readFile */
	cout << "Read file (" << __FILE__ << "): ";
	Memory::uint8Array textFile;
	try {
		textFile = IO::Utility::readFile(__FILE__);
		cout << "success" << endl;
		
//		/* Print a line of the text file, just for kicks */
//		string line((char *)&(*textFile), 5, 68);
//		cout << "\t\"" << line << '"' << endl;
		
	} catch (const Error::Exception &e) {
		cout << "ERROR (" << e.what() << ")" << endl;
		return (EXIT_FAILURE);
	}

	/* writeFile */
	string tempFileName = "temp_file";
	cout << "Write file: ";
	try {
		IO::Utility::writeFile(textFile, tempFileName);
		cout << "success" << endl;
	} catch (const Error::Exception &e) {
		cout << "ERROR (" << e.what() << ")" << endl;
		return (EXIT_FAILURE);
	}
	
	/* Diff the original file and the written file to check consistency */
	cout << "Diff original and written files: ";
	Memory::uint8Array textFile2;
	try {
		textFile2 = IO::Utility::readFile(tempFileName);

	} catch (const Error::Exception &e) {
		cout << "ERROR (" << e.what() << ")" << endl;
		return (EXIT_FAILURE);
	}
	if (textFile.size() != textFile2.size()) {
		cout << "ERROR (sizes differ)" << endl;
		return (EXIT_FAILURE);
	}
	for (size_t i = 0; i < textFile.size(); i++) {
		if (textFile.at(i) != textFile2.at(i)) {
			cout << "ERROR (data differs)" << endl;
			return (EXIT_FAILURE);
		}
	}
	cout << "success" << endl;
	
	/* Test the set aside of a file */
	cout << "Set aside file " << tempFileName << ": ";
	ostringstream sstr;
	for (int i = 1; i < 6; i++) {
		try {
			IO::Utility::setAsideName(tempFileName);
			sstr.str("");
			sstr << tempFileName << "." << i;
			if (!IO::Utility::fileExists(sstr.str())) {
				cout << "Failed to set aside to " <<
				    sstr.str() << endl;
				return (EXIT_FAILURE);
			}
			IO::Utility::writeFile(textFile, tempFileName);
		} catch (const Error::Exception &e) {
			cout << "Caught " << e.what() << endl;
			return (EXIT_FAILURE);
		}
	}
	cout << "Success." << endl;

	/* Test the set aside of a directory */
	string tempDirName = "temp_dir";
	cout << "Set aside directory " << tempDirName << ": ";
	if (mkdir(tempDirName.c_str(), 0777) != 0) {
		cout << "Failed to make temp directory" << endl;
		return (EXIT_FAILURE);
	}
	for (int i = 1; i < 6; i++) {
		try {
			IO::Utility::setAsideName(tempDirName);
			sstr.str("");
			sstr << tempDirName << "." << i;
			if (!IO::Utility::fileExists(sstr.str())) {
				cout << "Failed to set aside to " <<
				    sstr.str() << endl;
				return (EXIT_FAILURE);
			}
			mkdir(tempDirName.c_str(), 0777);
		} catch (const Error::Exception &e) {
			cout << "Caught " << e.what() << endl;
			return (EXIT_FAILURE);
		}
	}
	cout << "Success." << endl;

	/* Clean up */
	if (unlink(tempFileName.c_str()))
		cout << "Could not remove " << tempFileName << endl;
	if (rmdir(tempDirName.c_str()))
		cout << "Could not remove " << tempDirName << endl;
	for (int i = 1; i < 6; i++) {
		sstr.str("");
		sstr << tempFileName << "." << i;
		unlink(sstr.str().c_str());
		sstr.str("");
		sstr << tempDirName << "." << i;
		rmdir(sstr.str().c_str());
	}

	/* Create a directory path; tempDirName does not exist here */
	string firstLvl1 = tempDirName + "/temp";
	string endLvls = "foo/bar";
	string tree1 = firstLvl1 + "/" + endLvls;
	cout << "Create a new directory " << tree1 << ": ";
	try {
		IO::Utility::makePath(tree1, 0777);
	} catch (const Error::Exception &e) {
		cout << "FAIL: Caught " << e.what() << endl;
		return (EXIT_FAILURE);
	}
	if (IO::Utility::fileExists(tree1))
		cout << "success." << endl;
	else
		cout << "fail." << endl;

	/*
	 * Sum the size of a directory and its contents.
	 */
	cout << "Size of " << tempDirName << " is "
	    << IO::Utility::sumDirectoryUsage(tempDirName) << "." << endl;

	/*
 	 * Copy the contents of a directory from the top level.
 	 * create a file at the bottom to exercise file copying as well.
 	 */
	tempFileName = "temp_file";
	IO::Utility::writeFile(textFile, tree1 + "/" + tempFileName);

	string firstLvl2 = tempDirName + "/temp2";
	string tree2 = firstLvl2 + "/" + endLvls;
	cout << "Copy a directory tree " << firstLvl1 << " => "
	     << firstLvl2 << ": ";
	try {
		IO::Utility::copyDirectoryContents(firstLvl1, firstLvl2, true);
	} catch (const Error::Exception &e) {
		cout << "FAIL: Caught " << e.what() << endl;
		return (EXIT_FAILURE);
	}
	/* Check that the leaf file exists in the copy */
	if (IO::Utility::fileExists(tree2 + "/" + tempFileName))
		cout << "success." << endl;
	else
		cout << "fail." << endl;

	cout << "Test that source tree " << firstLvl1
	     << " was removed during copy: ";
	if (IO::Utility::fileExists(firstLvl1))
		cout << "fail." << endl;
	else
		cout << "success." << endl;

	/* Remove a directory tree */
	cout << "Remove the directory " << tempDirName << ": ";
	try {
		IO::Utility::removeDirectory(tempDirName);
	} catch (const Error::Exception &e) {
		cout << "FAIL: Caught " << e.what() << endl;
		return (EXIT_FAILURE);
	}
	if (IO::Utility::fileExists(tempDirName))
		cout << "fail." << endl;
	else
		cout << "success." << endl;
	
	/*
	 * Test temporary files.
	 */
	cout << "Temporary file name: ";
	string testTempFile;
	try {
		testTempFile = IO::Utility::createTemporaryFile("test");
	} catch (const Error::Exception &e) {
		cout << "FAIL: Caught " << e.what() << endl;
		return (EXIT_FAILURE);
	}
	cout << testTempFile << endl;
	unlink(testTempFile.c_str());
	
	cout << "Write to exclusive temporary file: ";
	FILE *tempFp = nullptr;
	try {
		tempFp = IO::Utility::createTemporaryFile(testTempFile,
		    "test");
	} catch (const Error::Exception &e) {
		cout << "FAIL: Caught " << e.what() << endl;
		return (EXIT_FAILURE);
	}
	string testContents = "This is a test entry for the temp file";
	if (fwrite(testContents.c_str(), 1, testContents.size(), tempFp) != 
	    testContents.size()) {
		cout << "FAIL: Couldn't write" << endl;
		return (EXIT_FAILURE);
	}
	Memory::uint8Array tempContentsRead = 
	    IO::Utility::readFile(testTempFile);
	for (size_t i = 0; i < tempContentsRead.size(); i++) {
		if (tempContentsRead[i] != testContents[i]) {
			cout << "FAIL: Contents differ" << endl;
			return (EXIT_FAILURE);
		}
	}
	cout << "Success." << endl;
	fclose(tempFp);
	unlink(testTempFile.c_str());

	try {
		return (testPipe());
	} catch (const Error::NotImplemented&) {
#ifdef _WIN32
		return (EXIT_SUCCESS);
#else /* _WIN32 */
		std::cout << "FAIL: Caught NotImplemented from testPipe\n";
		return (EXIT_FAILURE);
#endif
	}
}

