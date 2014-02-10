/*
 * qmfagent.cc
 *
 * Copyright (C) 2013 BISDN GmbH <andi@bisdn.de>
 *
 *
 *  Created on: 26.07.2013
 *      Author: andreas
 */

#include "qmfagent.h"

using namespace qmf;
using namespace rofl;

qmfagent* qmfagent::sqmfagent = (qmfagent*)0;

qmfagent&
qmfagent::get_instance() {
	if (0 == qmfagent::sqmfagent) {
		qmfagent::sqmfagent = new qmfagent();
	}
	return *(qmfagent::sqmfagent);
}


qmfagent::qmfagent():qmf_package("de.bisdn.ethcore")
{
	//Do nothing
}
		

void qmfagent::init(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);
 
	//Add additional arguments
	env_parser.add_option(
		coption(true, REQUIRED_ARGUMENT, 'q', "qmfaddr", "qmf broker address",
		std::string("127.0.0.1")));

	env_parser.add_option(
		coption(true, REQUIRED_ARGUMENT, 'x', "ethcoreID", "qmf ethcore ID",
		std::string("ethcore-0")));

	//Parse
	env_parser.parse_args();
	
	//Recover
	broker_url = env_parser.get_arg('q');
	ethcore_id = env_parser.get_arg('x');

	connection = qpid::messaging::Connection(broker_url, "{reconnect:True}");
	connection.open();

	session = qmf::AgentSession(connection, "{interval:10}");
	session.setVendor("bisdn.de");
	session.setProduct("ethcore");
	session.open();

	notifier = qmf::posix::EventNotifier(session);

	set_qmf_schema();

	// create single qxdpd instance
	qethcore.data = qmf::Data(sch_ethcore);
	qethcore.data.setProperty("ethcoreID", ethcore_id);
	std::stringstream name("ethcore");
	qethcore.addr = session.addData(qethcore.data, name.str());

	register_filedesc_r(notifier.getHandle());
}



qmfagent::~qmfagent()
{
	session.close();
	connection.close();
}



void
qmfagent::handle_timeout(int opaque)
{
	switch (opaque) {
	default: {

	} break;
	}
}



void
qmfagent::handle_revent(int fd)
{
	qmf::AgentEvent event;
	while (session.nextEvent(event, qpid::messaging::Duration::IMMEDIATE)) {
		switch (event.getType()) {
		case qmf::AGENT_METHOD: {
			bool running = method(event);
			(void)running;
		} break;
		default: {
			// do nothing
		} break;
		}
	}
}






void
qmfagent::set_qmf_schema()
{
	// exception
	sch_exception = qmf::Schema(qmf::SCHEMA_TYPE_DATA, qmf_package, "exception");
	sch_exception.addProperty(qmf::SchemaProperty("whatHappened", 	qmf::SCHEMA_DATA_STRING));
	sch_exception.addProperty(qmf::SchemaProperty("howBad", 		qmf::SCHEMA_DATA_INT));
	sch_exception.addProperty(qmf::SchemaProperty("details", 		qmf::SCHEMA_DATA_MAP));

    // xdpd
    sch_ethcore = qmf::Schema(qmf::SCHEMA_TYPE_DATA, qmf_package, "ethcore");
	sch_ethcore.addProperty(qmf::SchemaProperty("ethcoreID", 	qmf::SCHEMA_DATA_STRING));

	qmf::SchemaMethod vlanAddMethod("vlanAdd", "{desc:'add vlan'}");
	vlanAddMethod.addArgument(qmf::SchemaProperty("dpid",		qmf::SCHEMA_DATA_INT,		"{dir:INOUT}"));
	vlanAddMethod.addArgument(qmf::SchemaProperty("vid",		qmf::SCHEMA_DATA_INT,		"{dir:INOUT}"));
	sch_ethcore.addMethod(vlanAddMethod);

	qmf::SchemaMethod vlanDropMethod("vlanDrop", "{desc:'drop vlan'}");
	vlanDropMethod.addArgument(qmf::SchemaProperty("dpid",		qmf::SCHEMA_DATA_INT,		"{dir:INOUT}"));
	vlanDropMethod.addArgument(qmf::SchemaProperty("vid",		qmf::SCHEMA_DATA_INT,		"{dir:INOUT}"));
	sch_ethcore.addMethod(vlanDropMethod);

	qmf::SchemaMethod portAddMethod("portAdd", "{desc:'add port to vlan'}");
	portAddMethod.addArgument(qmf::SchemaProperty("dpid",		qmf::SCHEMA_DATA_INT,		"{dir:INOUT}"));
	portAddMethod.addArgument(qmf::SchemaProperty("vid",		qmf::SCHEMA_DATA_INT,		"{dir:INOUT}"));
	portAddMethod.addArgument(qmf::SchemaProperty("devname",	qmf::SCHEMA_DATA_STRING,	"{dir:INOUT}"));
	portAddMethod.addArgument(qmf::SchemaProperty("tagged",		qmf::SCHEMA_DATA_BOOL,		"{dir:INOUT}"));
	sch_ethcore.addMethod(portAddMethod);

	qmf::SchemaMethod portDropMethod("portDrop", "{desc:'drop port from vlan'}");
	portDropMethod.addArgument(qmf::SchemaProperty("dpid",		qmf::SCHEMA_DATA_INT,		"{dir:INOUT}"));
	portDropMethod.addArgument(qmf::SchemaProperty("vid",		qmf::SCHEMA_DATA_INT,		"{dir:INOUT}"));
	portDropMethod.addArgument(qmf::SchemaProperty("devname",	qmf::SCHEMA_DATA_STRING,	"{dir:INOUT}"));
	sch_ethcore.addMethod(portDropMethod);

    session.registerSchema(sch_exception);
    session.registerSchema(sch_ethcore);
}



