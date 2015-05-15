#include "switch_behavior.hpp"
#include "roflibs/of-dpa/switch_behavior_ofdpa.hpp"

namespace basebox {

switch_behavior::switch_behavior(const rofl::cdptid& dptid) :
		dptid(dptid)
{
	// TODO Auto-generated constructor stub

}

switch_behavior::~switch_behavior()
{
	// TODO Auto-generated destructor stub
}

switch_behavior*
switch_behavior_fabric::get_behavior(const int type, const rofl::cdptid& dptid)
{
	// todo update according to header
	// todo keep track of existing dptid<=>behavior mappings
	switch (type) {
		case 1:
			return new switch_behavior_ofdpa(dptid);
			break;
		default:
			return new switch_behavior(dptid);
			break;
	}
}

} /* namespace basebox */
