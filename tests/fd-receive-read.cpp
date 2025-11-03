//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   fd-receive-read.cpp
 *
 * @brief  Simple test program receiving a fd via a D-Bus method call
 *         and reading data from that fd
 */

#include <any>
#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <getopt.h>
#include <glib.h>
#include <unistd.h>

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
            {"output",        required_argument, nullptr, 'o'},
            {"buffer-size",   required_argument, nullptr, 'b'},
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
            case 'o':
                output = std::string(optarg);
                break;
            case 'b':
                bufsize = ::atol(optarg);
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
    std::string method = {"OpenFilePassFD"};
    std::string file = {};
    std::string output = {};
    ssize_t bufsize = 64 * 1024;
    bool quiet = false;
    DBus::Proxy::TargetPreset::Ptr preset = nullptr;
};



int test_fd_receive_read(int argc, char **argv)
{
    try
    {
        Options opts(argc, argv);

        auto conn = DBus::Connection::Create(opts.bustype);
        auto prx = DBus::Proxy::Client::Create(conn, opts.destination);

        int fd = -1;
        GVariantBuilder *param = glib2::Builder::Create("(s)");
        glib2::Builder::Add(param, opts.file);
        GVariant *r = prx->GetFD(fd,
                                 opts.preset,
                                 opts.method,
                                 glib2::Builder::Finish(param));

        bool res = glib2::Value::Extract<bool>(r, 0) && (fd > 0);
        g_variant_unref(r);
        if (!opts.quiet)
        {
            std::cout << "Success: " << (res ? "true" : "false") << std::endl;
            std::cout << "Buffer size: " << std::to_string(opts.bufsize) << std::endl;
        }
        if (!res)
        {
            return 2;
        }

        char buf[opts.bufsize + 1];
        memset(&buf, 0, opts.bufsize + 1);

        std::ofstream outstr;
        if (!opts.output.empty())
        {
            outstr = std::ofstream(opts.output, std::ofstream::binary);
        }

        ssize_t s = -1;
        do
        {
            s = ::read(fd, &buf, opts.bufsize);
            if (s < 0)
            {
                throw DBus::Exception("main()",
                                      "Failed reading from file descriptor:"
                                          + std::string(strerror(errno)));
            }
            if (!opts.output.empty())
            {
                if (!opts.quiet)
                {
                    std::cout << "Read " << std::to_string(s) << " bytes" << std::endl;
                }
                outstr.write(buf, s);
            }
            else
            {
                std::cout.write(buf, s);
            }
            memset(&buf, 0, opts.bufsize + 1);
        } while (s > 0);
        close(fd);
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
    return Tests::Program::test_fd_receive_read(argc, argv);
}