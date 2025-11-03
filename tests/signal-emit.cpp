//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   signal-emit.cpp
 *
 * @brief  Very simple and generic D-Bus signal sender tool
 */

#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <getopt.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/glib2/utils.hpp"
#include "../gdbuspp/signals/emit.hpp"
#include "../gdbuspp/signals/target.hpp"
#include "test-utils.hpp"
#include "test-constants.hpp"


namespace Tests::Program {

using namespace DBus;
using namespace Tests;

class EmitOpts : protected Tests::Utils::OptionParser
{
  public:
    EmitOpts(const int argc, char **argv)
    {
        static struct option long_opts[] = {
            // clang-format off
            {"system",        no_argument,       nullptr, 'Y'},
            {"session",       no_argument,       nullptr, 'E'},
            {"destination",   required_argument, nullptr, 'd'},
            {"object-path",   required_argument, nullptr, 'p'},
            {"interface",     required_argument, nullptr, 'i'},
            {"signal-name",   required_argument, nullptr, 's'},
            {"repeat-send",   required_argument, nullptr, 'r'},
            {"delay-send",    required_argument, nullptr, 'D'},
            {"data-type",     required_argument, nullptr, 't'},
            {"data-value",    required_argument, nullptr, 'v'},
            {"quiet",         no_argument,       nullptr, 'q'},
            {"help",          no_argument,       nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
            // clang-format on
        };

        int opt;
        optind = 1;
        std::string destination{};
        std::string object_path{Constants::GenPath("signals")};
        std::string object_interface{Constants::GenInterface("signals")};

        while ((opt = getopt_long(argc, argv, "YEd:p:i:r:D:s:t:v:qh", long_opts, nullptr)) != -1)
        {
            switch (opt)
            {
            case 'Y':
                bustype = DBus::BusType::SYSTEM;
                break;
            case 'E':
                bustype = DBus::BusType::SESSION;
                break;
            case 'd':
                destination = std::string(optarg);
                break;
            case 'p':
                object_path = std::string(optarg);
                break;
            case 'i':
                object_interface = std::string(optarg);
                break;
            case 'r':
                repeat_send = atoi(optarg);
                break;
            case 'D':
                delay_send = atoi(optarg);
                break;
            case 's':
                signal_name = std::string(optarg);
                break;
            case 't':
                data_type = std::string(optarg);
                break;
            case 'v':
                data_values.push_back(std::string(optarg));
                break;
            case 'q':
                quiet = true;
                break;
            case 'h':
                help(argv[0], long_opts);
                exit(0);
            }
        }
        target = Signals::Target::Create(destination, object_path, object_interface);
    };

    DBus::BusType bustype = DBus::BusType::SESSION;
    Signals::Target::Ptr target = nullptr;
    std::string signal_name{};
    uint32_t repeat_send = 1;
    uint32_t delay_send = 0;
    std::string data_type{};
    std::vector<std::string> data_values{};
    bool quiet = false;
};



int test_signal_emit(int argc, char **argv)
{
    EmitOpts options(argc, argv);
    std::ostringstream log;
    try
    {
        // Process all the --data-value values into a GVariant *
        // "package" to be used in the proxy call
        GVariant *data = nullptr;
        try
        {
            data = Tests::Utils::generate_gvariant(log,
                                                   options.data_type,
                                                   options.data_values,
                                                   true);
        }
        catch (const Tests::Utils::Exception &excp)
        {
            std::cerr << "** ERROR ** " << excp.what() << std::endl;
            return 2;
        }


        auto dbuscon = DBus::Connection::Create(options.bustype);
        auto sendsignal = Signals::Emit::Create(dbuscon);

        sendsignal->AddTarget(options.target);
        for (uint32_t i = 0; i < options.repeat_send; ++i)
        {
            if (options.delay_send > 0)
            {
                usleep(options.delay_send);
            }
            sendsignal->SendGVariant(options.signal_name, data);
        }
        g_variant_unref(data);

        if (!options.quiet)
        {
            std::cout << log.str();
        }

        return 0;
    }
    catch (const DBus::Exception &excp)
    {
        std::cout << log.str() << std::endl;
        std::cerr << "** EXCEPTION **  " << excp.what() << std::endl;
        return 2;
    }
}

} // namespace Tests::Program

int main(int argc, char **argv)
{
    return Tests::Program::test_signal_emit(argc, argv);
}