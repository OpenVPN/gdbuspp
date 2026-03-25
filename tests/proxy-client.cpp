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
#include <limits>
#include <string>
#include <getopt.h>
#include <glib.h>
#include <unistd.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/glib2/utils.hpp"
#include "../gdbuspp/proxy.hpp"
#include "../gdbuspp/proxy/utils.hpp"
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
            {"verbose",       no_argument,       nullptr, 'V'},
            {"quiet",         no_argument,       nullptr, 'q'},
            {"introspect",    no_argument,       nullptr, 'Q'},
            {"lookup",        no_argument,       nullptr, 'L'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
            // clang-format on
        };

        int opt;
        optind = 1;
        while ((opt = getopt_long(argc, argv, "YEd:p:i:Lm:g:s:S:I:U:B:t:v:X:x:VqQh", long_opts, nullptr)) != -1)
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
            case 'L':
                lookup_object = true;
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
            case 'V':
                verbose = true;
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

        if (introspect && lookup_object)
        {
            throw DBus::Exception("test_proxy", "Cannot use both --introspect and --lookup at the same time");
        }

        if (verbose && quiet)
        {
            throw DBus::Exception("test_proxy", "Cannot use both --verbose and --quiet at the same time");
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
    bool lookup_object = false;
    bool verbose = false;
    bool quiet = false;
};



