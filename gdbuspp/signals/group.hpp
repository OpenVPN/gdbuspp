//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file signals/group.hpp
 *
 * @brief  Declaration of the DBus::Signals::Group, which is a helper
 *         class to provide a simple API to send various type of D-Bus
 *         signals from a single object using different methods.
 *         See tests/signal-group.cpp for an example.
 */

#pragma once

#include <map>
#include <memory>
#include <vector>

#include "../connection.hpp"
#include "../object/path.hpp"
#include "emit.hpp"


namespace DBus {
namespace Signals {

/**
 *  This defines a single signal variable member and the D-Bus data type
 *  it uses
 */
struct SignalArgument
{
  public:
    std::string name; ///< D-Bus signal argument name
    std::string type; ///< D-Bus signal argument data type
};

/**
 *  A collection of SignalArgument elements, which combined
 *  describes all the arguments (variables) sent by a specific
 *  D-Bus signal
 */
using SignalArgList = std::vector<struct SignalArgument>;


/**
 *  The Signals::Group class is a helper class to easily create
 *  an API in other classes to map C++ methods to sending specific
 *  and predefined D-Bus signals.
 *
 *  This class will take care of ensuring only known signals are
 *  sent and that the data they send is formatted correctly.
 */
class Group
{
  public:
    using Ptr = std::shared_ptr<Group>;

    /**
     *  Create a new Signals::Group container.  This object will keep an
     *  overview of all registered signals and their signal signature, to
     *  be able to generate the D-Bus introspection data and to validate
     *  signal parameters before sending them.
     *
     *  This class is not intended to be used directly, but through another
     *  class inheriting this class.
     *
     *  @code
     *
     *  class MySignalGroup : public DBus::Signals::Group
     *  {
     *    public:
     *    using Ptr = std::shared_ptr<MySignalGroup>;
     *
     *    MySignalGroup(DBus::Connection::Ptr connection)
     *        : DBus::Signals::Group(connection)
     *    {
     *    }
     *  };
     *
     *  int main()
     *  {
     *      // ...
     *      auto connection = DBus::Connection::Create(DBus::BusType::Session)
     *      auto my_signals = DBus::Signals::Group::Create<MySignalGroup>(connection);
     *      // ...
     *  }
     *
     *  @endcode
     *
     * @tparam C      Class inheriting Signals::Group, which is being
     *                instantiated
     * @tparam Args   Argument template for the constructor of the class being
     *                instantiated
     * @param conn    DBus::Connection which signals will be sent through
     * @param all     The list of arguments forwarded to the constructor
     * @return std::shared_ptr<C> to the newly created object
     */
    template <typename C, typename... Args>
    [[nodiscard]] static std::shared_ptr<C> Create(DBus::Connection::Ptr conn, Args &&...all)
    {
        return std::shared_ptr<C>(new C(conn, std::forward<Args>(all)...));
    }

    virtual ~Group() noexcept = default;


    /**
     *  Register a new signal this instantiated object supports
     *
     *  This method takes a std::vector of the SignalArgument struct, which
     *  can be done inline:
     *
     *  @code
     *        RegisterSignal("signalName", {{"arg1", "s"}, {"arg2", "i"}});
     *  @endcode
     *
     *  The line above will allow a signal named 'signalName' to be sent
     *  via this object, where it takes two arguments - arg1 (string) and
     *  arg2 (int)
     *
     * @param signal_name std::string of the signal name to register
     * @param signal_type SignalArgList containing all the arguments this signal
     *                    will contain
     */
    void RegisterSignal(const std::string &signal_name, const SignalArgList signal_type);


    /**
     *  Generates the D-Bus introspection data for all the registered
     *  signals.  This is typically not used directly, but called via the
     *  @DBus::Object::Base::GenerateIntrospection() method.
     *
     * @return const std::string containing the XML introspection data
     */
    const std::string GenerateIntrospection();


