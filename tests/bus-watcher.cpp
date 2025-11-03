//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Răzvan Cojocaru <razvan.cojocaru@openvpn.com>
//

/**
 * @file   bus-watcher.cpp
 *
 * @brief  Trivial D-Bus bus watcher example.
 *
 */

#include <future>
#include <iostream>

#include "../gdbuspp/bus-watcher.hpp"
#include "../gdbuspp/mainloop.hpp"
#include "test-constants.hpp"


namespace Tests::Program {

int buswatch_test(int argc, char **argv)
{
    using namespace std::literals;

    try
    {
        auto loop = DBus::MainLoop::Create();

        const char busname[] = "net.openvpn.gdbuspp.test.simple";
        DBus::BusWatcher watcher(DBus::BusType::SESSION, busname);

        watcher.SetNameDisappearedHandler(
            [&](const std::string &bus_name)
            {
                std::cout << "Bus name disappeared: " << bus_name << std::endl;
                loop->Stop();
            });

        auto async_mainloop = std::async(std::launch::async,
                                         [&]
                                         {
                                             loop->Run();
                                         });

        std::cout << "Waiting for bus name " << busname
                  << " to appear ..." << std::endl;

        if (!watcher.WaitFor(10s))
        {
            loop->Stop();
            async_mainloop.get();

            std::cerr << "Timeout waiting for " << busname << " to appear!"
                      << std::endl;
            return 1;
        }

        std::cout << busname << " appeared!" << std::endl;

        async_mainloop.get();
    }
    catch (const DBus::Exception &excp)
    {
        std::cout << "EXCEPTION (DBus): " << excp.what() << std::endl;
        return 9;
    }
    catch (const std::exception &excp)
    {
        std::cout << "EXCEPTION: " << excp.what() << std::endl;
        return 8;
    }
    return 0;
}
} // namespace Tests::Program


int main(int argc, char **argv)
{
    return Tests::Program::buswatch_test(argc, argv);
}