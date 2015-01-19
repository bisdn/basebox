/*
 * ctermdev.hpp
 *
 *  Created on: 28.08.2014
 *      Author: andreas
 */

#ifndef CTERMDEV_HPP_
#define CTERMDEV_HPP_

#include <inttypes.h>

#include <ostream>
#include <exception>

#include <rofl/common/cdpid.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fipv6frame.h>
#include <rofl/common/crofbase.h>

#include <roflibs/netlink/ctundev.hpp>
#include <roflibs/netlink/cprefix.hpp>
#include <roflibs/netlink/cnetdev.hpp>
#include <roflibs/netlink/cnetlink.hpp>
#include <roflibs/netlink/clogging.hpp>

namespace roflibs {
namespace gtp {

class eTermDevBase : public std::runtime_error {
public:
	eTermDevBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eTermDevNotFound : public eTermDevBase {
public:
	eTermDevNotFound(const std::string& __arg) : eTermDevBase(__arg) {};
};

class ctermdev : public rofcore::ctundev {
public:

	/**
	 *
	 */
	ctermdev(rofcore::cnetdev_owner *netdev_owner, const std::string& devname,
			const rofl::cdptid& dptid, uint8_t ofp_table_id) :
				rofcore::ctundev(netdev_owner, devname), state(STATE_DETACHED),
				dptid(dptid), ofp_table_id(ofp_table_id)
	{};


	/**
	 *
	 */
	virtual
	~ctermdev() {};


public:

	/**
	 *
	 */
	void
	add_prefix_in4(const rofcore::cprefix_in4& prefix) {
		try {
			if (prefixes_in4.find(prefix) != prefixes_in4.end()) {
				return;
			}
			rofcore::cnetlink::get_instance().add_addr_in4(get_ifindex(), prefix.get_addr(), prefix.get_prefixlen());
			prefixes_in4.insert(prefix);
			if (STATE_ATTACHED == state) {
				handle_dpt_open(prefix);
			}
		} catch (rofcore::eNetLinkFailed& e) {
			rofcore::logging::debug << "[rofcore][ctermdev][add_prefix_in4] failed to set address via netlink" << std::endl;
		} catch (rofl::eRofDptNotFound& e) {
			rofcore::logging::debug << "[rofcore][ctermdev][add_prefix_in4] dpt not found" << std::endl;
		}
	};

	/**
	 *
	 */
	void
	drop_prefix_in4(const rofcore::cprefix_in4& prefix) {
		try {
			if (prefixes_in4.find(prefix) == prefixes_in4.end()) {
				return;
			}
			rofcore::cnetlink::get_instance().drop_addr_in4(get_ifindex(), prefix.get_addr(), prefix.get_prefixlen());
			if (STATE_ATTACHED == state) {
				handle_dpt_close(prefix);
			}
			prefixes_in4.erase(prefix);
		} catch (rofcore::eNetLinkFailed& e) {
			rofcore::logging::debug << "[rofcore][ctermdev][drop_prefix_in4] failed to set address via netlink" << std::endl;
		} catch (rofl::eRofDptNotFound& e) {
			rofcore::logging::debug << "[rofcore][ctermdev][drop_prefix_in4] dpt not found" << std::endl;
		}
	};

	/**
	 *
	 */
	bool
	has_prefix_in4(const rofcore::cprefix_in4& prefix) const {
		return (not (prefixes_in4.find(prefix) == prefixes_in4.end()));
	};

public:

	/**
	 *
	 */
	void
	add_prefix_in6(const rofcore::cprefix_in6& prefix) {
		try {
			if (prefixes_in6.find(prefix) != prefixes_in6.end()) {
				return;
			}
			rofcore::cnetlink::get_instance().add_addr_in6(get_ifindex(), prefix.get_addr(), prefix.get_prefixlen());
			prefixes_in6.insert(prefix);
			if (STATE_ATTACHED == state) {
				handle_dpt_open(prefix);
			}
		} catch (rofcore::eNetLinkFailed& e) {
			rofcore::logging::debug << "[rofcore][ctermdev][add_prefix_in6] failed to set address via netlink" << std::endl;
		} catch (rofl::eRofDptNotFound& e) {
			rofcore::logging::debug << "[rofcore][ctermdev][add_prefix_in6] dpt not found" << std::endl;
		}
	};

