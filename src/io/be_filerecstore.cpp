/******************************************************************************
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 ******************************************************************************/

#include <sys/dir.h>
#include <sys/stat.h>
#include <iostream>
#include <be_filerecstore.h>

static const string _fileArea = "theFiles";

BiometricEvaluation::FileRecordStore::FileRecordStore(
    const string &name,
    const string &description)
    throw (ObjectExists, StrategyError) : RecordStore(name, description)
{
	_cursorPos = 1;
	_theFilesDir = RecordStore::canonicalName(_fileArea);
	if (mkdir(_theFilesDir.c_str(), S_IRWXU) != 0)
		throw StrategyError("Could not create file area directory");
	return;
}

BiometricEvaluation::FileRecordStore::FileRecordStore(
    const string &name)
    throw (ObjectDoesNotExist, StrategyError) : RecordStore(name)
{
	_cursorPos = 1;
	_theFilesDir = RecordStore::canonicalName(_fileArea);
	if (mkdir(_theFilesDir.c_str(), S_IRWXU) != 0)
	return;
}

uint64_t
BiometricEvaluation::FileRecordStore::getSpaceUsed()
    throw (StrategyError)
{
	DIR *dir;
	dir = opendir(_theFilesDir.c_str());
	if (dir == NULL)
		throw StrategyError("Cannot open store directory");

	uint64_t total = RecordStore::getSpaceUsed();
	struct dirent *entry;
	struct stat sb;
	string cname;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_ino == 0)
			continue;
		if (entry->d_type == DT_DIR)	/* skip '.' and '..' */
			continue;
		cname = entry->d_name;
		cname = FileRecordStore::canonicalName(cname);
		if (stat(cname.c_str(), &sb) != 0)	
			throw StrategyError("Cannot stat store file");
		total += sb.st_blocks * S_BLKSIZE;
	}	
	return (total);
}

void
BiometricEvaluation::FileRecordStore::insert( 
    const string &key,
    const void *data,
    const uint64_t size)
    throw (ObjectExists, StrategyError)
{
	string pathname = FileRecordStore::canonicalName(key);
	if (fileExists(pathname))
		throw ObjectExists();

	try {
		writeNewRecordFile(pathname, data, size);
	} catch (StrategyError& e) {
		throw e;
	}

	_count++;

}

void
BiometricEvaluation::FileRecordStore::remove( 
    const string &key)
    throw (ObjectDoesNotExist, StrategyError)
{
	string pathname = FileRecordStore::canonicalName(key);
	if (!fileExists(pathname))
		throw ObjectDoesNotExist();

	if (std::remove(pathname.c_str()) != 0)
		throw StrategyError("Could not remove " + pathname);

	_count--;
}

uint64_t
BiometricEvaluation::FileRecordStore::read(
    const string &key,
    void *data)
    throw (ObjectDoesNotExist, StrategyError)
{
	string pathname = FileRecordStore::canonicalName(key);
	if (!fileExists(pathname))
		throw ObjectDoesNotExist();

	/* Allow exceptions to propagate out of here */
	uint64_t size = getFileSize(pathname);
	std::FILE *fp = std::fopen(pathname.c_str(), "rb");
	if (fp == NULL)
		throw StrategyError("Could not open " + pathname);

	std::size_t sz = fread(data, 1, size, fp);
	std::fclose(fp);
	if (sz != size)
		throw StrategyError("Could not write " + pathname);
	return(size);
}

void
BiometricEvaluation::FileRecordStore::replace(
    const string &key,
    void * data,
    const uint64_t size)
    throw (ObjectDoesNotExist, StrategyError)
{
	string pathname = FileRecordStore::canonicalName(key);
	if (!fileExists(pathname))
		throw ObjectDoesNotExist();

	try {
		writeNewRecordFile(pathname, data, size);
	} catch (StrategyError& e) {
		throw e;
	}
}

uint64_t
BiometricEvaluation::FileRecordStore::length(
    const string &key)
    throw (ObjectDoesNotExist, StrategyError)
{
	string pathname = FileRecordStore::canonicalName(key);
	if (!fileExists(pathname))
		throw ObjectDoesNotExist();

	return (getFileSize(pathname));
}

void
BiometricEvaluation::FileRecordStore::flush(
    const string &key)
    throw (ObjectDoesNotExist, StrategyError)
{
	string pathname = FileRecordStore::canonicalName(key);
	if (!fileExists(pathname))
		throw ObjectDoesNotExist();

	/*
	 * There's nothing to implement here as the record writes result
	 * in the file being closed.
	 */
}

uint64_t
BiometricEvaluation::FileRecordStore::sequence(
    string &key,
    void *data,
    int cursor)
    throw (ObjectDoesNotExist, StrategyError)
{
	if ((cursor != BE_RECSTORE_SEQ_START) &&
	    (cursor != BE_RECSTORE_SEQ_NEXT))
		throw StrategyError("Invalid cursor position as argument");

	DIR *dir;
	dir = opendir(_theFilesDir.c_str());
	if (dir == NULL)
		throw StrategyError("Cannot open store directory");

	/* If the current cursor position is START, then it doesn't matter
	 * what the client requests; we start at the first record.
	*/
	if ((_cursor == BE_RECSTORE_SEQ_START) ||
	    (cursor == BE_RECSTORE_SEQ_START))
		_cursorPos = 1;

	if (_cursorPos > _count)	/* Client needs to start over */
		throw ObjectDoesNotExist("No record at position");

	struct dirent *entry;
	int i = 1;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_ino == 0)
			continue;
		if (entry->d_type == DT_DIR)	/* skip '.' and '..' */
			continue;
		if (i == _cursorPos)
			break;
		i++;
	}	
	/* Sanity check */
	if (i > _cursorPos)
		throw StrategyError("Record cursor position out of sync");
	string _key = entry->d_name;
	uint64_t _size = FileRecordStore::read(_key, data);
	key = _key;
	_cursor = cursor;
	_cursorPos = i + 1;
	return (_size);
}

/******************************************************************************/
/* Private method implementations.                                            */
/******************************************************************************/
bool
BiometricEvaluation::FileRecordStore::fileExists(const string &pathname)
{
	struct stat sb;

	if (stat(pathname.c_str(), &sb) == 0)
		return (true);
	else
		return (false);
}

/*
 * Get the size of a file managed by this class, a record.
 */
uint64_t
BiometricEvaluation::FileRecordStore::getFileSize(const string &name)
    throw (ObjectDoesNotExist, StrategyError)
{
	struct stat sb;

	if (stat(name.c_str(), &sb) != 0)
		throw StrategyError("Getting stats on file");
	return ((uint64_t)sb.st_size);

}

/*
 * Writes a file, replacing any data that previously existed in the file.
 */
void
BiometricEvaluation::FileRecordStore::writeNewRecordFile( 
    const string &name,
    const void *data,
    const uint64_t size)
    throw (StrategyError)
{
	std::FILE *fp = std::fopen(name.c_str(), "wb");
	if (fp == NULL)
		throw StrategyError("Could not open " + name);

	std::size_t sz = fwrite(data, 1, size, fp);
	std::fclose(fp);
	if (sz != size)
		throw StrategyError("Could not write " + name);
}

string
BiometricEvaluation::FileRecordStore::canonicalName(const string &name)
{
	return(_theFilesDir + '/' + name);
}
