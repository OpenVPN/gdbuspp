//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   fd-send-fstat.cpp
 *
 * @brief  Simple test program sending a fd via a D-Bus method call
 *         which will run a fstat() on that fd and extract a few details
 */

#include <any>
#include <iostream>
#include <limits>
#include <string>
#include <getopt.h>
#include <glib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/proxy.hpp"
#include "test-utils.hpp"
#include "test-constants.hpp"

namespace Tests::Program {

using namespace Tests;


class Options : public Tests::Utils::OptionParser
{
  public:
    Options(int argc, char **argv)
    {
        static struct option long_opts[] = {
            // clang-format off
            {"system",        no_argument,       nullptr, 'Y'},
            {"session",       no_argument,       nullptr, 'E'},
            {"destination",   required_argument, nullptr, 'd'},
            {"object_path",   required_argument, nullptr, 'p'},
            {"interface",     required_argument, nullptr, 'i'},
            {"method",        required_argument, nullptr, 'm'},
            {"file",          required_argument, nullptr, 'f'},
            {"quiet",         no_argument,       nullptr, 'q'},
            {"help",          no_argument,       nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
            // clang-format on
        };

        int opt;
        optind = 1;
        while ((opt = getopt_long(argc, argv, "YEd:p:i:m:f:o:b:qh", long_opts, nullptr)) != -1)
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
            case 'm':
                method = std::string(optarg);
                break;
            case 'f':
                file = std::string(optarg);
                break;
            case 'q':
                quiet = true;
                break;
            case 'h':
                help(argv[0], long_opts);
                exit(0);
            }
        }

        preset = DBus::Proxy::TargetPreset::Create(object_path, object_interface);
    }

    DBus::BusType bustype = DBus::BusType::SESSION;
    std::string destination = Constants::GenServiceName("simple");
    std::string object_path = Constants::GenPath("simple1/methods");
    std::string object_interface = Constants::GenInterface("simple1");
    std::string method = {"fstatFileFromFD"};
    std::string file = {};
    bool quiet = false;
    DBus::Proxy::TargetPreset::Ptr preset = nullptr;
};


int test_fd_send_fstat(int argc, char **argv)
{
    try
    {
        Options opts(argc, argv);

        auto conn = DBus::Connection::Create(opts.bustype);
        auto prx = DBus::Proxy::Client::Create(conn, opts.destination);
        auto preset = DBus::Proxy::TargetPreset::Create(opts.object_path, opts.object_interface);

        int fd = open(opts.file.c_str(), O_RDONLY);
        GVariant *r = prx->SendFD(preset, opts.method, nullptr, fd);

        if (!opts.quiet)
        {
            std::ostringstream l;
            Tests::Utils::dump_gvariant(l, opts.method + " results", r);
            std::cout << l.str();
        }
        else
        {
            uid_t uid = glib2::Value::Extract<uint32_t>(r, 0);
            gid_t gid = glib2::Value::Extract<uint32_t>(r, 1);
            uint64_t size = glib2::Value::Extract<uint64_t>(r, 2);
            std::cout << "export testresult_uid=" << std::to_string(uid) << std::endl;
            std::cout << "export testresult_gid=" << std::to_string(gid) << std::endl;
            std::cout << "export testresult_size=" << std::to_string(size) << std::endl;
        }
        g_variant_unref(r);

        if (fd > 0)
        {
            close(fd);
        }
    }
    catch (const DBus::Exception &excp)
    {
        std::cerr << "** EXCEPTION **  " << excp.what() << std::endl;
        return 2;
    }
    return 0;
}

} // namespace Tests::Program

int main(int argc, char **argv)
{
    return Tests::Program::test_fd_send_fstat(argc, argv);
}