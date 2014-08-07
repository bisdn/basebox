/*
 * cmemberport.cpp
 *
 *  Created on: 07.08.2014
 *      Author: andreas
 */

#include "cmemberport.hpp"

using namespace ethcore;


void
cmemberport::handle_dpt_open(
		rofl::crofdpt& dpt)
{

}

void
cmemberport::handle_dpt_close(
		rofl::crofdpt& dpt)
{

}

void
cmemberport::handle_packet_in(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{

}

void
cmemberport::handle_flow_removed(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{

}

void
cmemberport::handle_port_status(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg)
{

}

void
cmemberport::handle_error_message(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg)
{

}



