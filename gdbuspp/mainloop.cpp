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
 *  This namespace is only used for holding library internal data.
 *
 *  The intention is to avoid hiding information useful while debugging,
 *  but at the same time keep it in a separate namespace indicating it
 *  is not to be directly exposed to the users of the library.
 */
namespace _private {

/**
 *  Internal mainloop object.  One process can only have one active
 *  mainloop
 */
GMainLoop *_int_glib2_mainloop = nullptr;
} // namespace _private


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
    _private::_int_glib2_mainloop = nullptr;
}


void MainLoop::Wait()
{
    if (_private::_int_glib2_mainloop)
    {
        g_main_loop_run(_private::_int_glib2_mainloop);
    }
}


bool MainLoop::Running() const
{
    return _private::_int_glib2_mainloop != nullptr;
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
