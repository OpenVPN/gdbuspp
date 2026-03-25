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

#include <algorithm>
#include <any>
#include <functional>
#include <limits>
#include <random>
#include <string>
#include <getopt.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <glib.h>
#include <unistd.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/proxy/utils.hpp"
#include "../gdbuspp/credentials/query.hpp"
#include "test-utils.hpp"
#include "test-constants.hpp"


namespace Tests::Program {

using namespace DBus;
using namespace Tests;

class ProxyOpts : protected Tests::Utils::OptionParser
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
            {"verbose",          no_argument,       nullptr, 'v'},
            {"help",             no_argument,       nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
            // clang-format on
        };

        int opt;
        optind = 1;
        while ((opt = getopt_long(argc, argv, "YEd:p:i:qvh", long_opts, nullptr)) != -1)
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
            case 'v':
                verbose = true;
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
    bool verbose = false;
};



/**
 *  Extract a list of well-known bus names which can be activated by
 *  the D-Bus.
 *
 *  This will exclude the 'org.freedesktop.DBus' service from the list,
 *  as that not return the expected return code when starting a service.
 *
 *  The resulting list will also be shuffled, to get some variation in
 *  the testing
 *
 * @param conn   DBus::Connection object to use for the lookup
 * @return std::vector<std::string> containing an unsorted list of all
 *         the D-Bus service names which can be activated
 */
std::vector<std::string> raw_ListActivatableNames(DBus::Connection::Ptr &conn)
{
    auto prx = DBus::Proxy::Client::Create(conn, "org.freedesktop.DBus");
    auto tgt = DBus::Proxy::TargetPreset::Create("/org/freedesktop/DBus",
                                                 "org.freedesktop.DBus");
    GVariant *r = prx->Call(tgt, "ListActivatableNames");
    if (r)
    {
        auto ret = glib2::Value::ExtractVector<std::string>(r);
        ret.erase(std::remove_if(ret.begin(),
                                 ret.end(),
                                 [](const std::string &v)
                                 {
                                     return v == "org.freedesktop.DBus";
                                 }));

        auto rd = std::random_device{};
        auto rng = std::default_random_engine{rd()};
        std::shuffle(ret.begin(), ret.end(), rng);
        return ret;
    }
    return {};
}


