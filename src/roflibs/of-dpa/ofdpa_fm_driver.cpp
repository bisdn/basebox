#include "ofdpa_fm_driver.hpp"
#include "ofdpa_datatypes.h"
#include "roflibs/netlink/clogging.hpp"

#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/cofflowmod.h>
#include <rofl/common/openflow/openflow_common.h>
#include <rofl/common/openflow/coxmatch.h>

#include <linux/if_ether.h>

namespace basebox {

namespace ofdpa {

#define HAS_MASK_FLAG (1 << 8)

	/* OXM Flow match field types for OpenFlow experimenter class. */
	enum oxm_tlv_match_fields {
	OXM_TLV_EXPR_VRF = (rofl::openflow::OFPXMC_EXPERIMENTER << 16)
			| (OFPXMT_OFX_VRF << 9) | 2,
	OXM_TLV_EXPR_VRF_MASK = (rofl::openflow::OFPXMC_EXPERIMENTER << 16)
			| (OFPXMT_OFX_VRF << 9) | 4 | HAS_MASK_FLAG,
	};

}; // end of namespace ofdpa


class coxmatch_ofb_vrf : public rofl::openflow::coxmatch_exp {
public:
	coxmatch_ofb_vrf(uint16_t vrf) :
		coxmatch_exp(ofdpa::OXM_TLV_EXPR_VRF, OFDPA_EXP_ID, vrf)
	{}

	coxmatch_ofb_vrf(uint16_t vrf, uint16_t mask) :
		coxmatch_exp(ofdpa::OXM_TLV_EXPR_VRF_MASK, OFDPA_EXP_ID, vrf, mask)
	{}

	coxmatch_ofb_vrf(const coxmatch_exp& oxm) :
		coxmatch_exp(oxm)
	{}

	virtual
	~coxmatch_ofb_vrf()
	{}

