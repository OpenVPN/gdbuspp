//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file service.cpp
 *
 * @brief  Implementation of DBus::Service.  This is a class to help build
 *         applications providing a service on the D-Bus.
 */

#include <iostream>
#include <string>
#include <glib.h>

#include "connection.hpp"
#include "glib2/callbacks.hpp"
#include "mainloop.hpp"
#include "service.hpp"


namespace DBus {

Service::Exception::Exception(const std::string &err, GError *gliberr)
    : DBus::Exception("DBus::Service", err, gliberr)
{
}


Connection::Ptr Service::GetConnection() const noexcept
{
    return buscon;
}


Object::Manager::Ptr Service::GetObjectManager() const noexcept
{
    return object_manager;
}


void Service::PrepareIdleDetector(const std::chrono::duration<uint32_t> timeout)
{
    if (std::chrono::duration<uint32_t>(0) == timeout)
    {
        // Timeout set to 0, which disables this feature
        return;
    }

    if (service_mainloop)
    {
        throw Service::Exception("Idle detection must be enabled before the main loop is created");
    }
    service_mainloop = MainLoop::Create();
    object_manager->PrepareIdleDetector(timeout, service_mainloop);
}


void Service::RunIdleDetector(const bool run)
{
    object_manager->RunIdleDetector(run);
}


void Service::Run()
{
    if (!service_mainloop)
    {
        // If not created via PrepareIdleDetection(), create it now
        service_mainloop = MainLoop::Create();
    }
    service_mainloop->Run();
}


void Service::Stop()
{
    if (!service_mainloop)
    {
        throw Service::Exception("No main loop started by this service object");
    }
    object_manager->RunIdleDetector(false);
    service_mainloop->Stop();
}



Service::Service(Connection::Ptr busc, const std::string &busname_)
    : buscon(busc), busname(busname_)
{
    service_register();
}


Service::~Service() noexcept
{
    try
    {
        if (object_manager)
        {
            object_manager->RunIdleDetector(false);
        }
        service_unregister();
    }
    catch (const DBus::Exception &err)
    {
        std::cerr << "** ERROR **   D-Bus service '" << busname << "': "
                  << std::string(err.what()) << std::endl;
    }
}


void Service::service_register()
{
    // Acquire the requested bus name
    busid = g_bus_own_name_on_connection(buscon->ConnPtr(),
                                         busname.c_str(),
                                         G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                         glib2::Callbacks::_int_callback_name_acquired,
                                         glib2::Callbacks::_int_callback_name_lost,
                                         this,
                                         nullptr);
    if (busid < 1)
    {
        throw Service::Exception("Could not own bus name for " + busname);
    }
    object_manager = Object::Manager::CreateManager(buscon);
}


void Service::service_unregister() noexcept
{
    if (busid > 0)
    {
        g_bus_unown_name(busid);
    }
}

} // namespace DBus
