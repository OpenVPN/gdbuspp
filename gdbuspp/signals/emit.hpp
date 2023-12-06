//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file signals/emit.hpp
 *
 * @brief  Declaration of the DBus::Signals::Emit base functionality
 *         for sending D-Bus signals
 */

#pragma once

#include <memory>
#include <glib.h>

#include "../connection.hpp"
#include "exceptions.hpp"
#include "target.hpp"


namespace DBus {
namespace Signals {

/**
 *  Base implementation for sending D-Bus signals on a D-Bus connection.
 *
 *  These signals are not tied to a specific DBus::Object::Base and can
 *  be used completely independent
 *
 */
class Emit
{
  public:
    using Ptr = std::shared_ptr<Emit>;

    /**
     *  Create a new Signals::Emit object
     *
     * @param conn        DBus::Connection to use for emitting signals
     *
     * @return Emit::Ptr
     */
    [[nodiscard]] static Emit::Ptr Create(DBus::Connection::Ptr conn)
    {
        return Emit::Ptr(new Emit(conn));
    }

    virtual ~Emit() noexcept = default;


    /**
     *  Add a signal recipient target.  Empty string are allowed, which
     *  will be treated as a "wildcard".  If all are empty, it will be
     *  considered broadcast.
     *
     *  At least one target must be added.
     *
     * @param busname       std::string of the recipient of a signal
     * @param object_path   std::string of the D-Bus object the signal references
     * @param interface     std::string of the interface scope of the D-Bus object
     */
    void AddTarget(const std::string &busname,
                   const std::string &object_path,
                   const std::string &interface);

    /**
     *  Similar to the @AddTarget() above, but will take a Target::Ptr
     *  object as argument instead.
     *
     * @param target   Target::Ptr object with the target details
     */
    void AddTarget(Target::Ptr target);

    /**
     *  Clears all the registered/added signal targets.
     *
     *  Before a new signal can be sent after calling this method, the
     *  AddTarget() method must have been called first.
     */
    void ClearTargets() noexcept;

    /**
     *   Send a D-Bus signal to all registered targets.
     *
     * @param signal_name   std::string with the signal name to use
     * @param params        GVariant object to object with data values
     *                      to provide with the signal
     *
     * @return Returns true if emitting the signal was successfully,
     *         otherwise false.  This method will return instantly on
     *         error if more signal targets have been set up.
     */
    virtual bool SendGVariant(const std::string &signal_name, GVariant *params) const;


  protected:
    Emit(Connection::Ptr conn);

  private:
    Connection::Ptr connection = nullptr;
    Target::Collection targets{};
};

} // namespace Signals
} // namespace DBus