int test_proxy_client(int argc, char **argv)
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
        fmt::println("** ERROR **  Missing --object-path");
        errors = true;
    }
    if (errors)
    {
        return 2;
    };


    Tests::Utils::ResultLog log(options.verbose);
    try
    {

        auto conn = DBus::Connection::Create(options.bustype);

        if (options.introspect)
        {
            auto prx = Proxy::Client::Create(conn, options.destination);
            auto prxqry = Proxy::Utils::Query::Create(prx);
            fmt::println("{}", prxqry->Introspect(options.object_path));
            return 0;
        }

        if (options.lookup_object)
        {
            auto prx = Proxy::Client::Create(conn, options.destination);
            auto prxqry = Proxy::Utils::Query::Create(prx);
            if (prxqry->LookupObject(options.object_path))
            {
                if (!options.quiet)
                {
                    fmt::println("Found object at {}", options.object_path);
                }
                return 0;
            }
            else
            {
                if (!options.quiet)
                {
                    fmt::println("Object {} not found", options.object_path);
                }
                return 2;
            }
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
            fmt::println(stderr, "** ERROR ** {}", excp.what());
            return 2;
        }


        auto prx = Proxy::Client::Create(conn, options.destination);
        log.AddLogLine(fmt::format("Connected to: {}", prx));

        // Check the Proxy::Client::GetDestination() method
        if (prx->GetDestination() != options.destination)
        {
            log.AddLogLine(fmt::format(
                "** ERROR ** Proxy::Client::GetDestination() did not return {}",
                options.destination));
            throw DBus::Exception("Proxy::Client::GetDestination()",
                                  fmt::format("Unexpected result: {}", prx->GetDestination()));
        }
        if (!options.method.empty())
        {
            if (options.object_interface.empty())
            {
                fmt::println(stderr, "** ERROR **   --interface is missing");
                return 2;
            }

            log.AddLogLine(fmt::format(
                "Method call: {}, method={}",
                options.preset,
                options.method));
            GVariant *res = prx->Call(options.preset, options.method, data);
            Tests::Utils::dump_gvariant(log, "GVariant response", res);

            if (!Tests::Utils::log_data_type_value_check(log,
                                                         options.check_type,
                                                         options.check_response,
                                                         res))
            {
                if (options.quiet)
                {
                    fmt::println("UNEXPECTED RESULT");
                    log.DumpLogs();
                    return 3;
                }
            }

            g_variant_unref(res);
        }
        else if (ProxyOpts::PropertyMode::GET == options.property_mode)
        {
            if (options.object_interface.empty())
            {
                fmt::println(stderr, "** ERROR **   --interface is missing");
                errors = true;
            }

            if (options.property.empty())
            {
                fmt::println(stderr, "** ERROR **  The property name cannot be empty");
                errors = true;
            }
            if (errors)
            {
                return 2;
            }


            // This does logic might seem silly, as the 'data' argument already contains all
            // the information.  But it uses the data type from the GVariant *data to call
            // call the appropriate GetProperty*<>() method.  This way, there is a check
            // that the GetProperty*<>() method extracts the expected data type.  This is
            // primarily a test for the GetProperty*<>() method.
            auto fetch_content = [](DBus::Proxy::Client::Ptr &prx, const Proxy::TargetPreset::Ptr &tgt, const std::string &property, GVariant *data) -> std::string
            {
                std::string data_type = glib2::DataType::Extract(data);
                std::string ret;
                uint32_t idx = 0;
                if (data_type[0] == 'a')
                {
                    switch (data_type[1])
                    {
                    case 'b':
                        ret = "<bool>: \n";
                        for (const auto &e : prx->GetPropertyArray<bool>(tgt, property))
                        {
                            ret.append(fmt::format("  [{}] {}\n", idx, static_cast<bool>(e)));
                            idx++;
                        }
                        break;

                    case 'i':
                        ret = "<int32_t>: \n";
                        for (const auto &e : prx->GetPropertyArray<int32_t>(tgt, property))
                        {
                            ret.append(fmt::format("  [{}] {}\n", idx, e));
                            idx++;
                        }
                        break;

                    case 'o':
                        ret = "<DBus::Object::Path>: \n";
                        for (const auto &e : prx->GetPropertyArray<DBus::Object::Path>(tgt, property))
                        {
                            ret.append(fmt::format("  [{}] {:?}\n", idx, e));
                            idx++;
                        }
                        break;

                    case 's':
                        ret = "<string>: \n";
                        for (const auto &e : prx->GetPropertyArray<std::string>(tgt, property))
                        {
                            ret.append(fmt::format("  [{}] {:?}\n", idx, e));
                            idx++;
                        }
                        break;

                    case 't':
                        ret = "<uint64_t>: \n";
                        for (const auto &e : prx->GetPropertyArray<uint64_t>(tgt, property))
                        {
                            ret.append(fmt::format("  [{}] {}\n", idx, e));
                            idx++;
                        }
                        break;

                    case 'u':
                        ret = "<uint32_t>: \n";
                        for (const auto &e : prx->GetPropertyArray<uint32_t>(tgt, property))
                        {
                            ret.append(fmt::format("  [{}] {}\n", idx, e));
                            idx++;
                        }
                        break;

                    case 'x':
                        ret = "<int64_t>: \n";
                        for (const auto &e : prx->GetPropertyArray<uint64_t>(tgt, property))
                        {
                            ret.append(fmt::format("  [{}] {}\n", idx, e));
                            idx++;
                        }
                        break;

                    default:
                        ret = "";
                        break;
                    }
                }
                else
                {
                    switch (data_type[0])
                    {
                    case 'b':
                        ret = fmt::format("<bool>:  {}", prx->GetProperty<bool>(tgt, property));
                        break;

                    case 'i':
                        ret = fmt::format("<int32_t>:  {}", prx->GetProperty<int32_t>(tgt, property));
                        break;

                    case 'o':
                        ret = fmt::format("<DBus::Object::Path>:  {:?}", prx->GetProperty<DBus::Object::Path>(tgt, property));
                        break;

                    case 's':
                        ret = fmt::format("<std::string>:  {:?}", prx->GetProperty<std::string>(tgt, property));
                        break;

                    case 't':
                        ret = fmt::format("<uint64_t>:  {}", prx->GetProperty<uint64_t>(tgt, property));
                        break;

                    case 'u':
                        ret = fmt::format("<uint32_t>:  {}", prx->GetProperty<uint32_t>(tgt, property));
                        break;

                    case 'x':
                        ret = fmt::format("<int64_t>:  {}", prx->GetProperty<int64_t>(tgt, property));
                        break;

                    default:
                        ret = "";
                        break;
                    }
                }
                return ret;
            };

            log.AddLogLine(fmt::format(
                "Get Property: {}, property={}",
                options.preset,
                options.property));

            GVariant *res = prx->GetPropertyGVariant(options.preset, options.property);
            Tests::Utils::dump_gvariant(log, "GVariant response", res);
            log.AddLogLine(fmt::format("Get Property {}", fetch_content(prx, options.preset, options.property, res)));

            if (!Tests::Utils::log_data_type_value_check(log,
                                                         options.check_type,
                                                         options.check_response,
                                                         res))
            {
                if (options.quiet)
                {
                    fmt::println(stderr, "UNEXPECTED RESULT");
                    log.DumpLogs();
                    return 3;
                }
            }
            g_variant_unref(res);
        }
        else if (ProxyOpts::PropertyMode::SET == options.property_mode
                 || ProxyOpts::PropertyMode::SET_ANY == options.property_mode)
        {
            if (options.object_interface.empty())
            {
                fmt::println(stderr, "** ERROR **   --interface is missing");
                errors = true;
            }
            if (options.property.empty())
            {
                fmt::println(stderr, "** ERROR **  The property name cannot be empty");
                errors = true;
            }
            if (ProxyOpts::PropertyMode::SET == options.property_mode && !data)
            {
                fmt::println(stderr, "** ERROR **  Missing new data value (--data-type, --data-value)");
                errors = true;
            }
            else if (ProxyOpts::PropertyMode::SET_ANY == options.property_mode && !options.prop_val.has_value())
            {
                fmt::println(stderr, "** ERROR **  Missing property value to --property-set-*");
            }
            if (errors)
            {
                return 2;
            }

            log.AddLogLine(fmt::format("Set Property: {}, property={}", options.preset, options.property));
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
            fmt::println(stderr, "No operation provided: --introspect, --method, --property-get, --property-set");
            return 1;
        }

        if (!options.quiet)
        {
            log.DumpLogs();
        }
        g_thread_pool_stop_unused_threads();
    }
    catch (const DBus::Exception &excp)
    {
        log.DumpLogs();
        fmt::println(stderr, "** EXCEPTION **  {}", excp.what());
        return 2;
    }
    return 0;
}

} // namespace Tests::Program


int main(int argc, char **argv)
{
    return Tests::Program::test_proxy_client(argc, argv);
}