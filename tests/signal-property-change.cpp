//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C) 2026-  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C) 2026-  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   signal-subscribe.cpp
 *
 * @brief  A generic D-Bus signal monitor tool, using
 *         DBus::Signals::SubscriptionManager
 */

#include <limits>
#include <string>
#include <vector>
#include <getopt.h>
#include <fmt/printf.h>

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
            {"system",                  no_argument,       nullptr, 'Y'},
            {"session",                 no_argument,       nullptr, 'E'},
            {"sender",                  required_argument, nullptr, 'd'},
            {"object-path",             required_argument, nullptr, 'p'},
            {"interface",               required_argument, nullptr, 'i'},
            {"expect-property",         required_argument, nullptr, 'X'},
            {"expect-property-value",   required_argument, nullptr, 'x'},
            {"expect-count",            required_argument, nullptr, 'C'},
            {"quiet",                   no_argument,       nullptr, 'q'},
            {"verbose",                 no_argument,       nullptr, 'v'},
            {"help",                    no_argument,       nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
            // clang-format on
        };

        int opt;
        optind = 1;
        std::string destination{};
        std::string object_path{};
        std::string object_interface{};
        std::string chk_property_name;

        while ((opt = getopt_long(argc, argv, "YEd:p:i:X:x:C:qvh", long_opts, nullptr)) != -1)
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
            case 'X':
                if (!chk_property_name.empty())
                {
                    fmt::println(stderr,
                                 "{}: Cannot use -X again without haven passed -x first",
                                 argv[0]);
                    exit(1);
                }
                chk_property_name = std::string(optarg);
                break;
            case 'x':
                if (chk_property_name.empty())
                {
                    fmt::println(stderr,
                                 "{}: Cannot use -x before haven passed -X first",
                                 argv[0]);
                    exit(1);
                }
                check_properties[chk_property_name] = std::string(optarg);
                chk_property_name = {};
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

        if (!chk_property_name.empty())
        {
            fmt::println("{}: Missing -x argument after -X", argv[0]);
            exit(1);
        }

        if (verbose && quiet)
        {
            fmt::println("{}:  --verbose and --quiet cannot be combined", argv[0]);
            exit(1);
        }

        target = Signals::Target::Create(destination, object_path, "org.freedesktop.DBus.Properties");
    };

    DBus::BusType bustype = DBus::BusType::SESSION;
    Signals::Target::Ptr target = nullptr;
    std::map<std::string, std::string> check_properties{};
    uint64_t check_count = 0;
    bool quiet = false;
    bool verbose = false;
};


class ParsePropertiesChanged
{
  public:
    ParsePropertiesChanged(GVariant *params)
    {
        glib2::Utils::checkParams(__func__, params, "(sa{sv}as)");

        interface = glib2::Value::Extract<std::string>(params, 0);

        glib2::Dict::IterateDictionary(
            glib2::Value::ExtractChild(params, 1),
            [this](const std::string &property_name, GVariant *value)
            {
                this->changed[property_name] = Tests::Utils::parse_gvariant_to_string(value);
            });

        invalidated = glib2::Value::ExtractVector<std::string>(glib2::Value::ExtractChild(params, 2));
    }


    void Dump() const
    {
        fmt::print("\n **   PropertiesChanged:\n"
                   "\tinterface:  {}\n"
                   "\tproperties:\n",
                   interface);
        for (const auto &[name, value] : changed)
        {
            fmt::println("\t    - {}: {}", name, value);
        }
        fmt::println("\tinvalidated:");
        for (const auto &name : invalidated)
        {
            fmt::println("\t    - {}", name);
        }
    }

    bool Validate(const std::string &property, const std::string &expected)
    {
        try
        {
            std::string val = changed.at(property);
            return val == expected;
        }
        catch (const std::out_of_range &)
        {
            return false;
        }
    }


    std::string interface;
    std::map<std::string, std::string> changed;
    std::vector<std::string> invalidated;
};


// Just for lazy simplicity; keep these ones global
uint64_t signal_count = 1;
uint64_t signal_errors = 0;



void signal_handler(MainLoop::Ptr mainloop, const Options &opts, Signals::Event::Ptr &event)
{
    if (!opts.quiet)
    {
        fmt::println("{{{}}} {}", signal_count, event);
        if (opts.verbose)
        {
            std::ostringstream evdata;
            Tests::Utils::dump_gvariant(evdata, "        Signal Event", event->params);
            fmt::println("{}", evdata.str());
        }
    }

    ParsePropertiesChanged prop_chg(event->params);
    if (opts.verbose)
    {
        prop_chg.Dump();
        fmt::println("----------------");
    }

    // Check the content of the signal
    if (!opts.check_properties.empty())
    {
        bool error = false;

        for (const auto &[exp_prop, exp_value] : opts.check_properties)
        {
            error |= !prop_chg.Validate(exp_prop, exp_value);
            if (error)
            {
                ++signal_errors;
            }
        }

        if (opts.check_count > 0 && opts.check_count == signal_count)
        {
            fmt::println("Received the expected {} signals", signal_count);
            mainloop->Stop();
        }
    }
    ++signal_count;
}


int test_signal_property_change(int argc, char **argv)
{
    Options opts(argc, argv);
    try
    {
        auto dbuscon = DBus::Connection::Create(opts.bustype);
        auto sigmgr = Signals::SubscriptionManager::Create(dbuscon);

        auto mainloop = MainLoop::Create();
        sigmgr->Subscribe(opts.target,
                          "PropertiesChanged",
                          [&opts, mainloop](Signals::Event::Ptr &event)
                          {
                              signal_handler(mainloop, opts, event);
                          });

        if (!opts.quiet)
        {
            fmt::println("This process' unique D-Bus name: {}", dbuscon->GetUniqueBusName());
        }
        if (opts.verbose)
        {
            fmt::println("Watching for: {}", opts.target);
        }
        mainloop->Run();

        sigmgr->Unsubscribe(opts.target, "PropertiesChanged");

        if (signal_errors > 0)
        {
            fmt::println("Signal test result: FAIL.  {} errors occurred",
                         signal_errors);
            return 2;
        }
        return 0;
    }
    catch (const DBus::Exception &excp)
    {
        fmt::println(stderr, "** EXCEPTION **  {}", excp.what());
        return 2;
    }
}

} // namespace Tests::Program


int main(int argc, char **argv)
{
    return Tests::Program::test_signal_property_change(argc, argv);
}
