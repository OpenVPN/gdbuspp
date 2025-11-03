//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   proxy-client.cpp
 *
 * @brief  Very simple and generic D-Bus proxy client using the
 *         GDBus++ API  (DBus::Proxy::Client)
 */

#include <any>
#include <iostream>
#include <limits>
#include <string>
#include <getopt.h>
#include <glib.h>
#include <unistd.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/glib2/utils.hpp"
#include "../gdbuspp/proxy.hpp"
#include "test-utils.hpp"


namespace Tests::Program {

using namespace DBus;

class ProxyOpts : protected Tests::Utils::OptionParser
{
  public:
    enum class PropertyMode
    {
        UNSET,
        GET,
        SET,
        SET_ANY
    };


    ProxyOpts(const int argc, char **argv)
    {
        static struct option long_opts[] = {
            // clang-format off
            {"system",        no_argument,       nullptr, 'Y'},
            {"session",       no_argument,       nullptr, 'E'},
            {"destination",   required_argument, nullptr, 'd'},
            {"object_path",   required_argument, nullptr, 'p'},
            {"interface",     required_argument, nullptr, 'i'},
            {"method-call",   required_argument, nullptr, 'm'},
            {"property-get",  required_argument, nullptr, 'g'},
            {"property-set",  required_argument, nullptr, 's'},
            {"property-set-string", required_argument, nullptr, 'S'},
            {"property-set-int",    required_argument, nullptr, 'I'},
            {"property-set-uint",   required_argument, nullptr, 'U'},
            {"property-set-bool",   required_argument, nullptr, 'B'},
            {"data-type",     required_argument, nullptr, 't'},
            {"data-value",    required_argument, nullptr, 'v'},
            {"expect-type",   required_argument, nullptr, 'X'},
            {"expect-result", required_argument, nullptr, 'x'},
            {"quiet",         no_argument,       nullptr, 'q'},
            {"introspect",    no_argument,       nullptr, 'Q'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
            // clang-format on
        };

        int opt;
        optind = 1;
        while ((opt = getopt_long(argc, argv, "YEd:p:i:m:g:s:S:I:U:B:t:v:X:x:qQh", long_opts, nullptr)) != -1)
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
                object_path = DBus::Object::Path(optarg);
                break;
            case 'i':
                object_interface = std::string(optarg);
                break;
            case 'Q':
                introspect = true;
                break;
            case 'm':
                method = std::string(optarg);
                break;
            case 'g':
                property = std::string(optarg);
                property_mode = PropertyMode::GET;
                break;
            case 's':
                property = std::string(optarg);
                property_mode = PropertyMode::SET;
                break;
            case 'S':
                property = std::string(optarg);
                property_mode = PropertyMode::SET_ANY;
                break;
            case 'I':
            case 'U':
                property = std::string(optarg);
                property_mode = PropertyMode::SET_ANY;
                break;
            case 'B':
                property = std::string(optarg);
                property_mode = PropertyMode::SET_ANY;
                break;
            case 't':
                data_type = std::string(optarg);
                break;
            case 'v':
                data_values.push_back(std::string(optarg));
                break;
            case 'X':
                check_type = std::string(optarg);
                break;
            case 'x':
                check_response = std::string(optarg);
                break;
            case 'q':
                quiet = true;
                break;
            case 'h':
                help(argv[0], long_opts);
                exit(0);
            }

            if (PropertyMode::SET_ANY == property_mode && (optind < argc))
            {
                switch (opt)
                {
                case 'S':
                    prop_val = std::string(argv[optind++]);
                    break;
                case 'I':
                case 'U':
                    prop_val = std::atoi(argv[optind++]);
                    break;
                case 'B':
                    {
                        std::string boolstr(argv[optind++]);
                        prop_val = ("1" == boolstr || "yes" == boolstr || "true" == boolstr);
                        property_mode = PropertyMode::SET_ANY;
                    }
                }
            }
        }
        preset = Proxy::TargetPreset::Create(object_path, object_interface);
    };

