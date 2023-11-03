//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file signals/group.cpp
 *
 * @brief  Implementation of the DBus::Signals::Group
 */

#include <glib.h>

#include "exceptions.hpp"
#include "group.hpp"


namespace DBus {
namespace Signals {

void Group::RegisterSignal(const std::string &signal_name, const SignalArgList signal_type)
{
    try
    {
        (void)registered_signals.at(signal_name);
        throw Signals::Exception("Signal data type enforcement cannot be redefined");
    }
    catch (const std::out_of_range &)
    {
        registered_signals[signal_name] = signal_type;
    };
};


const std::string Group::GenerateIntrospection()
{
    std::ostringstream ret;

    for (const auto &sig : registered_signals)
    {
        // Prepare a temp storage to use for the D-Bus data type cache
        std::ostringstream type;

        type << "(";
        ret << "    <signal name='" << sig.first << "'>" << std::endl;
        for (const auto &spec : sig.second)
        {
            ret << "      <arg type='" << spec.type << "' "
                << "name='" << spec.name << "'/>" << std::endl;

            // Collect data type for the argument for the type cache
            type << spec.type;
        }
        ret << "    </signal>" << std::endl;

        // Save the gathered type into the type cache
        type << ")";
        type_cache[sig.first] = type.str();
    }
    return ret.str();
}


void Group::AddTarget(const std::string &busname,
                      const std::string &object_path,
                      const std::string &interface)
{
    Emit::AddTarget(busname, object_path, interface);
};


void Group::AddTarget(Target::Ptr target)
{
    Emit::AddTarget(target);
};


void Group::SendGVariant(const std::string &signal_name, GVariant *param)
{
    // If the type cache is empty, run the introspection generation
    // to build the cache
    if (type_cache.empty())
    {
        (void)GenerateIntrospection();
    }
    try
    {
        // Retrieve the expected type from the type cache for the signal ...
        std::string &exp_type = type_cache.at(signal_name);

        // ... and validate it with what we have recevied
        std::string param_type(g_variant_get_type_string(param));
        if (exp_type != param_type)
        {
            std::ostringstream err;
            err << "Invalid data type for '" << signal_name << "' "
                << "Expected '" << exp_type << "' "
                << "but received '" << param_type << "'";
            throw Signals::Exception(err.str());
        }
        Emit::SendGVariant(signal_name, param);
        g_variant_unref(param);
    }
    catch (const std::out_of_range &)
    {
        throw Signals::Exception("Not a registered signal: " + signal_name);
    }
}


Group::Group(DBus::Connection::Ptr conn)
    : Emit(conn)
{
}

} // namespace Signals
} // namespace DBus
