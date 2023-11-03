//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 *  @file signals/single-subscription.hpp
 *
 *  @brief Declares the class containing information about a single
 *         D-Bus signal subscription
 */

#pragma once

#include <memory>
#include <glib.h>

#include "event.hpp"
#include "exceptions.hpp"
#include "target.hpp"


namespace DBus {
namespace Signals {

/**
 *  D-Bus signal subscription information for a single signal subscription
 */
class SingleSubscription
{
  public:
    using Ptr = std::shared_ptr<SingleSubscription>;

    /**
     *  Create a new subscription object
     *
     * @param tgt      Target::Ptr with information about from where this
     *                 subscription should receive D-Bus signals from
     * @param signame  std::string with the signal name the subscription listens to
     * @param cbfnc    Signals::CallbackFnc being called when a signal arrives
     *
     * @return SingleSubscription::Ptr
     */
    static SingleSubscription::Ptr Create(Target::Ptr tgt,
                                          const std::string &signame,
                                          Signals::CallbackFnc cbfnc)
    {
        return SingleSubscription::Ptr(new SingleSubscription(tgt, signame, cbfnc));
    }


    /**
     *  Update the subscription details with the glib2 GDBus signal subscription
     *  identifier.  This is needed when unsubscribing from a signal later on.
     *
     *  This can only be set once and may not be modified later on.
     *
     *  This can not be provided as part of the constructor since this
     *  information is not available when this object is created, and this
     *  object is needed to request a signal subscription via glib2.
     *
     * @param sigid   guint with the signal subscription ID
     */
    void SetSignalID(const guint sigid);

    /**
     *  Retrieve the signal subscription identifier for this subscription
     *
     * @return const guint
     */
    const guint GetSignalID() const;


    /**
     *  Check if the provided subscription identifier matches the identifier
     *  of this object
     *
     * @param sigid   guint of the subscription ID to check
     *
     * @returns true on match, otherwise false
     */
    const bool CheckSignalID(const guint sigid) const;
    Target::Ptr target;
    const std::string signal_name;
    Signals::CallbackFnc callback;

  private:
    guint signal_id = 0;

    SingleSubscription(Target::Ptr tgt,
                       const std::string &signame,
                       Signals::CallbackFnc cbfnc);
};

} // namespace Signals
} // namespace DBus
