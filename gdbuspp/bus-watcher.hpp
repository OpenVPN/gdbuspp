//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Răzvan Cojocaru <razvan.cojocaru@openvpn.com>
//

/**
 * @file bus-watcher.hpp
 *
 * @brief  Subscribes to notifications about bus names coming and going.
 */

#pragma once

#include <condition_variable>
#include <functional>
#include <gio/gio.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "connection.hpp"
#include "exceptions.hpp"


namespace DBus {

class Connection;

/**
 *  Watches the bus to see when a name appears, and once it has appeared, optionally
 *  notifies the user when it disappears.
 */
class BusWatcher
{
  public:
    using Ptr = std::shared_ptr<BusWatcher>;

    class Exception : public DBus::Exception
    {
      public:
        Exception(const std::string &errm);
    };

    /**
     *  Sets up a watch on a bus name.
     *
     * @param bus_type  Specify if we should watch the session or the system bus.
     * @param bus_name  The name of the bus we want to know about.
     * @param start     If this is `true`, and the bus name has not yet appeared on
     *                  the bus, registering for watching the bus will also try to
     *                  activate the service that owns the bus name.
     */
    BusWatcher(BusType bus_type, const std::string &bus_name, bool start = false);

    /**
     *  Unwatches the bus name.
     */
    ~BusWatcher();

    BusWatcher(const BusWatcher &) = delete;
    BusWatcher &operator=(const BusWatcher &) = delete;

    /**
     *  Wait for a specified period of time for the bus name to appear
     *
     * @param  timeout How long to wait for the bus name to appear.
     * @return `true` if the function exited because the bus name appeared,
     *         `false` if it exited because a timeout occured while waiting.
     */
    template <typename Rep, typename Period>
    bool WaitFor(const std::chrono::duration<Rep, Period> &timeout)
    {
        std::unique_lock<std::mutex> lock{name_appeared_cv_mutex_};

        return name_appeared_cv_.wait_for(lock, timeout, [this]
                                          {
                                              return name_appeared_;
                                          });
    }

    /**
     *  Set an (optional) callback to be invoked when the bus name disappears
     *
     * @param  disappeared_handler The callback to be invoked when the bus name
     *                             disappears.
     */
    template <typename Callable>
    void SetNameDisappearedHandler(Callable &&disappeared_handler)
    {
        std::lock_guard lock{name_appeared_cv_mutex_};
        name_disappeared_callback_ = std::forward<Callable>(disappeared_handler);
    }

  private:
    static void on_name_appeared(GDBusConnection *connection,
                                 const gchar *name,
                                 const gchar *name_owner,
                                 gpointer user_data);

    static void on_name_disappeared(GDBusConnection *connection,
                                    const gchar *name,
                                    gpointer user_data);

  private:
    guint watcher_id_{0};
    std::function<void(const std::string &)> name_disappeared_callback_;
    std::condition_variable name_appeared_cv_;
    std::mutex name_appeared_cv_mutex_;
    bool name_appeared_{false};
};

}; // namespace DBus
