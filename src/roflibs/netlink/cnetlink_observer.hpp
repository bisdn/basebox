/*
 * cnetlink_observer.hpp
 *
 *  Created on: Apr 29, 2016
 *      Author: tobi
 */

#ifndef SRC_ROFLIBS_NETLINK_CNETLINK_OBSERVER_HPP_
#define SRC_ROFLIBS_NETLINK_CNETLINK_OBSERVER_HPP_

namespace rofcore {

class cnetlink_common_observer {
public:
  /**
   *
   */
  cnetlink_common_observer() {}

  /**
   *
   */
  virtual ~cnetlink_common_observer() {}

  /**
   *
   * @param rtl
   */
  virtual void link_created(unsigned int ifindex) {}

  /**
   *
   * @param rtl
   */
  virtual void link_updated(const crtlink &newlink) {}

  /**
   *
   * @param ifindex
   */
  virtual void link_deleted(unsigned int ifindex) {}

  /**
   *
   * @param rtl
   */
  virtual void addr_in4_created(unsigned int ifindex, uint16_t adindex) {}

  /**
   *
   * @param rtl
   */
  virtual void addr_in4_updated(unsigned int ifindex, uint16_t adindex) {}

  /**
   *
   * @param ifindex
   */
  virtual void addr_in4_deleted(unsigned int ifindex, uint16_t adindex) {}

  /**
   *
   * @param rtl
   */
  virtual void addr_in6_created(unsigned int ifindex, uint16_t adindex) {}

  /**
   *
   * @param rtl
   */
  virtual void addr_in6_updated(unsigned int ifindex, uint16_t adindex) {}

  /**
   *
   * @param ifindex
   */
  virtual void addr_in6_deleted(unsigned int ifindex, uint16_t adindex) {}

  /**
   *
   * @param rtl
   */
  virtual void route_in4_created(uint8_t table_id, unsigned int rtindex) {}

  /**
   *
   * @param rtl
   */
  virtual void route_in4_updated(uint8_t table_id, unsigned int rtindex) {}

  /**
   *
   * @param ifindex
   */
  virtual void route_in4_deleted(uint8_t table_id, unsigned int rtindex) {}

  /**
   *
   * @param rtl
   */
  virtual void route_in6_created(uint8_t table_id, unsigned int rtindex) {}

  /**
   *
   * @param rtl
   */
  virtual void route_in6_updated(uint8_t table_id, unsigned int rtindex) {}

  /**
   *
   * @param ifindex
   */
  virtual void route_in6_deleted(uint8_t table_id, unsigned int rtindex) {}

  virtual void neigh_ll_created(unsigned int ifindex, uint16_t nbindex) {}

  virtual void neigh_ll_updated(unsigned int ifindex, uint16_t nbindex) {}

  virtual void neigh_ll_deleted(unsigned int ifindex, uint16_t nbindex) {}

  /**
   *
   * @param rtl
   */
  virtual void neigh_in4_created(unsigned int ifindex, uint16_t nbindex) {}

  /**
   *
   * @param rtl
   */
  virtual void neigh_in4_updated(unsigned int ifindex, uint16_t nbindex) {}

  /**
   *
   * @param ifindex
   */
  virtual void neigh_in4_deleted(unsigned int ifindex, uint16_t nbindex) {}

  /**
   *
   * @param rtl
   */
  virtual void neigh_in6_created(unsigned int ifindex, uint16_t nbindex) {}

  /**
   *
   * @param rtl
   */
  virtual void neigh_in6_updated(unsigned int ifindex, uint16_t nbindex) {}

  /**
   *
   * @param ifindex
   */
  virtual void neigh_in6_deleted(unsigned int ifindex, uint16_t nbindex) {}

private:
  cnetlink_common_observer(const cnetlink_common_observer &other) =
      delete; // non construction-copyable
  cnetlink_common_observer &
  operator=(const cnetlink_common_observer &) = delete; // non copyable
};

class auto_reg_cnetlink_common_observer : public cnetlink_common_observer {
public:
  auto_reg_cnetlink_common_observer() {
    cnetlink::get_instance().subscribe(this);
  }

  ~auto_reg_cnetlink_common_observer() override {
    cnetlink::get_instance().unsubscribe(this);
  }
};
}
#endif /* SRC_ROFLIBS_NETLINK_CNETLINK_OBSERVER_HPP_ */
