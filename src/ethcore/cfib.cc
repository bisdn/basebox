/*
 * cfib.cc
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#include <cfib.h>

using namespace ethercore;

std::map<uint64_t, std::map<uint16_t, cfib*> > cfib::fibs;


/*static*/
cfib&
cfib::get_fib(uint64_t dpid, uint16_t vid)
{
	if (cfib::fibs[dpid].find(vid) == cfib::fibs[dpid].end()) {
		new cfib(dpid, vid);
	}
	return *(cfib::fibs[dpid][vid]);
}


cfib::cfib(uint64_t dpid, uint16_t vid) :
		dpid(dpid),
		vid(vid),
		rofbase(0),
		dpt(0)
{
	if (cfib::fibs[dpid].find(vid) != cfib::fibs[dpid].end()) {
		throw eFibExists();
	}
	cfib::fibs[dpid][vid] = this;
}



cfib::~cfib()
{
	cfib::fibs[dpid].erase(vid);
}



void
cfib::dpt_bind(rofl::crofbase *rofbase, rofl::cofdpt *dpt)
{
	if (((0 != this->rofbase) && (rofbase != this->rofbase)) ||
			((0 != this->dpt) && (dpt != this->dpt))) {
		throw eFibBusy();
	}
	this->rofbase 	= rofbase;
	this->dpt 		= dpt;
}



void
cfib::dpt_release(rofl::crofbase *rofbase, rofl::cofdpt *dpt)
{
	if (((0 != this->rofbase) && (rofbase != this->rofbase)) ||
			((0 != this->dpt) && (dpt != this->dpt))) {
		throw eFibInval();
	}
	this->rofbase	= 0;
	this->dpt		= 0;

	for (std::map<rofl::cmacaddr, cfibentry*>::iterator
			it = fibtable.begin(); it != fibtable.end(); ++it) {
		delete (it->second);
	}
	fibtable.clear();
}



void
cfib::fib_timer_expired(cfibentry *entry)
{
	if (fibtable.find(entry->get_lladdr()) != fibtable.end()) {
#if 0
		entry->set_out_port_no(OFPP12_FLOOD);
#else
		fibtable[entry->get_lladdr()]->flow_mod_delete();
		fibtable.erase(entry->get_lladdr());
		delete entry;
#endif
	}

	std::cerr << "EXPIRED: " << *this << std::endl;
}



void
cfib::fib_update(
		rofl::crofbase *rofbase,
		rofl::cofdpt *dpt,
		rofl::cmacaddr const& src,
		uint32_t in_port)
{
	std::cerr << "UPDATE: src: " << src << std::endl;

	// update cfibentry for src/inport
	if (fibtable.find(src) == fibtable.end()) {
		fibtable[src] = new cfibentry(this, rofbase, dpt, src, in_port);
		fibtable[src]->flow_mod_add();

		std::cerr << "UPDATE[2.1]: " << *this << std::endl;

	} else {
		fibtable[src]->set_out_port_no(in_port);

		std::cerr << "UPDATE[3.1]: " << *this << std::endl;

	}
}



cfibentry&
cfib::fib_lookup(
		rofl::crofbase *rofbase,
		rofl::cofdpt *dpt,
		rofl::cmacaddr const& dst,
		rofl::cmacaddr const& src,
		uint32_t in_port)
{
	std::cerr << "LOOKUP: dst: " << dst << " src: " << src << std::endl;

	// sanity checks
	if (src.is_multicast()) {
		throw eFibInval();
	}

	fib_update(rofbase, dpt, src, in_port);

	if (dst.is_multicast()) {
		throw eFibInval();
	}

	// find out-port for dst
	if (fibtable.find(dst) == fibtable.end()) {

		fibtable[dst] = new cfibentry(this, rofbase, dpt, dst, OFPP12_FLOOD);
		fibtable[dst]->flow_mod_add();

		std::cerr << "LOOKUP[1]: " << *this << std::endl;
	}

	return *(fibtable[dst]);
}





