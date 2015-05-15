#ifndef SRC_BASEBOXD_SWITCH_BEHAVIOR_HPP_
#define SRC_BASEBOXD_SWITCH_BEHAVIOR_HPP_

#include <rofl/common/crofdpt.h>

namespace basebox {

/* forward declarations */
class switch_behavior;

class switch_behavior_fabric {
public:
	static switch_behavior*
	get_behavior(const int type, const rofl::cdptid& dptid = rofl::cdptid());	// todo get behavior by evaluating switch description, features, port stats, table stats, ...
};

class switch_behavior {
public:
	switch_behavior(rofl::cdptid const& dptid);

	virtual
	~switch_behavior();

	// todo check if we need handle_dpt_open
	void
	handle_dpt_close(const rofl::cdptid& dptid) {}

	void
	handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {}

	void
	handle_flow_removed(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {}

	void
	handle_port_status(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg) {}

	void
	handle_error_message(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg) {}

	virtual int
	get_switch_type() { return -1; }

protected:

	rofl::cdptid						dptid;

private:
	/* currently non copyable objects only */
	switch_behavior( const switch_behavior& other ) = delete;
	switch_behavior& operator=( const switch_behavior& ) = delete;

};

} /* namespace basebox */

#endif /* SRC_BASEBOXD_SWITCH_BEHAVIOR_HPP_ */
