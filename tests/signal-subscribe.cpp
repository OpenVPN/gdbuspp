//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   signal-subscribe.cpp
 *
 * @brief  A generic D-Bus signal monitor tool, using
 *         DBus::Signals::SubscriptionManager
 */

#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <getopt.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/glib2/utils.hpp"
#include "../gdbuspp/mainloop.hpp"
#include "../gdbuspp/signals/subscriptionmgr.hpp"
#include "../gdbuspp/signals/target.hpp"
#include "test-utils.hpp"


namespace Tests::Program {

using namespace DBus;


class Options : protected Tests::Utils::OptionParser
{
  public:
    Options(const int argc, char **argv)
    {
        static struct option long_opts[] = {
            // clang-format off
            {"system",        no_argument,       nullptr, 'Y'},
            {"session",       no_argument,       nullptr, 'E'},
            {"sender",        required_argument, nullptr, 'd'},
            {"object-path",   required_argument, nullptr, 'p'},
            {"interface",     required_argument, nullptr, 'i'},
            {"signal-name",   required_argument, nullptr, 's'},
            {"expect-type",   required_argument, nullptr, 'X'},
            {"expect-result", required_argument, nullptr, 'x'},
            {"expect-count",  required_argument, nullptr, 'C'},
            {"quiet",         no_argument,       nullptr, 'q'},
            {"verbose",       no_argument,       nullptr, 'v'},
            {"help",          no_argument,       nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
            // clang-format on
        };

        int opt;
        optind = 1;
        std::string destination{};
        std::string object_path{};
        std::string object_interface{};

        while ((opt = getopt_long(argc, argv, "YEd:p:i:s:X:x:C:qvh", long_opts, nullptr)) != -1)
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
            case 's':
                signal_name = std::string(optarg);
                break;
            case 'X':
                check_type = std::string(optarg);
                break;
            case 'x':
                check_value.push_back(std::string(optarg));
                break;
            case 'C':
                check_count = ::atol(optarg);
                break;
            case 'q':
                quiet = true;
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                help(argv[0], long_opts);
                exit(0);
            }
        }

        if (signal_name.empty() && !check_type.empty())
        {
            std::cerr << argv[0] << ":"
                      << " ** ERROR ** --expect-type requires --signal-name"
                      << std::endl;
            exit(1);
        }

        if (check_type.length() != check_value.size())
        {
            std::cerr << argv[0] << ":"
                      << " ** ERROR ** Too few or too many --expect-value "
                      << "arguments compared to type described by --expect-type"
                      << std::endl;
            exit(1);
        }

        if (verbose && quiet)
        {
            std::cerr << argv[0] << ":"
                      << " --verbose and --quiet cannot be combined"
                      << std::endl;
            exit(1);
        }

        target = Signals::Target::Create(destination, object_path, object_interface);
    };

    DBus::BusType bustype = DBus::BusType::SESSION;
    Signals::Target::Ptr target = nullptr;
    std::string signal_name{};
    std::string check_type{};
    std::vector<std::string> check_value{};
    uint64_t check_count = 0;
    bool quiet = false;
    bool verbose = false;
};


// Just for lazy simplicity; keep these ones global
uint64_t signal_count = 1;
uint64_t signal_errors = 0;

void signal_handler(MainLoop::Ptr mainloop, const Options &opts, Signals::Event::Ptr &event)
{
    if (!opts.quiet)
    {
        std::cout << "{" << std::to_string(signal_count) << "} " << event << std::endl;
        if (opts.verbose)
        {
            std::ostringstream evdata;
            Tests::Utils::dump_gvariant(evdata, "        Signal Event", event->params);
            std::cout << evdata.str();
        }
    }

    // Check the content of the signal
    if (!opts.check_type.empty() && opts.check_value.size() > 0)
    {
        bool error = false;
        std::string type(g_variant_get_type_string(event->params));
        if (type != "(" + opts.check_type + ")")
        {
            std::cerr << "        Received unexpected data type: '" << type << "'"
                      << ", expected '" << opts.check_type << "'" << std::endl;
            error = true;
        };


        uint32_t idx = 0;
        std::vector<std::string> values = Tests::Utils::convert_from_gvariant(event->params);
        for (const auto &v : values)
        {
            if (opts.verbose)
            {
                std::cout << "           type=" << type[idx + 1] << ", "
                          << "value='" << v << "'"
                          << std::endl;
            }
            if (idx < opts.check_value.size())
            {
                if (v != opts.check_value[idx])
                {
                    std::cout << "       Received unexpected data value: "
                              << "[" << std::to_string(idx) << "] - "
                              << "received '" << v << "'"
                              << ", expected '" << opts.check_value[idx] << "'"
                              << std::endl;
                    error = true;
                }
                ++idx;
            }
            else
            {
                std::cout << "       Received unexpected additional data value: "
                          << "[" << std::to_string(idx) << "] - "
                          << "received '" << v << "'"
                          << std::endl;
                error = true;
            }
        }
        if (error)
        {
            ++signal_errors;
        }

        if (opts.check_count > 0 && opts.check_count == signal_count)
        {
            std::cout << "Received the expected " << std::to_string(signal_count)
                      << " signals" << std::endl;
            mainloop->Stop();
        }
    }
    ++signal_count;
}


int test_signal_subscribe(int argc, char **argv)
{
    Options opts(argc, argv);
    std::ostringstream log;
    try
    {
        auto dbuscon = DBus::Connection::Create(opts.bustype);
        auto sigmgr = Signals::SubscriptionManager::Create(dbuscon);

        auto mainloop = MainLoop::Create();
        sigmgr->Subscribe(opts.target,
                          opts.signal_name,
                          [&opts, mainloop](Signals::Event::Ptr &event)
                          {
                              signal_handler(mainloop, opts, event);
                          });

        std::cout << "This process' unique D-Bus name:" << dbuscon->GetUniqueBusName() << std::endl;
        mainloop->Run();

        sigmgr->Unsubscribe(opts.target, opts.signal_name);

        if (signal_errors > 0)
        {
            std::cout << "Signal test result: FAIL.  "
                      << std::to_string(signal_errors) << " errors occurred"
                      << std::endl;
            return 2;
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
    return Tests::Program::test_signal_subscribe(argc, argv);
}
