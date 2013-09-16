/*
 * cdirectory.h
 *
 *  Created on: 16.09.2013
 *      Author: andreas
 */

#ifndef CDIRECTORY_H_
#define CDIRECTORY_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __cplusplus
}
#endif

#include <map>
#include <string>
#include <iostream>
#include <exception>

#include "cfile.h"

namespace rutils
{

class eDirBase			: public std::exception {};

class eDirSyscall		: public eDirBase {};
class eDirAccess		: public eDirSyscall {};
class eDirEMfile		: public eDirSyscall {};
class eDirENfile		: public eDirSyscall {};
class eDirNoEnt			: public eDirSyscall {};
class eDirNoMem			: public eDirSyscall {};
class eDirNotDir		: public eDirSyscall {};
class eDirBadFd			: public eDirSyscall {};

class eDirExists		: public eDirBase {};
class eDirNotFound		: public eDirBase {};

class cdirectory
{
	std::string 	dirpath;
	DIR				*dir_handle;

	std::map<std::string, cdirectory*>  dirs;
	std::map<std::string, cfile*>  		files;

public:

	cdirectory(
			std::string const& dirpath);

	virtual
	~cdirectory();

private:

	void
	purge_files();

	void
	purge_dirs();

	void
	readdir();

	void
	addfile(
			std::string const& filename);

	void
	delfile(
			std::string const& filename);

	void
	adddir(
			std::string const& dirname);

	void
	deldir(
			std::string const& dirname);

public:

	friend std::ostream&
	operator<< (std::ostream& os, cdirectory const& dir) {
		os << "<cdirectory[" << dir.dirpath << "] ";
			os << "<dirs: ";
			for (std::map<std::string, cdirectory*>::const_iterator
					it = dir.dirs.begin(); it != dir.dirs.end(); ++it) {
				os << "\t" << *(it->second) << " ";
			}
			os << "> ";
			os <<"<files: ";
			for (std::map<std::string, cfile*>::const_iterator
					it = dir.files.begin(); it != dir.files.end(); ++it) {
				os << "\t" << *(it->second) << " ";
			}
			os << ">";
		os << " >";
		return os;
	};
};

}; // end of namespace

#endif /* CDIRECTORY_H_ */
