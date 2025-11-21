//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file signals/emit.cpp
 *
 * @brief  Implementation of the DBus::Signals::Emit base functionality
 *         for sending D-Bus signals
 */

#include <iostream>
#include <string>
#include <glib.h>

#include "../features/debug-log.hpp"
#include "../glib2/utils.hpp"
#include "../glib2/strings.hpp"
#include "../object/path.hpp"
#include "exceptions.hpp"
#include "emit.hpp"


namespace DBus {
namespace Signals {

void Emit::AddTarget(const std::string &busname,
                     const Object::Path &object_path,
                     const std::string &interface)
{
    targets.push_back(Target::Create(busname, object_path, interface));
}


void Emit::AddTarget(Target::Ptr target)
{
    targets.push_back(target);
}


void Emit::ClearTargets() noexcept
{
    targets.clear();
}


bool Emit::SendGVariant(const std::string &signal_name, GVariant *params) const
{
    if (!connection || !connection->Check())
    {
        GDBUSPP_LOG("D-Bus connection is not valid");
        return false;
    }

    if (targets.empty())
    {
        throw Signals::Exception("No targets provided.  Cannot send any signal");
    }


    GError *err = nullptr;
    g_variant_ref_sink(params);
    for (const auto &tgt : targets)
    {
        GDBUSPP_LOG("Signals::Emit -- " << tgt << "; "
                                        << "signal_name='" << signal_name << "'"
                                        << ", params=" << glib2::Utils::DumpToString(params));
        if (!g_dbus_connection_emit_signal(connection->ConnPtr(),
                                           str2gchar(tgt->busname),
                                           str2gchar(tgt->object_path),
                                           str2gchar(tgt->object_interface),
                                           str2gchar(signal_name),
                                           params,
                                           &err))
        {
            std::cerr << "[GDBus++ Error: " << tgt << "] "
                      << "Failed to send signal '" << signal_name << "' ";
            if (err)
            {
                std::cerr << "glib2 Error: " << err->message;
            }
            std::cerr << std::endl;
            return false;
        }
    }
    return true;
}


Emit::Emit(Connection::Ptr conn)
    : connection(conn)
{
}

} // namespace Signals
} // namespace DBus
