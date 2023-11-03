//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file signals/event.hpp
 *
 * @brief  The DBus::Signals::Event object is the container of signal
 *         information received.  This is object is being passed to the
 *         callback function receiving the signal
 */

#pragma once

#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <glib.h>


namespace DBus {
namespace Signals {


/**
 *  Container for signal information received
 */
class Event
{
  public:
    using Ptr = std::shared_ptr<Signals::Event>;

    /**
     *   Creates a new signal event object
     *
     * @param sender           std::string with the D-Bus signal sender unique bus ID
     * @param object_path      std::string with the D-Bus object path the signal is related to
     * @param object_interface std::string with the object interface scope of the signal
     * @param signal_name      std::string with the signal name of this event
     * @param params           GVariant * to the object containing additional signal values
     *
     * @return Event::Ptr
     */
    static Event::Ptr Create(const std::string &sender,
                             const std::string &object_path,
                             const std::string &object_interface,
                             const std::string &signal_name,
                             GVariant *params)
    {
        return Event::Ptr(new Event(sender, object_path, object_interface, signal_name, params));
    }

    ~Event() noexcept = default;


    friend std::ostream &operator<<(std::ostream &os, const Event::Ptr &ev)
    {
        std::ostringstream r;
        r << "Signal::Event("
          << "sender=" << ev->sender << ", "
          << "path=" << ev->object_path << ", "
          << "signal_name=" << ev->signal_name << ", ";
        if (ev->params)
        {
            r << "parameter_type=" << std::string(g_variant_get_type_string(ev->params));
        }
        r << ")";
        return os << r.str();
    }


    const std::string sender;
    const std::string object_path;
    const std::string object_interface;
    const std::string signal_name;
    GVariant *params;

  private:
    Event(const std::string &sender_,
          const std::string &object_path_,
          const std::string &object_interface_,
          const std::string &signal_name_,
          GVariant *params_)
        : sender(sender_), object_path(object_path_),
          object_interface(object_interface_), signal_name(signal_name_),
          params(params_)
    {
    }
};

/**
 *  Declaration of the signal callback function API receiving the Event object
 */
using CallbackFnc = std::function<void(DBus::Signals::Event::Ptr &event)>;


} // namespace Signals
} // namespace DBus
