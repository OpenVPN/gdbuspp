//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   signal-emitgroup.cpp
 *
 * @brief  Test program using the DBus::Signals::Group interface
 *         sending D-Bus signals
 */

#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <getopt.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/glib2/utils.hpp"
#include "../gdbuspp/proxy.hpp"
#include "../gdbuspp/signals/group.hpp"
#include "../gdbuspp/signals/target.hpp"
#include "test-utils.hpp"
#include "test-constants.hpp"

using namespace DBus;
using namespace Test;

class Options : public TestUtils::OptionParser
{
  public:
    Options(int argc, char **argv)
    {
        static struct option long_opts[] = {
            // clang-format off
            {"system",             no_argument,       nullptr, 'Y'},
            {"session",            no_argument,       nullptr, 'E'},
            {"target",             required_argument, nullptr, 't'},
            {"object_path",        required_argument, nullptr, 'p'},
            {"interface",          required_argument, nullptr, 'i'},
            {"log-type",           required_argument, nullptr, 'l'},
            {"show-introspection", no_argument,       nullptr, 'Q'},
            {"quiet",              no_argument,       nullptr, 'q'},
            {"help",               no_argument,       nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
            // clang-format on
        };

        int opt;
        optind = 1;
        while ((opt = getopt_long(argc, argv, "YEt:p:i:l:Qqh", long_opts, nullptr)) != -1)
        {
            switch (opt)
            {
            case 'Y':
                bustype = DBus::BusType::SYSTEM;
                break;
            case 'E':
                bustype = DBus::BusType::SESSION;
                break;
            case 't':
                target.push_back(std::string(optarg));
                break;
            case 'p':
                object_path = std::string(optarg);
                break;
            case 'i':
                object_interface = std::string(optarg);
                break;
            case 'l':
                log_types.push_back(std::string(optarg));
                break;
            case 'Q':
                show_introspection = true;
                break;
            case 'q':
                quiet = true;
                break;
            case 'h':
                help(argv[0], long_opts);
                exit(0);
            }
        }

        if (target.size() == 0)
        {
            // If no targets has been added, do broadcast
            target.push_back("");
        }

        // default to test sending all signal types
        if (log_types.size() == 0)
        {
            log_types = {"info", "error", "debug", "invalid"};
        }
    }

    DBus::BusType bustype = DBus::BusType::SESSION;
    std::vector<std::string> target = {};
    std::string object_path = Constants::GenPath("signals");
    std::string object_interface = Constants::GenInterface("signals");
    std::vector<std::string> log_types;
    bool quiet = false;
    bool show_introspection = false;
};



class LogExample : public Signals::Group
{
  public:
    LogExample(DBus::Connection::Ptr conn,
               const std::string &path,
               const std::string &interface,
               const std::string &progname)
        : Group(conn, path, interface),
          program_name(progname)
    {
        RegisterSignal("Info",
                       {{"id", "i"},
                        {"message", "s"}});
        RegisterSignal("Error",
                       {{"code", "u"},
                        {"message", "s"},
                        {"object_name", "s"}});
        RegisterSignal("Debug",
                       {{"code", "t"},
                        {"message", "s"},
                        {"details", "s"},
                        {"object_name", "s"}});
    }

    void Info(const int id, const std::string &msg)
    {
        GVariant *p = g_variant_new("(is)", id, msg.c_str());
        SendGVariant("Info", p);
    }

    void Error(const unsigned int code, const std::string &msg)
    {
        GVariant *p = g_variant_new("(uss)", code, msg.c_str(), program_name.c_str());
        SendGVariant("Error", p);
    }

    void Debug(const unsigned int code,
               const std::string &msg,
               const std::string &details)
    {
        GVariant *p = g_variant_new("(tsss)", code, msg.c_str(), details.c_str(), program_name.c_str());
        SendGVariant("Debug", p);
    }

    void Invalid()
    {
        GVariant *p = g_variant_new("(s)", program_name.c_str());
        SendGVariant("Debug", p);
    }

  private:
    const std::string program_name;
};



int main(int argc, char **argv)
{

    std::ostringstream log;
    try
    {
        Options opts(argc, argv);

        auto dbuscon = DBus::Connection::Create(opts.bustype);
        auto log = Signals::Group::Create<LogExample>(dbuscon,
                                                      opts.object_path,
                                                      opts.object_interface,
                                                      "signal-group");

        if (opts.show_introspection)
        {
            std::cout << log->GenerateIntrospection();
            return 0;
        }

        for (const auto &tgt : opts.target)
        {
            log->AddTarget(tgt);
        }

        for (const auto &log_type : opts.log_types)
        {
            if ("info" == log_type)
            {
                log->Info(1, "Testing Info signal");
            }
            else if ("error" == log_type)
            {
                log->Error(2, "Error signal test");
            }
            else if ("debug" == log_type)
            {
                log->Debug(3, "A debug message", "With details here");
            }
            else if ("invalid" == log_type)
            {
                try
                {
                    log->Invalid();
                    throw DBus::Exception("signal-group-test",
                                          "log->Invalid() should fail; it didn't");
                }
                catch (const DBus::Signals::Exception &excp)
                {
                    std::string err(excp.what());
                    const std::string expect_error("Invalid data type for 'Debug' Expected '(tsss)' but received '(s)'");
                    if (err.find(expect_error) != std::string::npos)
                    {
                        if (!opts.quiet)
                        {
                            std::cout << "log->Invalid() test passed" << std::endl;
                        }
                    }
                    else
                    {
                        throw DBus::Exception("signal-group-test",
                                              "log->Invalid() threw unexpected exception:" + err);
                    }
                }
            }
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
