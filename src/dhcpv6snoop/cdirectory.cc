/*
 * cdirectory.cc
 *
 *  Created on: 16.09.2013
 *      Author: andreas
 */

#include "cdirectory.h"

using namespace rutils;


cdirectory::cdirectory(std::string const& dirpath) :
		dirpath(dirpath)
{
	if ((dir_handle = opendir(dirpath.c_str())) == 0) {
		switch (errno) {
		case EACCES: throw eDirAccess();
		case EMFILE: throw eDirEMfile();
		case ENFILE: throw eDirENfile();
		case ENOENT: throw eDirNoEnt();
		case ENOMEM: throw eDirNoMem();
		case ENOTDIR: throw eDirNotDir();
		}
	}

	readdir();
}



cdirectory::cdirectory(
		std::string const& dirname,
		std::string const& dirpath) :
		dirpath(dirpath + "/" + dirname)
{
	if ((dir_handle = opendir(dirpath.c_str())) == 0) {
		switch (errno) {
		case EACCES: throw eDirAccess();
		case EMFILE: throw eDirEMfile();
		case ENFILE: throw eDirENfile();
		case ENOENT: throw eDirNoEnt();
		case ENOMEM: throw eDirNoMem();
		case ENOTDIR: throw eDirNotDir();
		}
	}

	readdir();
}


cdirectory::~cdirectory()
{
	purge_dirs();

	purge_files();

	if (closedir(dir_handle) < 0) {
		switch (errno) {
		case EBADF: throw eDirBadFd();
		}
	}
}



void
cdirectory::purge_files()
{
	for (std::map<std::string, cfile*>::iterator
			it = files.begin(); it != files.end(); ++it) {
		delete (it->second);
	}
	files.clear();
}



void
cdirectory::purge_dirs()
{
	for (std::map<std::string, cdirectory*>::iterator
			it = dirs.begin(); it != dirs.end(); ++it) {
		delete (it->second);
	}
	dirs.clear();
}



void
cdirectory::readdir()
{
	purge_dirs();

	purge_files();

	struct dirent **namelist;
	int n;

    n = scandir(dirpath.c_str(), &namelist, &select, alphasort);
    if (n < 0)
        perror("scandir");
    else {
        while (n--) {
            fprintf(stderr, "%s\n", namelist[n]->d_name);
    		struct stat statbuf;

    		if (::stat(namelist[n]->d_name, &statbuf) < 0) {
    			continue;
    		}

    		if (S_ISREG(statbuf.st_mode)) {
    			addfile(namelist[n]->d_name);
    		}
    		else if (S_ISDIR(statbuf.st_mode)) {
    			adddir(namelist[n]->d_name);
    		}
    		else {

    		}
            free(namelist[n]);
        }
        free(namelist);
    }
}



int
cdirectory::select(const struct dirent* dir)
{
	std::string name(dir->d_name);

	if (name == "..")
		return 0;
	if (name == ".")
		return 0;

	return 1;
}



void
cdirectory::addfile(std::string const& filename)
{
	if (files.find(filename) != files.end()) {
		return;
	}
	files[filename] = new cfile(filename, dirpath);
}



void
cdirectory::delfile(std::string const& filename)
{
	if (files.find(filename) == files.end()) {
		return;
	}
	delete files[filename];
	files.erase(filename);
}



void
cdirectory::adddir(
		std::string const& dirname)
{
	if (dirs.find(dirname) != dirs.end()) {
		return;
	}
	dirs[dirname] = new cdirectory(dirname, dirpath);
}



void
cdirectory::deldir(
		std::string const& dirname)
{
	if (dirs.find(dirname) == dirs.end()) {
		return;
	}
	delete dirs[dirname];
	dirs.erase(dirname);
}



