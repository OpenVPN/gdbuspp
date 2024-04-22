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

#include <algorithm>

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
                                                     target->GetBusName(srvqry),
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
    subscription_list.erase(it);
}



SubscriptionManager::SubscriptionManager(DBus::Connection::Ptr conn)
    : connection(conn)
{
    srvqry = DBus::Proxy::Utils::DBusServiceQuery::Create(conn);
}


SubscriptionManager::~SubscriptionManager() noexcept
{
    // All subscribed signals need to be unsubscribed when this subscription
    // manager is deleted, otherwise the D-Bus service will continue to
    // trigger the callback method for these signals - and the callbacks
    // defined via this subscription manager will be deleted.
    for (const auto &sub : subscription_list)
    {
        g_dbus_connection_signal_unsubscribe(connection->ConnPtr(),
                                             sub->GetSignalID());
    }
    subscription_list.clear();
}

} // namespace Signals
} // namespace DBus
