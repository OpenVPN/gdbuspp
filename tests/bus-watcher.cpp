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
#include "../gdbuspp/features/fmt-compat.hpp"
#include <fmt/printf.h>


#include "../gdbuspp/bus-watcher.hpp"
#include "../gdbuspp/mainloop.hpp"
#include "test-constants.hpp"


namespace Tests::Program {

int test_buswatch(int argc, char **argv)
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
                fmt::print(FMT_COMPILE("Bus name disappeared: {}\n"), bus_name);
                loop->Stop();
            });

        auto async_mainloop = std::async(std::launch::async,
                                         [&]
                                         {
                                             loop->Run();
                                         });

        fmt::print(FMT_COMPILE("Waiting for bus name {} to appear ...\n"), busname);

        if (!watcher.WaitFor(10s))
        {
            loop->Stop();
            async_mainloop.get();

            fmt::print(stderr, FMT_COMPILE("Timeout waiting for {} to appear!\n"), busname);
            return 1;
        }

        fmt::print(FMT_COMPILE("{} appeared!\n"), busname);

        loop->Stop();
        async_mainloop.get();
    }
    catch (const DBus::Exception &excp)
    {
        fmt::print(FMT_COMPILE("EXCEPTION (DBus): {}\n"), excp.what());
        return 9;
    }
    catch (const std::exception &excp)
    {
        fmt::print(FMT_COMPILE("EXCEPTION: {}\n"), excp.what());
        return 8;
    }
    return 0;
}
} // namespace Tests::Program


int main(int argc, char **argv)
{
    return Tests::Program::test_buswatch(argc, argv);
}