	friend std::ostream&
	operator<< (std::ostream& os, const coxmatch_ofb_vrf& oxm) {
		os << dynamic_cast<const coxmatch&>(oxm);
		os << rofcore::indent(2) << "<coxmatch_ofb_vlan_vid >" << std::endl;
		os << rofcore::indent(4) << "<vlan-vid: 0x" << std::hex
				<< (int) oxm.get_u16value() << "/0x" << (int) oxm.get_u16mask()
				<< std::dec << " >" << std::endl;
		return os;
	}
};

static inline uint64_t
gen_flow_mod_type_cookie(uint64_t val) {
	return (val<< 8*7);
}

ofdpa_fm_driver::ofdpa_fm_driver(rofl::crofdpt& dpt) :
		dpt(dpt),
		default_idle_timeout(30) // todo idle timeout should be configurable
{
}

ofdpa_fm_driver::~ofdpa_fm_driver()
{
}

void
ofdpa_fm_driver::enable_port_pvid_ingress(const std::string &port_name, uint16_t vid)
{
	// XXX This contradicts the OF-DPA ASS (p. 64) "The VLAN_VID value cannot
	// be used in a VLAN Filtering rule."
	// todo Send ticket to BCM to clarify
	enable_port_vid_ingress(port_name, vid);

	// check params
	assert(vid < 0x1000);
	uint32_t port_no;

	try {
		port_no = dpt.get_ports().get_port(port_name).get_port_no();
	} catch (rofl::openflow::ePortsNotFound& e) {
		rofcore::logging::error << __PRETTY_FUNCTION__ << " not an of-port:" << std::endl;
		return;
	}

	rofl::openflow::cofflowmod fm(dpt.get_version());

	fm.set_command(rofl::openflow::OFPFC_ADD);
	fm.set_table_id(OFDPA_FLOW_TABLE_ID_VLAN);

	fm.set_idle_timeout(0);
	fm.set_hard_timeout(0);
	fm.set_priority(2);
	fm.set_cookie(gen_flow_mod_type_cookie(OFDPA_FTT_VLAN_VLAN_ASSIGNMENT) | 0);

	fm.set_match().set_in_port(port_no);
	fm.set_match().set_vlan_vid(0, rofl::openflow::OFPVID_PRESENT | 0);

	fm.set_instructions().set_inst_apply_actions().set_actions().
			add_action_set_field(rofl::cindex(0)).set_oxm(
			rofl::openflow::coxmatch_ofb_vlan_vid(vid));

	// set vrf
//	fm.set_instructions().set_inst_apply_actions().set_actions().
//			add_action_set_field(rofl::cindex(1)).set_oxm(
//					coxmatch_ofb_vrf(vid)); // currently vid == vrf

	fm.set_instructions().set_inst_goto_table().set_table_id(
			OFDPA_FLOW_TABLE_ID_TERMINATION_MAC);

	rofcore::logging::debug << __FUNCTION__ << ": send flow-mod:" << std::endl
			<< fm;

	dpt.send_flow_mod_message(rofl::cauxid(0), fm);
}

void
ofdpa_fm_driver::enable_port_vid_ingress(const std::string &port_name, uint16_t vid)
{
	assert(vid < 0x1000);
	uint32_t port_no;

	try {
		port_no = dpt.get_ports().get_port(port_name).get_port_no();
	} catch (rofl::openflow::ePortsNotFound& e) {
		rofcore::logging::error << __PRETTY_FUNCTION__ << " not an of-port:" << std::endl;
		return;
	}

	rofl::openflow::cofflowmod fm(dpt.get_version());

	 // todo check what happens if this is added two times?
	fm.set_command(rofl::openflow::OFPFC_ADD);
	fm.set_table_id(OFDPA_FLOW_TABLE_ID_VLAN);

	fm.set_idle_timeout(0);
	fm.set_hard_timeout(0);
	fm.set_priority(3);
	fm.set_cookie(gen_flow_mod_type_cookie(OFDPA_FTT_VLAN_VLAN_FILTERING) | 0);

	fm.set_match().set_in_port(port_no);
	fm.set_match().set_vlan_vid(rofl::openflow::OFPVID_PRESENT | vid, rofl::openflow::OFPVID_PRESENT | 0xfff);

	// set vrf
//	fm.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(
//			rofl::cindex(0)).set_oxm(coxmatch_ofb_vrf(vid)); // currently vid == vrf

	fm.set_instructions().set_inst_goto_table().set_table_id(
			OFDPA_FLOW_TABLE_ID_TERMINATION_MAC);

	rofcore::logging::debug << __FUNCTION__ << ": send flow-mod:" << std::endl
			<< fm;

	dpt.send_flow_mod_message(rofl::cauxid(0), fm);
}

uint32_t
ofdpa_fm_driver::enable_port_vid_egress(const std::string &port_name, uint16_t vid, bool untagged)
{
	// equals l2 interface group, so maybe rename this

	assert(vid < 0x1000);
	uint32_t port_no;

	try {
		port_no = dpt.get_ports().get_port(port_name).get_port_no();
	} catch (rofl::openflow::ePortsNotFound& e) {
		rofcore::logging::error << __PRETTY_FUNCTION__ << " not an of-port:" << std::endl;
		return rofl::openflow::OFPG_MAX;
	}

	uint32_t group_id = (0x0fff & vid) << 16 | (0xffff & port_no);
	rofl::openflow::cofgroupmod gm(dpt.get_version());

	gm.set_command(rofl::openflow::OFPGC_ADD);
	gm.set_type(rofl::openflow::OFPGT_INDIRECT);
	gm.set_group_id(group_id);

	rofl::cindex i(0);
	if (untagged) {
		gm.set_buckets().add_bucket(0).set_actions().
				add_action_pop_vlan(i++);
	}
	gm.set_buckets().set_bucket(0).set_actions().
			add_action_output(i++).set_port_no(port_no);

	rofcore::logging::debug << __FUNCTION__ << ": send group-mod:" << std::endl << gm;

	dpt.send_group_mod_message(rofl::cauxid(0), gm);

	return group_id;
}

uint32_t
ofdpa_fm_driver::enable_group_l2_multicast(uint16_t vid, uint16_t id,
		const std::list<uint32_t>& l2_interfaces, bool update)
{
	assert(vid < 0x1000);

	static const uint16_t identifier = 0x1000;
	static uint16_t current_ident = 0;
	uint16_t next_ident = current_ident;
	if (update) {
		next_ident ^= identifier;
	}

	uint32_t group_id = 3 << 28 | (0x0fff & vid) << 16
			| (0xffff & (id | next_ident));

	rofl::openflow::cofgroupmod gm(dpt.get_version());

	gm.set_command(rofl::openflow::OFPGC_ADD);
	gm.set_type(rofl::openflow::OFPGT_ALL);
	gm.set_group_id(group_id);

	uint32_t bucket_id = 0;

	for (const uint32_t &i : l2_interfaces) {
		gm.set_buckets().add_bucket(bucket_id++).set_actions()
			.add_action_group(rofl::cindex(0)).set_group_id(i);
	}

	rofcore::logging::debug << __FUNCTION__ << ": send group-mod:" << std::endl
			<< gm;
	dpt.send_group_mod_message(rofl::cauxid(0), gm);

	if (update) {
		// update arp policy
		enable_policy_arp(vid, group_id, true);

		// delete old entry
		rofl::openflow::cofgroupmod gm(dpt.get_version());

		gm.set_command(rofl::openflow::OFPGC_DELETE);
		gm.set_type(rofl::openflow::OFPGT_ALL);
		gm.set_group_id(3 << 28 | (0x0fff & vid) << 16 | (0xffff & (id | current_ident)));

		rofcore::logging::debug << __FUNCTION__ << ": send group-mod delete" << std::endl;
		dpt.send_group_mod_message(rofl::cauxid(0), gm); // xxx this does not work currently

		current_ident = next_ident;
	}

	return group_id;
}

#if 0
void
ofdpa_fm_driver::enable_bridging_dlf_vlan(uint16_t vid, uint32_t group_id,
		bool do_pkt_in)
{
	assert(vid < 0x1000);

	rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

	rofl::openflow::cofflowmod fm(dpt.get_version());
	fm.set_table_id(OFDPA_FLOW_TABLE_ID_BRIDGING);

	fm.set_idle_timeout(0);
	fm.set_hard_timeout(0);
	fm.set_priority(2);
	fm.set_cookie(gen_flow_mod_type_cookie(OFDPA_FTT_BRIDGING_DLF_VLAN)|0);

	fm.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT, 0x0fff);

