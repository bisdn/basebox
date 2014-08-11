#include "cethbase.hpp"


int
main(int argc, char** argv)
{
	return ethcore::cethbase::get_instance().run(argc, argv);
}

