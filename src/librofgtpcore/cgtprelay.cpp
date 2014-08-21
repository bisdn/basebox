/*
 * cgtprelay.cpp
 *
 *  Created on: 21.08.2014
 *      Author: andreas
 */

#include "cgtprelay.hpp"
#include "cgtpcore.hpp"

using namespace rofgtp;


void
cgtprelay::handle_read(
		rofl::csocket& socket)
{
	unsigned int pkts_rcvd_in_round = 0;

	try {

#if 0
		while (true) {

			// read from socket more bytes, at most "msg_len - msg_bytes_read"
			int rc = socket.recv((void*)(fragment->somem() + msg_bytes_read), msg_len - msg_bytes_read);

		}
#endif

	} catch (rofl::eSocketRxAgain& e) {

		// more bytes are needed, keep pointer to msg in "fragment"
		rofcore::logging::debug << "[rofl][sock] eSocketRxAgain: no further data available on socket" << std::endl;

	} catch (rofl::eSysCall& e) {

		rofcore::logging::warn << "[rofl][sock] failed to read from socket: " << e << std::endl;

		// close socket, as it seems, we are out of sync
		socket.close();

	} catch (rofl::RoflException& e) {

		rofcore::logging::warn << "[rofl][sock] dropping invalid message: " << e << std::endl;

		// close socket, as it seems, we are out of sync
		socket.close();
	}

}
