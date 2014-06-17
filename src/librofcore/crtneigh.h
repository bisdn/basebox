/*
 * crtneigh.h
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#ifndef CRTNEIGH_H_
#define CRTNEIGH_H_ 1

#include <ostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#include <netlink/route/neighbour.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/logging.h>
#include <rofl/common/caddress.h>

namespace rofcore {

class crtneigh {
public:

	enum neigh_index_t {
		CRTNEIGH_ADDR_ALL = 0xffff,	// apply command to all neighbors
	};

public:

	/**
	 *
	 */
	crtneigh();

	/**
	 *
	 */
	virtual
	~crtneigh();

	/**
	 *
	 */
	crtneigh(
			const crtneigh& neigh);

	/**
	 *
	 */
	crtneigh&
	operator= (
			const crtneigh& neigh);

	/**
	 *
	 */
	crtneigh(
			struct rtnl_neigh* neigh);


public:

	/**
	 *
	 */
	int
	get_state() const { return state; };

	/**
	 *
	 */
	std::string
	get_state_s() const;

	/**
	 *
	 */
	unsigned int
	get_flags() const { return flags; };

	/**
	 *
	 */
	int
	get_ifindex() const { return ifindex; };

	/**
	 *
	 */
	const rofl::cmacaddr&
	get_lladdr() const { return lladdr; };

	/**
	 *
	 */
	int
	get_family() const { return family; };

	/**
	 *
	 */
	int
	get_type() const { return type; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, crtneigh const& neigh) {
		os << rofl::indent(0) << "<crtneigh: >" << std::endl;
		os << rofl::indent(2) << "<state: " 	<< neigh.state 		<< " >" << std::endl;
		os << rofl::indent(2) << "<flags: " 	<< neigh.flags 		<< " >" << std::endl;
		os << rofl::indent(2) << "<ifindex: " 	<< neigh.ifindex 	<< " >" << std::endl;
		os << rofl::indent(2) << "<lladdr: " 	<< neigh.lladdr 	<< " >" << std::endl;
		os << rofl::indent(2) << "<family: " 	<< neigh.family 	<< " >" << std::endl;
		os << rofl::indent(2) << "<type: " 		<< neigh.type 		<< " >" << std::endl;
		return os;
	};

private:

	int				state;
	unsigned int	flags;
	int				ifindex;
	rofl::cmacaddr	lladdr;
	int				family;
	int				type;
};




class crtneigh_in4 : public crtneigh {
public:

	/**
	 *
	 */
	crtneigh_in4() {};

	/**
	 *
	 */
	virtual
	~crtneigh_in4() {};

	/**
	 *
	 */
	crtneigh_in4(
			const crtneigh_in4& neigh) { *this = neigh; };

	/**
	 *
	 */
	crtneigh_in4&
	operator= (
			const crtneigh_in4& neigh) {
		if (this == &neigh)
			return *this;
		crtneigh::operator= (neigh);
		dst = neigh.dst;
		return *this;
	};

	/**
	 *
	 */
	crtneigh_in4(
			struct rtnl_neigh* neigh) :
				crtneigh(neigh) {

		nl_object_get((struct nl_object*)neigh); // increment reference counter by one

		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		std::string s_dst;
		nl_addr2str(rtnl_neigh_get_dst(neigh), s_buf, sizeof(s_buf));
		if (std::string(s_buf) != std::string("none"))
			s_dst.assign(nl_addr2str(rtnl_neigh_get_dst(neigh), s_buf, sizeof(s_buf)));
		else
			s_dst.assign("0.0.0.0/0");

		dst = rofl::caddress_in4(s_dst.c_str());

		nl_object_put((struct nl_object*)neigh); // decrement reference counter by one
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in4
	get_dst() const { return dst; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtneigh_in4& neigh) {
		os << rofl::indent(0) << "<crtneigh_in4: >" << std::endl;
		os << rofl::indent(2) << "<dst: >" << std::endl;
		rofl::indent i(4); os << neigh.dst;
		return os;
	};


	/**
	 *
	 */
	class crtneigh_in4_find_by_dst {
		rofl::caddress_in4 dst;
	public:
		crtneigh_in4_find_by_dst(const rofl::caddress_in4& dst) :
			dst(dst) {};
		bool
		operator() (const std::pair<uint16_t,crtneigh_in4>& p) {
			return (p.second.dst == dst);
		};
	};

private:

	rofl::caddress_in4	dst;
};





class crtneigh_in6 : public crtneigh {
public:

	/**
	 *
	 */
	crtneigh_in6() {};

	/**
	 *
	 */
	virtual
	~crtneigh_in6() {};

	/**
	 *
	 */
	crtneigh_in6(
			const crtneigh_in6& neigh) { *this = neigh; };

	/**
	 *
	 */
	crtneigh_in6&
	operator= (
			const crtneigh_in6& neigh) {
		if (this == &neigh)
			return *this;
		crtneigh::operator= (neigh);
		dst = neigh.dst;
		return *this;
	};

	/**
	 *
	 */
	crtneigh_in6(
			struct rtnl_neigh* neigh) :
				crtneigh(neigh) {

		nl_object_get((struct nl_object*)neigh); // increment reference counter by one

		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		std::string s_dst;
		nl_addr2str(rtnl_neigh_get_dst(neigh), s_buf, sizeof(s_buf));
		if (std::string(s_buf) != std::string("none"))
			s_dst.assign(nl_addr2str(rtnl_neigh_get_dst(neigh), s_buf, sizeof(s_buf)));
		else
			s_dst.assign("0000:0000:0000:0000:0000:0000:0000:0000");

		dst = rofl::caddress_in6(s_dst.c_str());

		nl_object_put((struct nl_object*)neigh); // decrement reference counter by one
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in6
	get_dst() const { return dst; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtneigh_in6& neigh) {
		os << rofl::indent(0) << "<crtneigh_in6: >" << std::endl;
		os << rofl::indent(2) << "<dst: >" << std::endl;
		rofl::indent i(4); os << neigh.dst;
		return os;
	};


	/**
	 *
	 */
	class crtneigh_in6_find_by_dst {
		rofl::caddress_in6 dst;
	public:
		crtneigh_in6_find_by_dst(const rofl::caddress_in6& dst) :
			dst(dst) {};
		bool
		operator() (const std::pair<uint16_t,crtneigh_in6>& p) {
			return (p.second.dst == dst);
		};
	};

private:

	rofl::caddress_in6	dst;
};



}; // end of namespace

#endif /* CRTNEIGH_H_ */

