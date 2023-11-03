//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file mainloop.cpp
 *
 * @brief  Implementation of the C++ wrapping for the glib2 GMainLoop interface
 */


#include <glib.h>
#include <glib-unix.h>

#include "exceptions.hpp"
#include "mainloop.hpp"
#include "glib2/callbacks.hpp"


namespace DBus {

/**
 *  This is purely a hack to avoid exposing an internal glib2 related variable
 *  into the mainloop.hpp, which would require including a glib2 header files
 *  into mainloop.hpp - or expose it as a void * member in the DBus::MainLoop
 *  class.
 *
 *  Since this is intended to only be used
 */
namespace _private {
GMainLoop *_int_glib2_mainloop = nullptr;
}

MainLoop::Exception::Exception(const std::string &errm)
    : DBus::Exception("DBus::MainLoop", errm, nullptr)
{
}


void MainLoop::Run()
{
    if (_private::_int_glib2_mainloop)
    {
        throw MainLoop::Exception("A main loop is already running");
    }
    _private::_int_glib2_mainloop = g_main_loop_new(nullptr, false);
    g_unix_signal_add(SIGINT, glib2::Callbacks::_int_mainloop_stop_handler, _private::_int_glib2_mainloop);
    g_unix_signal_add(SIGTERM, glib2::Callbacks::_int_mainloop_stop_handler, _private::_int_glib2_mainloop);
    g_main_loop_run(_private::_int_glib2_mainloop);
    g_main_loop_unref(_private::_int_glib2_mainloop);
}


void MainLoop::Stop()
{
    if (!_private::_int_glib2_mainloop)
    {
        throw MainLoop::Exception("No main loop is running");
    }

    g_main_loop_quit(_private::_int_glib2_mainloop);
}


} // namespace DBus
