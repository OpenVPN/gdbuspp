//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   proxy-utils.cpp
 *
 * @brief  Simple tests for the DBus::Proxy::Utils functionality
 *
 *         This test requires the simple-service.cpp test service to be
 *         running
 */

#include <any>
#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include <getopt.h>
#include <glib.h>
#include <unistd.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/proxy/utils.hpp"
#include "../gdbuspp/credentials/query.hpp"
#include "test-utils.hpp"
#include "test-constants.hpp"

using namespace DBus;
using namespace Test;

class ProxyOpts : protected TestUtils::OptionParser
{
  public:
    ProxyOpts(const int argc, char **argv)
    {
        static struct option long_opts[] = {
            // clang-format off
            {"system",           no_argument,       nullptr, 'Y'},
            {"session",          no_argument,       nullptr, 'E'},
            {"destination",      required_argument, nullptr, 'd'},
            {"object-path",      required_argument, nullptr, 'p'},
            {"object-interface", required_argument, nullptr, 'i'},
            {"quiet",            no_argument,       nullptr, 'q'},
            {"help",             no_argument,       nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
            // clang-format on
        };

        int opt;
        optind = 1;
        while ((opt = getopt_long(argc, argv, "YEd:p:i:qh", long_opts, nullptr)) != -1)
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
            case 'q':
                quiet = true;
                break;

            case 'h':
                help(argv[0], long_opts);
                exit(0);
            }
        }
        preset = Proxy::TargetPreset::Create(object_path, object_interface);
    };

    DBus::BusType bustype = DBus::BusType::SESSION;
    Proxy::TargetPreset::Ptr preset = nullptr;
    std::string destination = Constants::GenServiceName("simple");
    std::string object_path = Constants::GenPath("simple1");
    std::string object_interface = Constants::GenInterface("simple1");
    bool quiet = false;
};


uint32_t success = 0;
uint32_t failed = 0;

void test_log(std::ostringstream &log,
              const std::string &descr,
              DBus::Proxy::Client::Ptr proxy,
              bool result,
              bool expect)
{
    log << "[" << (result == expect ? "PASS" : "FAIL") << "] "
        << "{result=" << (result ? "pass" : "fail")
        << ", expected=" << (expect ? "pass" : "fail")
        << "} " << descr;
    if (proxy)
    {
        log << "  -- Proxy: " << proxy;
    }
    log << std::endl;

    if (result == expect)
    {
        ++success;
    }
    else
    {
        ++failed;
    }
}

