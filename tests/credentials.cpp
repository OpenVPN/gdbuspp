//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   credentials.cpp
 *
 * @brief  Test program using the DBus::Credentials interface
 *         sending D-Bus signals
 */

#include <string>
#include <getopt.h>
#include "../gdbuspp/features/fmt-compat.hpp"
#include <fmt/printf.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/credentials/query.hpp"
#include "test-utils.hpp"
#include "test-constants.hpp"


namespace Tests::Program {

using namespace DBus;
using namespace Tests;

class Options : public Tests::Utils::OptionParser
{
  public:
    DBus::BusType bustype = DBus::BusType::SESSION;
    std::string destination{};
    std::string expect_result{};
    bool get_ubusname = false;
    bool get_uid = false;
    bool get_pid = false;

    Options(const int argc, char **argv)
    {
        static struct option long_opts[] = {
            // clang-format off
            {"system",        no_argument,       nullptr, 'Y'},
            {"session",       no_argument,       nullptr, 'E'},
            {"destination",   required_argument, nullptr, 'd'},
            {"get-bus-name",  no_argument,       nullptr, 'b'},
            {"get-pid",       no_argument,       nullptr, 'p'},
            {"get-uid",       no_argument,       nullptr, 'u'},
            {"expect-result", required_argument, nullptr, 'x'},
            {"help",          no_argument,       nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
            // clang-format on
        };

        int opt;
        optind = 1;
        while ((opt = getopt_long(argc, argv, "YEd:bpux:h", long_opts, nullptr)) != -1)
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
            case 'b':
                get_ubusname = true;
                break;
            case 'p':
                get_pid = true;
                break;
            case 'u':
                get_uid = true;
                break;
            case 'x':
                expect_result = std::string(optarg);
                break;
            case 'h':
                help(argv[0], long_opts);
                exit(0);
            }
        }

        if (destination.empty())
        {
            fmt::print(stderr, "Missing destination to check (--destination)\n");
            exit(1);
        }

        if (!get_ubusname && !get_uid && !get_pid)
        {
            if (!expect_result.empty())
            {
                fmt::print("Using --expect-result requires one of --get-bus-name, --get-uid or --get-pid\n");
                exit(1);
            }

            // If no specific query is defined, query them all by default
            get_ubusname = true;
            get_uid = true;
            get_pid = true;
        }
    }
};


void check_expectations(const std::string &expect, const std::string &result)
{
    if (!expect.empty())
    {
        if (expect != result)
        {
            fmt::print(FMT_COMPILE("Unexpected result; expected '{}'\n"), expect);
            exit(3);
        }
        exit(0);
    }
}


int test_credential_query(int argc, char **argv)
{
    try
    {
        Options opts(argc, argv);

        auto conn = DBus::Connection::Create(opts.bustype);
        auto creds = DBus::Credentials::Query::Create(conn);

        if (opts.get_ubusname)
        {
            std::string result = creds->GetUniqueBusName(opts.destination);
            fmt::print(FMT_COMPILE("Unique bus name: '{}'\n"), result);
            check_expectations(opts.expect_result, result);
        }

        if (opts.get_uid)
        {
            uid_t result = creds->GetUID(opts.destination);
            fmt::print(FMT_COMPILE("Owning user ID (uid): {}\n"), result);
            check_expectations(opts.expect_result, std::to_string(result));
        }

        if (opts.get_pid)
        {
            pid_t result = creds->GetPID(opts.destination);
            fmt::print(FMT_COMPILE("Owning process ID (pid): {}\n"), result);
            check_expectations(opts.expect_result, std::to_string(result));
        }
    }
    catch (const DBus::Exception &excp)
    {
        fmt::print(stderr, FMT_COMPILE("** EXCEPTION **  {}\n"), excp.what());
        return 2;
    }


    return 0;
}
} // namespace Tests::Program


int main(int argc, char **argv)
{
    return Tests::Program::test_credential_query(argc, argv);
}
