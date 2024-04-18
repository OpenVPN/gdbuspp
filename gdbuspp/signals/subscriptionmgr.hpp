//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 *  @file signals/subscriptionmgr.hpp
 *
 *  @brief Declaration of the DBus::Signal::SubscriptionManager
 *         This is used to subscribe to D-Bus signals which will trigger
 *         callback methods to be called when they occurs
 */

#pragma once

#include <memory>
#include <glib.h>

#include "../connection.hpp"
#include "single-subscription.hpp"
#include "target.hpp"


namespace DBus {
namespace Signals {

class SubscriptionManager
{
  public:
    using Ptr = std::shared_ptr<SubscriptionManager>;

    /**
     *  Create a new subscription manager, which is bound to a specific
     *  D-Bus connection
     *
     * @param conn   DBus::Connection to bind subscriptions to
     * @return SubscriptionManager::Ptr
     */
    static SubscriptionManager::Ptr Create(DBus::Connection::Ptr conn)
    {
        return SubscriptionManager::Ptr(new SubscriptionManager(conn));
    }

    ~SubscriptionManager() noexcept;


    /**
     *  Add a subscription for a specific D-Bus signal name, with a
     *  match based on the description in the Signals::Target object
     *
     * @param target        Signals::Target::Ptr object with the match filter
     * @param signal_name   std::string with the signal name to subscribe to
     * @param callback      Signals::CallbackFnc being called when matching
     *                      signals occurs
     */
    void Subscribe(Signals::Target::Ptr target,
                   const std::string &signal_name,
                   Signals::CallbackFnc callback);

    /**
     *  Unsubscribe from a D-Bus signal subscription
     *
     * @param target      Signals::Target::Ptr object of the subscription
     * @param signal_name std::string with the signal name to unsubscribe from
     */
    void Unsubscribe(Signals::Target::Ptr target, const std::string &signal_name);

  private:
    DBus::Connection::Ptr connection;
    std::vector<SingleSubscription::Ptr> subscription_list;

    SubscriptionManager(DBus::Connection::Ptr conn);
};

} // namespace Signals
} // namespace DBus