	/**
	 *
	 */
	void
	drop_prefix_in6(const rofcore::cprefix_in6& prefix) {
		try {
			if (prefixes_in6.find(prefix) == prefixes_in6.end()) {
				return;
			}
			rofcore::cnetlink::get_instance().drop_addr_in6(get_ifindex(), prefix.get_addr(), prefix.get_prefixlen());
			if (STATE_ATTACHED == state) {
				handle_dpt_close(prefix);
			}
			prefixes_in6.erase(prefix);
		} catch (rofcore::eNetLinkFailed& e) {
			rofcore::logging::debug << "[rofcore][ctermdev][drop_prefix_in6] failed to set address via netlink" << std::endl;
		} catch (rofl::eRofDptNotFound& e) {
			rofcore::logging::debug << "[rofcore][ctermdev][drop_prefix_in6] dpt not found" << std::endl;
		}
	};

	/**
	 *
	 */
	bool
	has_prefix_in6(const rofcore::cprefix_in6& prefix) const {
		return (not (prefixes_in6.find(prefix) == prefixes_in6.end()));
	};

public:

	/**
	 *
	 */
	void
	handle_dpt_open() {
		state = STATE_ATTACHED;
		for (std::set<rofcore::cprefix_in4>::const_iterator
				it = prefixes_in4.begin(); it != prefixes_in4.end(); ++it) {
			handle_dpt_open(*it);
		}
		for (std::set<rofcore::cprefix_in6>::const_iterator
				it = prefixes_in6.begin(); it != prefixes_in6.end(); ++it) {
			handle_dpt_open(*it);
		}
	};

	/**
	 *
	 */
	void
	handle_dpt_close() {
		state = STATE_DETACHED;
		for (std::set<rofcore::cprefix_in4>::const_iterator
				it = prefixes_in4.begin(); it != prefixes_in4.end(); ++it) {
			handle_dpt_close(*it);
		}
		for (std::set<rofcore::cprefix_in6>::const_iterator
				it = prefixes_in6.begin(); it != prefixes_in6.end(); ++it) {
			handle_dpt_close(*it);
		}
	};

private:

	/**
	 *
	 */
	void
	handle_dpt_open(const rofcore::cprefix_in4& prefix);

	/**
	 *
	 */
	void
	handle_dpt_close(const rofcore::cprefix_in4& prefix);

	/**
	 *
	 */
	void
	handle_dpt_open(const rofcore::cprefix_in6& prefix);

	/**
	 *
	 */
	void
	handle_dpt_close(const rofcore::cprefix_in6& prefix);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const ctermdev& termdev) {
		os << rofcore::indent(0) << "<ctermdev "
				<< "devname: " << termdev.get_devname() << " "
				<< "#in4-prefix(es): " << (int)termdev.prefixes_in4.size() << " "
				<< "#in6-prefix(es): " << (int)termdev.prefixes_in6.size() << " >" << std::endl;
		rofcore::indent i(2);
		for (std::set<rofcore::cprefix_in4>::const_iterator
				it = termdev.prefixes_in4.begin(); it != termdev.prefixes_in4.end(); ++it) {
			os << *(it);
		}
		for (std::set<rofcore::cprefix_in6>::const_iterator
				it = termdev.prefixes_in6.begin(); it != termdev.prefixes_in6.end(); ++it) {
			os << *(it);
		}
		return os;
	};

private:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t						state;
	rofl::cdptid							dptid;
	uint8_t									ofp_table_id;
	std::set<rofcore::cprefix_in4>			prefixes_in4;
	std::set<rofcore::cprefix_in6>			prefixes_in6;
};

}; // end of namespace gtp
}; // end of namespace roflibs

#endif /* CTERMDEV_HPP_ */
