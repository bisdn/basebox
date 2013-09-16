/*
 * cfile.h
 *
 *  Created on: 16.09.2013
 *      Author: andreas
 */

#ifndef CFILE_H_
#define CFILE_H_

#include <iostream>

namespace rutils
{

class cfile
{
	std::string			dirpath;
	std::string			filename;

public:

	cfile(
			std::string const& filename,
			std::string const& dirpath = std::string("./"));

	virtual
	~cfile();


public:

	friend std::ostream&
	operator<< (std::ostream& os, cfile const& file) {
		os << "<cfile[" << file.dirpath << "/" << file.filename << "]>";
		return os;
	};
};

};

#endif /* CFILE_H_ */
