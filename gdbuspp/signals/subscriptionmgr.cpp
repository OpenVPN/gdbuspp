//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 *  @file signals/subscriptionmgr.cpp
 *
 *  @brief Implementation of the DBus::Signal::SubscriptionManager
 *         This is used to subscribe to D-Bus signals which will trigger
 *         callback methods to be called when they occurs
 */

#include "subscriptionmgr.hpp"
#include "../glib2/callbacks.hpp"
#include "../glib2/strings.hpp"


namespace DBus {
namespace Signals {

void SubscriptionManager::Subscribe(Signals::Target::Ptr target,
                                    const std::string &signal_name,
                                    Signals::CallbackFnc callback)
{
    SingleSubscription::Ptr sub = SingleSubscription::Create(target, signal_name, callback);

    guint sigid = g_dbus_connection_signal_subscribe(connection->ConnPtr(),
                                                     str2gchar(target->busname),
                                                     str2gchar(target->object_interface),
                                                     str2gchar(signal_name),
                                                     str2gchar(target->object_path),
                                                     nullptr,
                                                     G_DBUS_SIGNAL_FLAGS_NONE,
                                                     glib2::Callbacks::_int_dbus_connection_signal_handler,
                                                     sub.get(),
                                                     nullptr /* destructor */);
    if (0 == sigid)
    {
        throw Signals::Exception(target,
                                 "Failed to subscribe to '" + signal_name + "'");
    }
    sub->SetSignalID(sigid);
    subscription_list.push_back(sub);
}


void SubscriptionManager::Unsubscribe(Signals::Target::Ptr target, const std::string &signal_name)
{
    auto it = std::find_if(subscription_list.begin(),
                           subscription_list.end(),
                           [&target, &signal_name](const SingleSubscription::Ptr sub)
                           {
                               return (signal_name == sub->signal_name) && (target == sub->target);
                           });
    if (subscription_list.end() == it)
    {
        throw Signals::Exception(target,
                                 "No subscription for '" + signal_name + "'");
    }
    g_dbus_connection_signal_unsubscribe(connection->ConnPtr(),
                                         it->get()->GetSignalID());
}



SubscriptionManager::SubscriptionManager(DBus::Connection::Ptr conn)
    : connection(conn)
{
}

} // namespace Signals
} // namespace DBus