    DBus::BusType bustype = DBus::BusType::SESSION;
    Proxy::TargetPreset::Ptr preset = nullptr;
    std::string destination{};
    DBus::Object::Path object_path;
    std::string object_interface{};
    std::string method{};
    std::string property{};
    std::any prop_val{};
    std::string data_type{};
    std::string check_type{};
    std::string check_response{};
    std::vector<std::string> data_values{};
    PropertyMode property_mode = PropertyMode::UNSET;
    bool introspect = false;
    bool quiet = false;
};



int test_proxy_client(int argc, char **argv)
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
        if (errors)
        {
            return 2;
        };


        auto conn = DBus::Connection::Create(options.bustype);

        if (options.introspect)
        {
            auto prx = Proxy::Client::Create(conn, options.destination);
            GVariant *res = prx->Call(options.object_path,
                                      "org.freedesktop.DBus.Introspectable",
                                      "Introspect");
            std::cout << glib2::Value::Extract<std::string>(res, 0);
            return 0;
        }

        // Process all the --data-value values into a GVariant *
        // "package" to be used in the proxy call
        GVariant *data = nullptr;
        try
        {
            data = Tests::Utils::generate_gvariant(log,
                                                   options.data_type,
                                                   options.data_values,
                                                   ProxyOpts::PropertyMode::SET != options.property_mode);
        }
        catch (const Tests::Utils::Exception &excp)
        {
            std::cerr << "** ERROR ** " << excp.what() << std::endl;
            return 2;
        }


        auto prx = Proxy::Client::Create(conn, options.destination);
        log << "Connected to: " << prx << std::endl;

        // Check the Proxy::Client::GetDestination() method
        if (prx->GetDestination() != options.destination)
        {
            log << "** ERROR ** Proxy::Client::GetDestination() "
                << "did not return " << options.destination << std::endl;
            throw DBus::Exception("Proxy::Client::GetDestination()",
                                  "Unexpected result:" + prx->GetDestination());
        }
        if (!options.method.empty())
        {
            if (options.object_interface.empty())
            {
                std::cerr << "** ERROR **   --interface is missing" << std::endl;
                return 2;
            }

            log << "Method call: " << options.preset
                << ", method=" << options.method << std::endl;
            GVariant *res = prx->Call(options.preset, options.method, data);
            Tests::Utils::dump_gvariant(log, "GVariant response", res);

            std::ostringstream check_log;
            if (!Tests::Utils::log_data_type_value_check(check_log,
                                                         options.check_type,
                                                         options.check_response,
                                                         res))
            {
                if (options.quiet)
                {
                    std::cout << "UNEXPECTED RESULT:" << std::endl
                              << check_log.str();
                    return 3;
                }
            }
            log << check_log.str();

            g_variant_unref(res);
        }
        else if (ProxyOpts::PropertyMode::GET == options.property_mode)
        {
            if (options.object_interface.empty())
            {
                std::cerr << "** ERROR **   --interface is missing" << std::endl;
                errors = true;
            }

            if (options.property.empty())
            {
                std::cerr << "** ERROR **  The property name cannot be empty" << std::endl;
                errors = true;
            }
            if (errors)
            {
                return 2;
            }

            log << "Get Property: " << options.preset
                << ", property=" << options.property << std::endl;

            GVariant *res = prx->GetPropertyGVariant(options.preset, options.property);
            Tests::Utils::dump_gvariant(log, "GVariant response", res);


            std::string res_type = std::string(g_variant_get_type_string(res));
            if ("s" == res_type || "o" == res_type || "as" == res_type || "ao" == res_type)
            {
                log << "Get Property <string>: ";
                if ('a' == res_type[0])
                {
                    log << std::endl;
                    unsigned int i = 0;
                    for (const auto &e : prx->GetPropertyArray<std::string>(options.preset, options.property))
                    {
                        log << "  [" << std::to_string(i) << "] "
                            << e << std::endl;
                        ++i;
                    }
                }
                else
                {
                    log << prx->GetProperty<std::string>(options.preset, options.property)
                        << std::endl;
                }
            }
            else if ("i" == res_type || "ai" == res_type)
            {
                log << "Get Property <int>: ";
                if ('a' == res_type[0])
                {
                    log << std::endl;
                    unsigned int i = 0;
                    for (const auto &e : prx->GetPropertyArray<int>(options.preset, options.property))
                    {
                        log << "  [" << std::to_string(i) << "] "
                            << std::to_string(e) << std::endl;
                        ++i;
                    }
                }
                else
                {
                    log << prx->GetProperty<int>(options.preset, options.property)
                        << std::endl;
                }
            }
            else if ("u" == res_type || "au" == res_type)
            {
                log << "Get Property <unsigned int>: ";
                if ('a' == res_type[0])
                {
                    log << std::endl;
                    unsigned int i = 0;
                    for (const auto &e : prx->GetPropertyArray<unsigned int>(options.preset, options.property))
                    {
                        log << "  [" << std::to_string(i) << "] "
                            << std::to_string(e) << std::endl;
                        ++i;
                    }
                }
                else
                {
                    log << prx->GetProperty<unsigned int>(options.preset, options.property)
                        << std::endl;
                }
            }
            else if ("t" == res_type || "at" == res_type)
            {
                log << "Get Property <uint64_t>: ";
                if ('a' == res_type[0])
                {
                    log << std::endl;
                    unsigned int i = 0;
                    for (const auto &e : prx->GetPropertyArray<uint64_t>(options.preset, options.property))
                    {
                        log << "  [" << std::to_string(i) << "] "
                            << std::to_string(e) << std::endl;
                        ++i;
                    }
                }
                else
                {
                    log << prx->GetProperty<uint64_t>(options.preset, options.property)
                        << std::endl;
                }
            }



            std::ostringstream check_log;
            if (!Tests::Utils::log_data_type_value_check(check_log,
                                                         options.check_type,
                                                         options.check_response,
                                                         res))
            {
                if (options.quiet)
                {
                    std::cout << "UNEXPECTED RESULT:" << std::endl
                              << check_log.str();
                    return 3;
                }
            }
            log << check_log.str();
            g_variant_unref(res);
        }
        else if (ProxyOpts::PropertyMode::SET == options.property_mode
                 || ProxyOpts::PropertyMode::SET_ANY == options.property_mode)
        {
            if (options.object_interface.empty())
            {
                std::cerr << "** ERROR **   --interface is missing" << std::endl;
                errors = true;
            }
            if (options.property.empty())
            {
                std::cerr << "** ERROR **  The property name cannot be empty" << std::endl;
                errors = true;
            }
            if (ProxyOpts::PropertyMode::SET == options.property_mode && !data)
            {
                std::cerr << "** ERROR **  Missing new data value (--data-type, --data-value)" << std::endl;
                errors = true;
            }
            else if (ProxyOpts::PropertyMode::SET_ANY == options.property_mode && !options.prop_val.has_value())
            {
                std::cerr << "** ERROR **  Missing property value to --property-set-*" << std::endl;
            }
            if (errors)
            {
                return 2;
            }

            log << "Set Property: " << options.preset
                << ", property=" << options.property << std::endl;
            if (ProxyOpts::PropertyMode::SET == options.property_mode)
            {
                prx->SetPropertyGVariant(options.preset, options.property, data);
            }
            else
            {
                if (typeid(int) == options.prop_val.type())
                {
                    prx->SetProperty(options.preset, options.property, std::any_cast<int &>(options.prop_val));
                }
                else if (typeid(unsigned int) == options.prop_val.type())
                {
                    prx->SetProperty(options.preset, options.property, std::any_cast<unsigned int &>(options.prop_val));
                }
                else if (typeid(bool) == options.prop_val.type())
                {
                    prx->SetProperty(options.preset, options.property, std::any_cast<bool &>(options.prop_val));
                }
                else if (typeid(std::string) == options.prop_val.type())
                {
                    prx->SetProperty(options.preset, options.property, std::any_cast<std::string &>(options.prop_val));
                }
            }
        }
        else
        {
            std::cerr << "No operation provided: --introspect, --method, "
                      << "--property-get, --property-set"
                      << std::endl;
            return 1;
        }

        if (!options.quiet)
        {
            std::cout << log.str();
        }
        g_thread_pool_stop_unused_threads();
    }
    catch (const DBus::Exception &excp)
    {
        std::cout << log.str() << std::endl;
        std::cerr << "** EXCEPTION **  " << excp.what() << std::endl;
        return 2;
    }
    return 0;
}

} // namespace Tests::Program


int main(int argc, char **argv)
{
    return Tests::Program::test_proxy_client(argc, argv);
}