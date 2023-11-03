//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 *  @file signals/single-subscription.cpp
 *
 *  @brief Implements the class containing information about a single
 *         D-Bus signal subscription
 */

#include <glib.h>

#include "exceptions.hpp"
#include "single-subscription.hpp"


namespace DBus {
namespace Signals {

void SingleSubscription::SetSignalID(const guint sigid)
{
    if (signal_id != 0)
    {
        throw Signals::Exception(target, "Signal ID already set");
    }
    signal_id = sigid;
}


const guint SingleSubscription::GetSignalID() const
{
    return signal_id;
}


const bool SingleSubscription::CheckSignalID(const guint sigid) const
{
    return sigid == signal_id;
}


SingleSubscription::SingleSubscription(Target::Ptr tgt,
                                       const std::string &signame,
                                       Signals::CallbackFnc cbfnc)
    : target(tgt), signal_name(signame), callback(cbfnc)
{
}

} // namespace Signals
} // namespace DBus