	if (do_pkt_in) {
		fm.set_out_port(rofl::openflow::OFPP_CONTROLLER);

		fm.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(
				rofl::openflow::OFPP_CONTROLLER);
	}

	// fixme not supported in this table... check with BCM
	//fm.set_instructions().set_inst_clear_actions();

	fm.set_instructions().set_inst_goto_table().set_table_id(
			OFDPA_FLOW_TABLE_ID_ACL_POLICY);

	rofcore::logging::debug << __FUNCTION__ << ": send flow-mod:" << std::endl
			<< fm;
	dpt.send_flow_mod_message(rofl::cauxid(0), fm);
}
#endif

void
ofdpa_fm_driver::enable_policy_arp(uint16_t vid, uint32_t group_id, bool update)
{
	assert(vid < 0x1000);

	rofl::openflow::cofflowmod fm(dpt.get_version());
	fm.set_table_id(OFDPA_FLOW_TABLE_ID_ACL_POLICY);

	fm.set_idle_timeout(0);
	fm.set_hard_timeout(0);
	fm.set_priority(2);
	fm.set_cookie(gen_flow_mod_type_cookie(OFDPA_FTT_POLICY_ACL_IPV4_VLAN) | 0);

	fm.set_command(
			update ? rofl::openflow::OFPFC_MODIFY : rofl::openflow::OFPFC_ADD);

	//fm.set_match().set_eth_dst(rofl::cmacaddr("ff:ff:ff:ff:ff:ff"));
	fm.set_match().set_eth_type(ETH_P_ARP);

//	fm.set_out_port(rofl::openflow::OFPP_CONTROLLER);

	fm.set_instructions().set_inst_apply_actions().set_actions().add_action_output(
			rofl::cindex(0)).set_port_no(
			rofl::openflow::OFPP_CONTROLLER);

//	fm.set_instructions().set_inst_write_actions().set_actions()
//			.add_action_group(rofl::cindex(0)).set_group_id(group_id);

	rofcore::logging::debug << __FUNCTION__ << ": send flow-mod:" << std::endl
			<< fm;

	dpt.send_flow_mod_message(rofl::cauxid(0), fm);
}

void ofdpa_fm_driver::add_bridging_unicast_vlan(const rofl::cmacaddr& mac,
		uint16_t vid, uint32_t port_no, bool permanent)
{
	assert(vid < 0x1000);

	rofl::openflow::cofflowmod fm(dpt.get_version());
	fm.set_table_id(OFDPA_FLOW_TABLE_ID_BRIDGING);

	fm.set_idle_timeout(permanent ? 0 : default_idle_timeout);
	fm.set_hard_timeout(0);
	fm.set_priority(2);
	fm.set_cookie(gen_flow_mod_type_cookie(OFDPA_FTT_BRIDGING_UNICAST_VLAN) | port_no); // fixme cookiebox here?

	if (not permanent) {
		fm.set_flags(rofl::openflow::OFPFF_SEND_FLOW_REM);
	}

	fm.set_command(rofl::openflow::OFPFC_ADD);

	// fixme do not allow multicast mac here
	fm.set_match().set_eth_dst(mac);
	fm.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);

	uint32_t group_id = (0x0fff & vid) << 16 | (0xffff & port_no);
	fm.set_instructions().set_inst_write_actions().set_actions()
			.add_action_group(rofl::cindex(0)).set_group_id(group_id);
	fm.set_instructions().set_inst_goto_table().set_table_id(
			OFDPA_FLOW_TABLE_ID_ACL_POLICY);

	rofcore::logging::debug << __FUNCTION__ << ": send flow-mod:" << std::endl
			<< fm;

	dpt.send_flow_mod_message(rofl::cauxid(0), fm);
}

void ofdpa_fm_driver::remove_bridging_unicast_vlan(const rofl::cmacaddr& mac,
		uint16_t vid, uint32_t port_no)
{
	assert(vid < 0x1000);

	rofl::openflow::cofflowmod fm(dpt.get_version());
	fm.set_table_id(OFDPA_FLOW_TABLE_ID_BRIDGING);

	fm.set_priority(2);
	fm.set_cookie(gen_flow_mod_type_cookie(OFDPA_FTT_BRIDGING_UNICAST_VLAN) | port_no); // fixme cookiebox here?

	// todo do this strict?
	fm.set_command(rofl::openflow::OFPFC_DELETE);

	// fixme do not allow multicast mac here
	fm.set_match().set_eth_dst(mac);
	fm.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);

	dpt.send_flow_mod_message(rofl::cauxid(0), fm);
}

} /* namespace basebox */
