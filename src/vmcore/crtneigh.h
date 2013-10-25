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

#include <rofl/common/cmacaddr.h>
#include <rofl/common/caddress.h>

namespace dptmap
{

class crtneigh
{
public:

	enum neigh_index_t {
		CRTNEIGH_ADDR_ALL = 0xffff,	// apply command to all neighbors
	};

	int				state;
	unsigned int	flags;
	int				ifindex;
	rofl::cmacaddr	lladdr;
	rofl::caddress	dst;
	int				family;
	int				type;


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
			crtneigh const& neigh);


	/**
	 *
	 */
	crtneigh&
	operator= (
			crtneigh const& neigh);


	/**
	 *
	 */
	crtneigh(
			struct rtnl_neigh* neigh);


public:


	/**
	 *
	 */
	int get_state() const { return state; };


	/**
	 *
	 */
	std::string get_state_s() const;


	/**
	 *
	 */
	unsigned int get_flags() const { return flags; };


	/**
	 *
	 */
	int get_ifindex() const { return ifindex; };


	/**
	 *
	 */
	rofl::cmacaddr get_lladdr() const { return lladdr; };


	/**
	 *
	 */
	rofl::caddress get_dst() const { return dst; };


	/**
	 *
	 */
	int get_family() const { return family; };


	/**
	 *
	 */
	int get_type() const { return type; };


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, crtneigh const& neigh)
	{
		os << "<crtneigh: ";
			os << "state: " 	<< neigh.state << " ";
			os << "flags: " 	<< neigh.flags << " ";
			os << "ifindex: " 	<< neigh.ifindex << " ";
			os << "lladdr: " 	<< neigh.lladdr << " ";
			os << "dst: " 		<< neigh.dst << " ";
			os << "family: " 	<< neigh.family << " ";
			os << "type: " 		<< neigh.type << " ";
		os << ">";
		return os;
	};


	/**
	 *
	 */
	class crtneigh_find_by_dst : public std::unary_function<crtneigh,bool> {
		rofl::caddress dst;
	public:
		crtneigh_find_by_dst(rofl::caddress const& dst) :
			dst(dst) {};
		bool
		operator() (std::pair<uint16_t,crtneigh> const& p) {
			return (p.second.dst == dst);
		};
	};
};

}; // end of namespace

#endif /* CRTNEIGH_H_ */

