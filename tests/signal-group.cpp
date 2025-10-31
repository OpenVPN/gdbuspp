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
#include "../gdbuspp/signals/signal.hpp"
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
            log_types = {"info", "error", "debug", "singlestring", "invalid"};
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


class DebugSignal : public Signals::Signal
{
  public:
    /**
     *  Constructing an object to be used to send the "Debug" signal.
     *  This class does not define a D-Bus path or interface with the origin
     *  of this signal; that is handled via the Signal::Group object
     *
     * @param emitter   Signals::Emit::Ptr, provided automatically by
     *                  Signals::Group::CreateSignal<>()
     * @param prgnam    std::string with some debug details for the signal
     */
    DebugSignal(Signals::Emit::Ptr emitter, const std::string &prgnam)
        : Signals::Signal(emitter, "Debug"), program_name(prgnam)
    {
        SetArguments({
            {"code", glib2::DataType::DBus<uint64_t>()},
            {"message", glib2::DataType::DBus<std::string>()},
            {"details", glib2::DataType::DBus<std::string>()},
            {"program", glib2::DataType::DBus<std::string>()},
        });
    }

    void Send(const unsigned int code,
              const std::string &msg,
              const std::string &details)
    {
        GVariantBuilder *sigdata = glib2::Builder::Create("(tsss)");
        glib2::Builder::Add<uint64_t>(sigdata, code);
        glib2::Builder::Add(sigdata, msg);
        glib2::Builder::Add(sigdata, details);
        glib2::Builder::Add(sigdata, program_name);

        Signal::EmitSignal(glib2::Builder::Finish(sigdata));
    }

  private:
    const std::string program_name;
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
                       {
                           {"id", "i"},
                           {"message", "s"},
                       });
        RegisterSignal("Error",
                       {{"code", "u"},
                        {"message", "s"},
                        {"object_name", "s"}});
        RegisterSignal("SingleString",
                       {{"string", "s"}});
    }

    void Info(const int id, const std::string &msg)
    {
        GVariantBuilder *b = glib2::Builder::Create("(is)");
        glib2::Builder::Add(b, id);
        glib2::Builder::Add(b, msg);
        SendGVariant("Info", glib2::Builder::Finish(b));
    }

    void Error(const unsigned int code, const std::string &msg)
    {
        GVariantBuilder *b = glib2::Builder::Create("(uss)");
        glib2::Builder::Add(b, code);
        glib2::Builder::Add(b, msg);
        glib2::Builder::Add(b, program_name);
        SendGVariant("Error", glib2::Builder::Finish(b));
    }

    void SingleString(const std::string &msg)
    {
        SendGVariant("SingleString", glib2::Value::Create(msg));
    }

    void Invalid()
    {
        GVariant *p = glib2::Value::Create(program_name);
        try
        {
            // This is deliberately wrong - used for testing only
            // This shold fail via an exception which will be
            // processed in the main() function
            SendGVariant("Debug", p);
        }
        catch (...)
        {
            g_variant_unref(p);
            throw;
        }
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
        auto sig_log = Signals::Group::Create<LogExample>(dbuscon,
                                                          opts.object_path,
                                                          opts.object_interface,
                                                          "signal-group");
        auto debug = sig_log->CreateSignal<DebugSignal>("signal-group");

        if (opts.show_introspection)
        {
            std::cout << sig_log->GenerateIntrospection();
            return 0;
        }

        for (const auto &tgt : opts.target)
        {
            sig_log->AddTarget(tgt);
        }

        for (const auto &log_type : opts.log_types)
        {
            if ("info" == log_type)
            {
                sig_log->Info(1, "Testing Info signal");
            }
            else if ("error" == log_type)
            {
                sig_log->Error(2, "Error signal test");
            }
            else if ("singlestring" == log_type)
            {
                sig_log->SingleString("A simple single string");
            }
            else if ("debug" == log_type)
            {
                debug->Send(3, "A debug message", "With details here");
            }
            else if ("invalid" == log_type)
            {
                try
                {
                    sig_log->Invalid();
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
