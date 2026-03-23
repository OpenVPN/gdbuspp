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
#include <fmt/printf.h>


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
                fmt::println("Bus name disappeared: {}", bus_name);
                loop->Stop();
            });

        auto async_mainloop = std::async(std::launch::async,
                                         [&]
                                         {
                                             loop->Run();
                                         });

        fmt::println("Waiting for bus name {} to appear ...", busname);

        if (!watcher.WaitFor(10s))
        {
            loop->Stop();
            async_mainloop.get();

            fmt::println(stderr, "Timeout waiting for {} to appear!", busname);
            return 1;
        }

        fmt::println("{} appeared!", busname);

        async_mainloop.get();
    }
    catch (const DBus::Exception &excp)
    {
        fmt::println("EXCEPTION (DBus): {}", excp.what());
        return 9;
    }
    catch (const std::exception &excp)
    {
        fmt::println("EXCEPTION: {}", excp.what());
        return 8;
    }
    return 0;
}
} // namespace Tests::Program


int main(int argc, char **argv)
{
    return Tests::Program::buswatch_test(argc, argv);
}