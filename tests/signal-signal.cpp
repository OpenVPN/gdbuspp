//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   signal-signal.cpp
 *
 * @brief  Very simple test program for a DBus::Signals::Signal
 *         implementation
 */

#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <getopt.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/glib2/utils.hpp"
#include "../gdbuspp/signals/emit.hpp"
#include "../gdbuspp/signals/signal.hpp"
#include "../gdbuspp/signals/target.hpp"
#include "test-utils.hpp"
#include "test-constants.hpp"

using namespace DBus;
using namespace Test;

class SignalOpts : protected TestUtils::OptionParser
{
  public:
    SignalOpts(const int argc, char **argv)
    {
        static struct option long_opts[] = {
            // clang-format off
            {"system",        no_argument,       nullptr, 'Y'},
            {"session",       no_argument,       nullptr, 'E'},
            {"destination",   required_argument, nullptr, 'd'},
            {"object-path",   required_argument, nullptr, 'p'},
            {"interface",     required_argument, nullptr, 'i'},
            {"repeat-send",   required_argument, nullptr, 'r'},
            {"delay-send",    required_argument, nullptr, 'D'},
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
    uint32_t repeat_send = 1;
    uint32_t delay_send = 0;
    bool quiet = false;
};


class TestSignal : public Signals::Signal
{
  public:
    TestSignal(Signals::Emit::Ptr emitter)
        : Signals::Signal(emitter, "TestSignal")
    {
        SetArguments({{"value_1", glib2::DataType::DBus<std::string>()},
                      {"value_2", glib2::DataType::DBus<bool>()},
                      {"value_3", glib2::DataType::DBus<uint32_t>()}});
    }

    bool Send(const std::string &val1, const bool val2, const uint32_t val3)
    {
        return EmitSignal(g_variant_new(GetDBusType(),
                                        val1.c_str(),
                                        val2,
                                        val3));
    }
};


int main(int argc, char **argv)
{
    SignalOpts opts(argc, argv);
    auto dbc = DBus::Connection::Create(opts.bustype);

    auto sig_emit = Signals::Emit::Create(dbc);
    sig_emit->AddTarget(opts.target);

    auto testsig = Signals::Signal::Create<TestSignal>(sig_emit);
    for (uint32_t i = 0; i < opts.repeat_send; ++i)
    {
        std::stringstream msg;
        msg << "Test Signal " << std::to_string(i + 1);
        testsig->Send(msg.str(), (i % 2), 101 + i);
    }
}