template <typename FUNC>
void test_exception(std::ostringstream &log,
                    const std::string &descr,
                    DBus::Proxy::Client::Ptr proxy,
                    FUNC &&testfunc,
                    const std::vector<std::string> &exception_list)
{
    bool result = false;
    std::string error_msg{};
    try
    {
        testfunc();
        result = false;
    }
    catch (const DBus::Exception &excp)
    {
        for (const auto &exception : exception_list)
        {
            error_msg = std::string(excp.GetRawError());
            result = (error_msg == exception);
            if (result)
            {
                break;
            }
        }
    }
    std::ostringstream msg;
    msg << "Expecting exception string["
        << std::quoted(exception_list[0]) << "] " + descr;
    test_log(log, msg.str(), proxy, result, true);
    if (!result)
    {
        if (!error_msg.empty())
        {
            log << "       Received exception message: " << error_msg << std::endl;
        }
        else
        {
            log << "       EMPTY EXCEPTION MESSAGE" << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    std::ostringstream log;
    try
    {
        ProxyOpts options(argc, argv);

        bool errors = false;
        if (options.destination.empty())
        {
            std::cerr << "** ERROR **  Missing --destination" << std::endl;
            errors = true;
        }

        if (options.object_path.empty())
        {
            std::cerr << "** ERROR **  Missing --object-path" << std::endl;
            errors = true;
        }

        if (options.object_interface.empty())
        {
            std::cerr << "** ERROR **  Missing --object-interface" << std::endl;
            errors = true;
        }

        if (errors)
        {
            return 2;
        };

        auto conn = DBus::Connection::Create(options.bustype);

        // Good proxy setup
        auto proxy = DBus::Proxy::Client::Create(conn, options.destination);
        auto query = DBus::Proxy::Utils::Query::Create(proxy);
        std::string path = options.object_path;
        std::string interface = options.object_interface;

        // Bad proxy setup
        DBus::Proxy::Client::Ptr bad_proxy = nullptr;
        DBus::Proxy::Utils::Query::Ptr bad_query = nullptr;

        test_exception(log,
                       "bad proxy: non.existing.service",
                       bad_proxy,
                       [&]()
                       {
                           bad_proxy = DBus::Proxy::Client::Create(conn, "non.existing.service");
                       },
                       {// dbus-daemon error string
                        "Failed querying service 'non.existing.service': "
                        "Could not get owner of name 'non.existing.service': no such name",

                        // dbus-broker error string
                        "Failed querying service 'non.existing.service': "
                        "The name does not have an owner"});

        test_exception(log,
                       "bad query: non.existing.service",
                       bad_proxy,
                       [&]()
                       {
                           bad_query = DBus::Proxy::Utils::Query::Create(bad_proxy);
                       },
                       {"Invalid DBus::Proxy::Client object"});

        std::string incorrect_path = "/nonexisting/path";
        std::string incorrect_interface = "no.such.interface";

        // Run Utils::Query::CheckObjectExists() tests
        test_log(log,
                 "query->CheckObjectExists('" + path + "', '" + interface + "')",
                 proxy,
                 query->CheckObjectExists(path, interface),
                 true);
        test_log(log,
                 "query->CheckObjectExists('" + incorrect_path + "', '" + interface + "')",
                 proxy,
                 query->CheckObjectExists(incorrect_path, interface),
                 false);
        test_log(log,
                 "query->CheckObjectExists('" + path + "', '" + incorrect_interface + "')",
                 proxy,
                 query->CheckObjectExists(path, incorrect_interface),
                 false);
        test_log(log,
                 "query->CheckObjectExists('" + incorrect_path + "', '" + incorrect_interface + "')",
                 proxy,
                 query->CheckObjectExists(incorrect_path, incorrect_interface),
                 false);

        // Run Utils::Query::ServiceVersion() tests
        test_log(log,
                 "query->ServiceVersion('" + path + "', '" + interface + "')",
                 proxy,
                 query->ServiceVersion(path, interface) == "0.1.2.3",
                 true);

        test_exception(
            log,
            "query->ServiceVersion('" + incorrect_path + "', '" + incorrect_interface + "')",
            proxy,
            [query, incorrect_path, incorrect_interface]()
            {
                query->ServiceVersion(incorrect_path, incorrect_interface);
            },
            {"Object path/interface does not exists"});

        // Run Utils::Query::Introspection() tetss
        {
            // Retrieve a copy of the introspection data directly.
            // This will be used to check the C++ Introspection() method output
            GVariant *res = proxy->Call(path,
                                        "org.freedesktop.DBus.Introspectable",
                                        "Introspect");
            auto chk_value = glib2::Value::Extract<std::string>(res, 0);
            g_variant_unref(res);

            test_log(log,
                     "query->Introspect('" + path + "')",
                     proxy,
                     query->Introspect(path) == chk_value,
                     true);
            test_log(log,
                     "query->Introspect('" + incorrect_path + "')",
                     proxy,
                     query->Introspect(incorrect_path) == chk_value,
                     false);
        }


        // Run Proxy::Utils::DBusServiceQuery tests
        {
            auto service_qry = Proxy::Utils::DBusServiceQuery::Create(conn);

            test_log(log,
                     "service_qry->StartServiceByName(org.freedesktop.systemd1) == 2",
                     nullptr,
                     service_qry->StartServiceByName("org.freedesktop.systemd1") == 2,
                     true);

            test_exception(
                log,
                "service_qry->StartServiceByName(non.existing.service)",
                nullptr,
                [service_qry]()
                {
                    service_qry->StartServiceByName("non.existing.service");
                },
                {// dbus-daemon error string
                 "Failed querying service 'non.existing.service': "
                 "The name non.existing.service was not provided by any "
                 ".service files",

                 // dbus-broker error string
                 "Failed querying service 'non.existing.service': "
                 "The name is not activatable"});

            test_log(log,
                     "service_qry->CheckServiceAvail(org.freedesktop.systemd1) == true",
                     nullptr,
                     service_qry->CheckServiceAvail("org.freedesktop.systemd1") == true,
                     true);
            test_log(log,
                     "service_qry->CheckServiceAvail(non.existing.service) == true",
                     nullptr,
                     service_qry->CheckServiceAvail("non.existing.service") == true,
                     false);

            auto creds = DBus::Credentials::Query::Create(conn);
            std::string chk_busname = creds->GetUniqueBusName(options.destination);
            test_log(log,
                     "service_qry->GetNameOwner(" + options.destination + ")",
                     nullptr,
                     service_qry->GetNameOwner(options.destination) == chk_busname,
                     true);
            test_exception(
                log,
                "service_qry->GetNameOwner(non.existing.service)",
                nullptr,
                [service_qry]()
                {
                    service_qry->GetNameOwner("non.existing.service");
                },
                {// dbus-daemon error string
                 "Failed querying service 'non.existing.service': "
                 "Could not get owner of name 'non.existing.service': "
                 "no such name",

                 // dbus-broker error string
                 "Failed querying service 'non.existing.service': "
                 "The name does not have an owner"});

            test_log(log,
                     "service_qry->LookupService(org.freedesktop.DBus)",
                     nullptr,
                     service_qry->LookupService("org.freedesktop.DBus"),
                     true);
            test_log(log,
                     "service_qry->LookupService(non.existing.service)",
                     nullptr,
                     service_qry->LookupService("non.existing.service"),
                     false);

            test_log(log,
                     "service_qry->LookupActivatable(org.freedesktop.DBus)",
                     nullptr,
                     service_qry->LookupActivatable("org.freedesktop.DBus"),
                     true);
            test_log(log,
                     "service_qry->LookupActivatable(non.existing.service)",
                     nullptr,
                     service_qry->LookupActivatable("non.existing.service"),
                     false);
        }



        if (!options.quiet)
        {
            std::cout << log.str();
        }
        std::cout << ">> Passed tests: " << std::to_string(success) << std::endl
                  << ">> Failed tests: " << std::to_string(failed) << std::endl
                  << ">> OVERALL TEST RESULT: "
                  << (success > 0 && failed == 0 ? "PASS" : "FAIL")
                  << std::endl;
        g_thread_pool_stop_unused_threads();
        return (success > 0 && failed == 0) ? 0 : 2;
    }
    catch (const DBus::Exception &excp)
    {
        std::cout << log.str() << std::endl;
        std::cerr << "** EXCEPTION **  " << excp.what() << std::endl;
        std::cerr << "** EXCEPTION **  RawError: '" << excp.GetRawError() << "'" << std::endl;
        return 2;
    }
    return 9;
}