int test_proxy_utils(int argc, char **argv)
{
    ProxyOpts options(argc, argv);

    bool errors = false;
    if (options.destination.empty())
    {
        fmt::println(stderr, "** ERROR **  Missing --destination");
        errors = true;
    }

    if (options.object_path.empty())
    {
        fmt::println(stderr, "** ERROR **  Missing --object-path");
        errors = true;
    }

    if (options.object_interface.empty())
    {
        fmt::println(stderr, "** ERROR **  Missing --object-interface");
        errors = true;
    }

    if (errors)
    {
        return 2;
    };

    Tests::Utils::ResultLog testlogger(options.verbose);

    try
    {

        auto conn = DBus::Connection::Create(options.bustype);

        // Good proxy setup
        auto proxy = DBus::Proxy::Client::Create(conn, options.destination);
        auto query = DBus::Proxy::Utils::Query::Create(proxy);
        std::string path = options.object_path;
        std::string interface = options.object_interface;

        // Bad proxy setup
        DBus::Proxy::Client::Ptr bad_proxy = nullptr;
        DBus::Proxy::Utils::Query::Ptr bad_query = nullptr;

        testlogger.RunExceptionTest(
            "bad proxy: non.existing.service",
            bad_proxy,
            [&]()
            {
                bad_proxy = DBus::Proxy::Client::Create(conn, "non.existing.service", 3);
            },
            {"Service 'non.existing.service' cannot be reached"});

        testlogger.RunExceptionTest(
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
        testlogger.RunFunctionTest(
            fmt::format("query->CheckObjectExists('{}', '{}')", path, interface),
            proxy,
            [&query, &path, &interface]() -> bool
            {
                return query->CheckObjectExists(path, interface);
            },
            true);

        testlogger.RunFunctionTest(
            fmt::format("query->CheckObjectExists('{}', '{}')", incorrect_path, interface),
            proxy,
            [&query, &incorrect_path, &interface]() -> bool
            {
                return query->CheckObjectExists(incorrect_path, interface);
            },
            false);


        testlogger.RunFunctionTest(
            fmt::format("query->CheckObjectExists('{}', '{}')", path, incorrect_interface),
            proxy,
            [&query, &path, &incorrect_interface]() -> bool
            {
                return query->CheckObjectExists(path, incorrect_interface);
            },
            false);

        testlogger.RunFunctionTest(
            fmt::format("query->CheckObjectExists('{}', '{}')", incorrect_path, incorrect_interface),
            proxy,
            [&query, &incorrect_path, &incorrect_interface]() -> bool
            {
                return query->CheckObjectExists(incorrect_path, incorrect_interface);
            },
            false);

        // Run Utils::Query::ServiceVersion() tests
        testlogger.RunFunctionTest(
            fmt::format("query->ServiceVersion('{}', '{}')", path, interface),
            proxy,
            [&query, &path, &interface]() -> bool
            {
                return query->ServiceVersion(path, interface) == "0.1.2.3";
            },
            true);

        testlogger.RunExceptionTest(
            fmt::format("query->ServiceVersion('{}', '{}')", incorrect_path, incorrect_interface),
            proxy,
            [query, incorrect_path, incorrect_interface]()
            {
                query->ServiceVersion(incorrect_path, incorrect_interface);
            },
            {"Service is inaccessible"});

        // Run Utils::Query::Introspection() tetss
        {
            // Retrieve a copy of the introspection data directly.
            // This will be used to check the C++ Introspection() method output
            GVariant *res = proxy->Call(path,
                                        "org.freedesktop.DBus.Introspectable",
                                        "Introspect");
            auto chk_value = glib2::Value::Extract<std::string>(res, 0);
            g_variant_unref(res);

            testlogger.RunFunctionTest(
                fmt::format("query->Introspect('{}')", path),
                proxy,
                [&query, &path, &chk_value]() -> bool
                {
                    return query->Introspect(path) == chk_value;
                },
                true);

            testlogger.RunFunctionTest(
                fmt::format("query->Introspect('{}')", incorrect_path),
                proxy,
                [&query, &incorrect_path, &chk_value]() -> bool
                {
                    return query->Introspect(incorrect_path) == chk_value;
                },
                false);
        }


        // Run Proxy::Utils::DBusServiceQuery tests
        {
            auto srv_activatable = raw_ListActivatableNames(conn);
            auto service_qry = Proxy::Utils::DBusServiceQuery::Create(conn);
            std::string started_srv;

            testlogger.RunFunctionTest(
                "service_qry->StartServiceByName(...) == 2", nullptr, [&testlogger, &service_qry, &srv_activatable, &started_srv]() -> bool
                {
                    // Loop through activatable services until one starts
                    for (const auto &srv : srv_activatable)
                    {
                        bool r = service_qry->StartServiceByName(srv) == 2;
                        if (r)
                        {
                            testlogger.AddLogLine(fmt::format("            [lambda]      >>   Started service: {}", srv));
                            started_srv = srv;
                            return true;
                        }
                    }
                    return false;
                },
                true);

            testlogger.RunExceptionTest(
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

            testlogger.RunFunctionTest(
                fmt::format("service_qry->CheckServiceAvail({}) == true", started_srv),
                nullptr,
                [&service_qry, &started_srv]() -> bool
                {
                    return service_qry->CheckServiceAvail(started_srv, 5) == true;
                },
                true);


            testlogger.RunFunctionTest(
                "service_qry->CheckServiceAvail(non.existing.service) == true",
                nullptr,
                [&service_qry]() -> bool
                {
                    return service_qry->CheckServiceAvail("non.existing.service", 3) == true;
                },
                false);


            auto creds = DBus::Credentials::Query::Create(conn);
            std::string chk_busname = creds->GetUniqueBusName(options.destination);

            testlogger.RunFunctionTest(
                fmt::format("service_qry->GetNameOwner({})", options.destination),
                nullptr,
                [&service_qry, &options, &chk_busname]() -> bool
                {
                    return service_qry->GetNameOwner(options.destination) == chk_busname;
                },
                true);

            testlogger.RunExceptionTest(
                "service_qry->GetNameOwner(non.existing.service)",
                nullptr,
                [&service_qry]()
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

            testlogger.RunFunctionTest(
                "service_qry->LookupService(org.freedesktop.DBus)",
                nullptr,
                [&service_qry]() -> bool
                {
                    return service_qry->LookupService("org.freedesktop.DBus");
                },
                true);

            testlogger.RunFunctionTest(
                "service_qry->LookupService(non.existing.service)",
                nullptr,
                [&service_qry]() -> bool
                {
                    return service_qry->LookupService("non.existing.service");
                },
                false);

            testlogger.RunFunctionTest(
                "service_qry->LookupActivatable(org.freedesktop.DBus)",
                nullptr,
                [&service_qry]() -> bool
                {
                    return service_qry->LookupActivatable("org.freedesktop.DBus");
                },
                true);

            testlogger.RunFunctionTest(
                "service_qry->LookupActivatable(non.existing.service)",
                nullptr,
                [&service_qry]() -> bool
                {
                    return service_qry->LookupActivatable("non.existing.service");
                },
                false);
        }



        if (!options.quiet)
        {
            testlogger.DumpLogs();
        }
        bool test_result = testlogger.PrintTestReport();
        g_thread_pool_stop_unused_threads();
        return (test_result ? 0 : 2);
    }
    catch (const DBus::Exception &excp)
    {
        testlogger.DumpLogs();
        fmt::print(stderr, "** EXCEPTION **  {}\n"
                           "** EXCEPTION ** RawError: {}\n",
                   excp.what(),
                   excp.GetRawError());
        return 2;
    }
    return 9;
}

} // namespace Tests::Program


int main(int argc, char **argv)
{
    return Tests::Program::test_proxy_utils(argc, argv);
}