bool
qmfagent::method(qmf::AgentEvent& event)
{
	std::string const& name = event.getMethodName();
	(void)name;

	try {

		if (name == "vlanAdd") {
			return methodVlanAdd(event);
		}
		else if (name == "vlanDrop") {
			return methodVlanDrop(event);
		}
		else if (name == "portAdd") {
			return methodPortAdd(event);
		}
		else if (name == "portDrop") {
			return methodPortDrop(event);
		}
		else {
			session.raiseException(event, "command not found");
		}

	} catch (std::exception const& e) {

		std::cerr << "EXCEPTION: " << e.what() << std::endl;
		session.raiseException(event, e.what());
		throw;
	}

	return true;
}



bool
qmfagent::methodVlanAdd(qmf::AgentEvent& event)
{
	try {
		uint64_t dpid 			= event.getArguments()["dpid"].asUint64();
		uint16_t vid			= event.getArguments()["vid"].asUint16();

		event.addReturnArgument("dpid", dpid);
		event.addReturnArgument("vid", vid);

		rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
		versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
		ethercore::ethcore::get_instance(versionbitmap).add_vlan(dpid, vid);

		session.methodSuccess(event);

		return true;

	} catch (...) {
		session.raiseException(event, "add vlan failed");

	}
	return false;
}



bool
qmfagent::methodVlanDrop(qmf::AgentEvent& event)
{
	try {
		uint64_t dpid 			= event.getArguments()["dpid"].asUint64();
		uint16_t vid			= event.getArguments()["vid"].asUint16();

		event.addReturnArgument("dpid", dpid);
		event.addReturnArgument("vid", vid);

		rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
		versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
		ethercore::ethcore::get_instance(versionbitmap).drop_vlan(dpid, vid);

		session.methodSuccess(event);

		return true;

	} catch (...) {
		session.raiseException(event, "drop vlan failed");

	}
	return false;
}




bool
qmfagent::methodPortAdd(qmf::AgentEvent& event)
{
	try {
		uint64_t dpid 			= event.getArguments()["dpid"].asUint64();
		uint16_t vid			= event.getArguments()["vid"].asUint16();
		std::string devname		= event.getArguments()["devname"].asString();
		bool tagged				= event.getArguments()["tagged"].asBool();

		event.addReturnArgument("dpid", 	dpid);
		event.addReturnArgument("vid", 		vid);
		event.addReturnArgument("devname", 	devname);
		event.addReturnArgument("tagged", 	tagged);

		rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
		versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
		ethercore::ethcore::get_instance(versionbitmap).add_port_to_vlan(dpid, devname, vid, tagged);

		session.methodSuccess(event);

		return true;

	} catch (...) {
		session.raiseException(event, "add port failed");

	}
	return false;
}



bool
qmfagent::methodPortDrop(qmf::AgentEvent& event)
{
	try {
		uint64_t dpid 			= event.getArguments()["dpid"].asUint64();
		uint16_t vid			= event.getArguments()["vid"].asUint16();
		std::string devname		= event.getArguments()["devname"].asString();

		event.addReturnArgument("dpid", 	dpid);
		event.addReturnArgument("vid", 		vid);
		event.addReturnArgument("devname", 	devname);

		rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
		versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
		ethercore::ethcore::get_instance(versionbitmap).drop_port_from_vlan(dpid, devname, vid);

		session.methodSuccess(event);

		return true;

	} catch (...) {
		session.raiseException(event, "drop port failed");

	}
	return false;
}



