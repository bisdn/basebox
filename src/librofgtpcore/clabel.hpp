/*
 * clabel.hpp
 *
 *  Created on: 19.08.2014
 *      Author: andreas
 */

#ifndef CLABEL_HPP_
#define CLABEL_HPP_

#include <ostream>

#include <rofl/common/caddress.h>

#include "caddress_gtp.hpp"
#include "cteid.hpp"
#include "clogging.h"

namespace rofgtp {

class clabel {
public:

	/**
	 *
	 */
	clabel() {};

	/**
	 *
	 */
	~clabel() {};

	/**
	 *
	 */
	clabel(const cteid& teid) :
		teid(teid) {};

	/**
	 *
	 */
	clabel(const clabel& label) { *this = label; };

	/**
	 *
	 */
	clabel&
	operator= (const clabel& label) {
		if (this == &label)
			return *this;
		teid = label.teid;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const clabel& label) const {
		return (teid < label.teid);
	};

public:

	/**
	 *
	 */
	const cteid&
	get_teid() const { return teid; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const clabel& label) {
		os << rofcore::indent(0) << "<clabel >" << std::endl;
		rofcore::indent i(2); os << label.get_teid();
		return os;
	};

private:

	cteid		teid;
};



class clabel_in4 : public clabel {
public:

	/**
	 *
	 */
	clabel_in4() {};

	/**
	 *
	 */
	~clabel_in4() {};

	/**
	 *
	 */
	clabel_in4(
			const caddress_gtp_in4& saddr, const caddress_gtp_in4& daddr, const cteid& teid) :
				clabel(teid), saddr(saddr), daddr(daddr) {};

	/**
	 *
	 */
	clabel_in4(const clabel_in4& label) { *this = label; };

	/**
	 *
	 */
	clabel_in4&
	operator= (const clabel_in4& label) {
		if (this == &label)
			return *this;
		clabel::operator= (label);
		saddr = label.saddr;
		daddr = label.saddr;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const clabel_in4& label) const {
		return (clabel::operator< (label) && (saddr < label.saddr) && (daddr < label.daddr));
	};

public:

	/**
	 *
	 */
	const caddress_gtp_in4&
	get_saddr() const { return saddr; };

	/**
	 *
	 */
	const caddress_gtp_in4&
	get_daddr() const { return daddr; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const clabel_in4& label) {
		os << rofcore::indent(0) << "<clabel_in4 >" << std::endl;
		{ rofcore::indent i(2); os << dynamic_cast<const clabel&>( label ); };
		os << rofcore::indent(2) << "<saddr >" << std::endl;
		{ rofcore::indent i(4); os << label.get_saddr(); };
		os << rofcore::indent(2) << "<daddr >" << std::endl;
		{ rofcore::indent i(4); os << label.get_daddr(); };
		return os;
	};

private:

	caddress_gtp_in4	saddr;
	caddress_gtp_in4	daddr;
};



class clabel_in6 : public clabel {
public:

	/**
	 *
	 */
	clabel_in6() {};

	/**
	 *
	 */
	~clabel_in6() {};

	/**
	 *
	 */
	clabel_in6(
			const caddress_gtp_in6& saddr, const caddress_gtp_in6& daddr, const cteid& teid) :
				clabel(teid), saddr(saddr), daddr(daddr) {};

	/**
	 *
	 */
	clabel_in6(const clabel_in6& label) { *this = label; };

	/**
	 *
	 */
	clabel_in6&
	operator= (const clabel_in6& label) {
		if (this == &label)
			return *this;
		clabel::operator= (label);
		saddr = label.saddr;
		daddr = label.saddr;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const clabel_in6& label) const {
		return (clabel::operator< (label) && (saddr < label.saddr) && (daddr < label.daddr));
	};

public:

	/**
	 *
	 */
	const caddress_gtp_in6&
	get_saddr() const { return saddr; };

	/**
	 *
	 */
	const caddress_gtp_in6&
	get_daddr() const { return daddr; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const clabel_in6& label) {
		os << rofcore::indent(0) << "<clabel_in6 >" << std::endl;
		{ rofcore::indent i(2); os << dynamic_cast<const clabel&>( label ); };
		os << rofcore::indent(2) << "<saddr >" << std::endl;
		{ rofcore::indent i(4); os << label.get_saddr(); };
		os << rofcore::indent(2) << "<daddr >" << std::endl;
		{ rofcore::indent i(4); os << label.get_daddr(); };
		return os;
	};

private:

	caddress_gtp_in6	saddr;
	caddress_gtp_in6	daddr;
};

}; // end of namespace rofgtp

#endif /* CLABEL_HPP_ */
