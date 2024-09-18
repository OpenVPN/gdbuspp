//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Răzvan Cojocaru <razvan.cojocaru@openvpn.com>
//

/**
 * @file bus-watcher.cpp
 *
 * @brief  Utility for monitoring appearing / disappearing bus names
 */


#include "exceptions.hpp"
#include "bus-watcher.hpp"


namespace DBus {


BusWatcher::BusWatcher(BusType bus_type, const std::string &bus_name, bool start)
    : bus_type_(bus_type)
{
    watcher_id_ = g_bus_watch_name((bus_type_ == BusType::SESSION ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM),
                                   bus_name.c_str(),
                                   (start ? G_BUS_NAME_WATCHER_FLAGS_AUTO_START : G_BUS_NAME_WATCHER_FLAGS_NONE),
                                   on_name_appeared,
                                   on_name_disappeared,
                                   this,
                                   nullptr);
}


BusWatcher::~BusWatcher()
{
    g_bus_unwatch_name(watcher_id_);
}


BusWatcher::Exception::Exception(const std::string &errm)
    : DBus::Exception("DBus::BusWatcher", errm, nullptr)
{
}


void BusWatcher::on_name_appeared(GDBusConnection *connection,
                                  const gchar *name,
                                  const gchar *name_owner,
                                  gpointer user_data)
{
    BusWatcher *watcher = static_cast<BusWatcher *>(user_data);

    std::lock_guard lock{watcher->name_appeared_cv_mutex_};
    watcher->name_appeared_ = true;
    watcher->name_appeared_cv_.notify_all();
}


void BusWatcher::on_name_disappeared(GDBusConnection *connection,
                                     const gchar *name,
                                     gpointer user_data)
{
    BusWatcher *watcher = static_cast<BusWatcher *>(user_data);

    std::lock_guard lock{watcher->name_appeared_cv_mutex_};

    if (watcher->name_appeared_ && watcher->name_disappeared_callback_)
    {
        watcher->name_disappeared_callback_(name);
    }

    watcher->name_appeared_ = false;
}


} // namespace DBus