    /**
     *  Update the D-Bus object path used when sending signals
     *
     * @param new_path
     */
    void ModifyPath(const Object::Path &new_path) noexcept;

    /**
     *  Add a signal recipient target.  Empty string are allowed, which
     *  will be treated as a "wildcard".  If all are empty, it will be
     *  considered broadcast.
     *
     *  At least one target must be added.
     *
     *  This method is a bit different from the one found in
     *  Signals::Emit::AddTarget().  This one being provided in Signals::Groups
     *  only takes the busname as the target; the object path and interface
     *  is already declared via the constructor.
     *
     * @param busname       std::string of the recipient of a signal
     */
    void AddTarget(const std::string &busname);


    /**
     *  Sends the D-Bus signals with the provided parameters
     *
     *  This calls @Emit::SendGVariant() with the same arguments once
     *  the signal name and the expected D-Bus data type signature has
     *  been validated.
     *
     * @param signal_name   std::string of the D-Bus signal to send
     * @param param         GVariant glib2 object containing the signal
     *                      data payload
     */
    void SendGVariant(const std::string &signal_name, GVariant *param);


    /**
     *  Create a separate distribution list for a group of signal targets
     *
     * @param groupname  std::string with a name for the distribution list
     */
    void GroupCreate(const std::string &groupname);


    /**
     *  Deletes a specific distribution list
     *
     * @param groupname  std::string with the distribution list to remove
     */
    void GroupRemove(const std::string &groupname);


    /**
     *  Adds a D-Bus bus name where signals will be sent for a specific
     *  distribution list
     *
     * @param groupname  std::string with the group name to add the recipient
     * @param busname    std::String with the bus name of the recipient
     */
    void GroupAddTarget(const std::string &groupname, const std::string &busname);


    /**
     *  Similar to GroupAddTarget(), where it takes a std::vector of recipients
     *  to add to the group list.
     *
     * @param groupname  std::string with the group name to add the recipient
     * @param list       std::vector<std::String> with bus names of the recipients
     */
    void GroupAddTargetList(const std::string &groupname, const std::vector<std::string> &list);


    /**
     *  Removes all the recipients in a distribution list
     *
     * @param groupname
     */
    void GroupClearTargets(const std::string &groupname);


    /**
     *  Sends a signal to all recipients in a single distribution list
     *
     * @param groupname    std::string with the group name of recipients to signal
     * @param signal_name  std::string of the D-Bus signal to send
     * @param param         GVariant glib2 object containing the signal
     *                      data payload
     */
    void GroupSendGVariant(const std::string &groupname,
                           const std::string &signal_name,
                           GVariant *param);


  protected:
    /**
     * Signals::Group contructor
     *
     * @param conn              DBus::Connection object where signals will be sent
     * @param object_path       Object::Path of the D-Bus object the signal references
     * @param object_interface  std::string of the interface scope of the D-Bus object
     */
    Group(DBus::Connection::Ptr conn,
          const Object::Path &object_path,
          const std::string &object_interface);

  private:
    /// D-Bus connection used to emit signals
    DBus::Connection::Ptr connection{};

    /**
     *  All signals registered via RegisterSignal ends up here.  This is
     *  used to gather all the needed information for the D-Bus introspection
     *  generation
     */
    std::map<std::string, SignalArgList> registered_signals;

    /**
     * This is a look-up cache of all the D-Bus type string used for a
     * specific signal.  This is created based on the data collected in
     * registered_signals.
     */
    std::map<std::string, std::string> type_cache;

    /// D-Bus object path these signals are sent from from
    Object::Path object_path;

    /// D-Bus object interface these signals belongs to
    const std::string object_interface;

    /**
     *  Signal groups; each named group has their own Emit object with
     *  their own set of signal targets.
     *
     *  The calls not using a group name explicitly uses the "__default__"
     *  group name.  This group name is reserved and may not be used by
     *  any of the Group methods.
     */
    std::map<std::string, Signals::Emit::Ptr> signal_groups{};
};

} // namespace Signals
} // namespace DBus
