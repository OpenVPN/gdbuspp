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
#include <iostream>
#include <iomanip>
#include <limits>
#include <random>
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

bool test_log(std::ostringstream &log,
              const std::string &descr,
              DBus::Proxy::Client::Ptr proxy,
              std::function<bool()> &&test_func,
              bool expect)
{
    time_t start, end;
    time(&start);
    bool result = test_func();
    time(&end);

    log << "[" << (result == expect ? "PASS" : "FAIL") << "] "
        << "{result=" << (result ? "pass" : "fail")
        << ", expected=" << (expect ? "pass" : "fail")
        << ", execution_time=" << double(end - start) << "s"
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
    return result;
}

template <typename FUNC>
void test_exception(std::ostringstream &log,
                    const std::string &descr,
                    DBus::Proxy::Client::Ptr proxy,
                    FUNC &&testfunc,
                    const std::vector<std::string> &exception_list)
{
    std::ostringstream msg;
    msg << "Expecting exception string["
        << std::quoted(exception_list[0]) << "] " + descr;

    std::string error_msg{};
    auto test_code = [&testfunc, &exception_list, &error_msg]() -> bool
    {
        bool res = false;
        try
        {
            testfunc();
            res = false;
        }
        catch (const DBus::Exception &excp)
        {
            for (const auto &exception : exception_list)
            {
                error_msg = std::string(excp.GetRawError());
                res = (error_msg == exception);
                if (res)
                {
                    break;
                }
            }
        }
        return res;
    };

    bool result = test_log(log, msg.str(), proxy, test_code, true);
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
                           bad_proxy = DBus::Proxy::Client::Create(conn, "non.existing.service", 3);
                       },
                       {"Service 'non.existing.service' cannot be reached"});

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
        auto test1 = [&query, &path, &interface]() -> bool
        {
            return query->CheckObjectExists(path, interface);
        };
        test_log(log,
                 "query->CheckObjectExists('" + path + "', '" + interface + "')",
                 proxy,
                 test1,
                 true);

        auto test2 = [&query, &incorrect_path, &interface]() -> bool
        {
            return query->CheckObjectExists(incorrect_path, interface);
        };
        test_log(log,
                 "query->CheckObjectExists('" + incorrect_path + "', '" + interface + "')",
                 proxy,
                 test2,
                 false);

        auto test3 = [&query, &path, &incorrect_interface]() -> bool
        {
            return query->CheckObjectExists(path, incorrect_interface);
        };
        test_log(log,
                 "query->CheckObjectExists('" + path + "', '" + incorrect_interface + "')",
                 proxy,
                 test3,
                 false);

        auto test4 = [&query, &incorrect_path, &incorrect_interface]() -> bool
        {
            return query->CheckObjectExists(incorrect_path, incorrect_interface);
        };
        test_log(log,
                 "query->CheckObjectExists('" + incorrect_path + "', '" + incorrect_interface + "')",
                 proxy,
                 test4,
                 false);

        // Run Utils::Query::ServiceVersion() tests
        auto test5 = [&query, &path, &interface]() -> bool
        {
            return query->ServiceVersion(path, interface) == "0.1.2.3";
        };
        test_log(log,
                 "query->ServiceVersion('" + path + "', '" + interface + "')",
                 proxy,
                 test5,
                 true);

        test_exception(
            log,
            "query->ServiceVersion('" + incorrect_path + "', '" + incorrect_interface + "')",
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

            auto test_introsp = [&query, &path, &chk_value]() -> bool
            {
                return query->Introspect(path) == chk_value;
            };
            test_log(log,
                     "query->Introspect('" + path + "')",
                     proxy,
                     test_introsp,
                     true);

            auto test_incorr_introsp = [&query, &incorrect_path, &chk_value]() -> bool
            {
                return query->Introspect(incorrect_path) == chk_value;
            };
            test_log(log,
                     "query->Introspect('" + incorrect_path + "')",
                     proxy,
                     test_incorr_introsp,
                     false);
        }


        // Run Proxy::Utils::DBusServiceQuery tests
        {
            auto srv_activatable = raw_ListActivatableNames(conn);
            auto service_qry = Proxy::Utils::DBusServiceQuery::Create(conn);
            std::string started_srv;
            auto test_start_srv_by_name = [&service_qry, &srv_activatable, &started_srv, &log]() -> bool
            {
                // Loop through activatable services until one starts
                for (const auto &srv : srv_activatable)
                {
                    bool r = service_qry->StartServiceByName(srv) == 2;
                    if (r)
                    {
                        log << "            [lambda]      >>   Started service:"
                            << srv << std::endl;
                        started_srv = srv;
                        return true;
                    }
                }
                return false;
            };

            test_log(log,
                     "service_qry->StartServiceByName(...) == 2",
                     nullptr,
                     test_start_srv_by_name,
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

            auto test_check_srv_avail = [&service_qry, &started_srv]() -> bool
            {
                return service_qry->CheckServiceAvail(started_srv, 5) == true;
            };
            test_log(log,
                     "service_qry->CheckServiceAvail(" + started_srv + ") == true",
                     nullptr,
                     test_check_srv_avail,
                     true);

            auto test_check_srv_avail_non = [&service_qry]() -> bool
            {
                return service_qry->CheckServiceAvail("non.existing.service", 3) == true;
            };

            test_log(log,
                     "service_qry->CheckServiceAvail(non.existing.service) == true",
                     nullptr,
                     test_check_srv_avail_non,
                     false);

            auto creds = DBus::Credentials::Query::Create(conn);
            std::string chk_busname = creds->GetUniqueBusName(options.destination);

            auto test_getname_own_opt = [&service_qry, &options, &chk_busname]() -> bool
            {
                return service_qry->GetNameOwner(options.destination) == chk_busname;
            };
            test_log(log,
                     "service_qry->GetNameOwner(" + options.destination + ")",
                     nullptr,
                     test_getname_own_opt,
                     true);

            test_exception(
                log,
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

            auto test_lookup_srv_exist = [&service_qry]() -> bool
            {
                return service_qry->LookupService("org.freedesktop.DBus");
            };
            test_log(log,
                     "service_qry->LookupService(org.freedesktop.DBus)",
                     nullptr,
                     test_lookup_srv_exist,
                     true);

            auto test_lookup_srv_non = [&service_qry]() -> bool
            {
                return service_qry->LookupService("non.existing.service");
            };
            test_log(log,
                     "service_qry->LookupService(non.existing.service)",
                     nullptr,
                     test_lookup_srv_non,
                     false);

            auto test_lookupact_avail = [&service_qry]() -> bool
            {
                return service_qry->LookupActivatable("org.freedesktop.DBus");
            };
            test_log(log,
                     "service_qry->LookupActivatable(org.freedesktop.DBus)",
                     nullptr,
                     test_lookupact_avail,
                     true);

            auto test_lookupact_non = [&service_qry]() -> bool
            {
                return service_qry->LookupActivatable("non.existing.service");
            };
            test_log(log,
                     "service_qry->LookupActivatable(non.existing.service)",
                     nullptr,
                     test_lookupact_non,
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
