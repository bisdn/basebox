/*
 * croftransactions.hpp
 *
 *  Created on: 21.04.2014
 *      Author: andreas
 */

#ifndef CROFTRANSACTIONS_HPP_
#define CROFTRANSACTIONS_HPP_

#include <set>
#include <iostream>
#include <algorithm>

#include "croftransaction.hpp"

namespace rofcore {

class croftransactions {

	std::set<croftransaction> 	transactions;

public:

	/**
	 *
	 */
	croftransactions();

	/**
	 *
	 */
	virtual
	~croftransactions();

	/**
	 *
	 */
	croftransactions(
			croftransactions const& roftransactions);

	/**
	 *
	 */
	croftransactions&
	operator= (
			croftransactions const& roftransactions);

public:

	/**
	 *
	 */
	croftransaction&
	add_ta(
			cctlxid const& ctlxid);

	/**
	 *
	 */
	croftransaction&
	set_ta(
			cctlxid const& ctlxid);

	/**
	 *
	 */
	croftransaction const&
	get_ta(
			cctlxid const& ctlxid) const;

	/**
	 *
	 */
	void
	drop_ta(
			cctlxid const& ctlxid);

	/**
	 *
	 */
	bool
	has_ta(
			cctlxid const& ctlxid) const;

	/**
	 *
	 */
	croftransaction&
	add_ta(
			cdptxid const& ctlxid);

	/**
	 *
	 */
	croftransaction&
	set_ta(
			cdptxid const& ctlxid);

	/**
	 *
	 */
	croftransaction const&
	get_ta(
			cdptxid const& ctlxid) const;

	/**
	 *
	 */
	void
	drop_ta(
			cdptxid const& ctlxid);

	/**
	 *
	 */
	bool
	has_ta(
			cdptxid const& ctlxid) const;

public:

	friend std::ostream&
	operator<< (std::ostream& os, croftransactions const& roftransactions) {
		// TODO
		return os;
	};
};

}; // end of namespace rofcore

#endif /* CROFTRANSACTIONS_HPP_ */
