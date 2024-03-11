//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file signals/signal.cpp
 *
 * @brief  Implementaion of the DBus::Signals::Signal interface
 *         This is used to create a generic signal class which to easily
 *         generate the data needed to send the signals.  This is typically
 *         used via Signals::Group::CreateSignal<>()
 */

#include <memory>
#include <string>
#include <utility>
#include <glib.h>

#include "emit.hpp"
#include "group.hpp"
#include "signal.hpp"


namespace DBus::Signals {

Signal::Signal(Emit::Ptr em, const std::string &sig_name)
    : emitter(em), signal_name(sig_name)
{
}


const std::string &Signal::GetName() const
{
    return signal_name;
}


const SignalArgList &Signal::GetArguments() const
{
    return signal_arguments;
}

void Signal::SetArguments(const SignalArgList &sigargs)
{
    if (!signal_arguments.empty())
    {
        throw Signals::Exception("Signal arguments already set");
    }
    signal_arguments = std::move(sigargs);
    dbustype = SignalArgSignature(signal_arguments);
}


const char *Signal::GetDBusType() const
{
    if (dbustype.empty())
    {
        throw Signals::Exception("No signal data signature declared");
    }
    return dbustype.c_str();
}


const bool Signal::EmitSignal(GVariant *params) const
{
    // Retrieve the D-Bus data type of the GVariant value container
    // and the data type this signal is declared as; to verify it is
    // as expected
    const std::string signature{GetDBusType()};
    const std::string param_type{g_variant_get_type_string(params)};
    if (param_type != signature)
    {
        std::ostringstream err;
        err << "Signal signature does not match expectations: "
            << param_type << " != " << signature;
        throw Signals::Exception(err.str());
    }

    // Send the signal via the Signals::Emit object
    const bool ret = emitter->SendGVariant(signal_name, params);
    g_variant_unref(params);

    return ret;
}

} // namespace DBus::Signals
