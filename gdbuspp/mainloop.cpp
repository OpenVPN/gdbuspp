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
 *  This anonymous namespace is only used for
 *  holding library internal data.
 */
namespace {

/**
 *  Internal mainloop object.  One process can only have one active
 *  mainloop
 */
GMainLoop *_int_glib2_mainloop = nullptr;
} // namespace


MainLoop::Exception::Exception(const std::string &errm)
    : DBus::Exception("DBus::MainLoop", errm, nullptr)
{
}


void MainLoop::Run()
{
    if (_int_glib2_mainloop)
    {
        throw MainLoop::Exception("A main loop is already running");
    }
    _int_glib2_mainloop = g_main_loop_new(nullptr, false);
    g_unix_signal_add(SIGINT, glib2::Callbacks::_int_mainloop_stop_handler, _int_glib2_mainloop);
    g_unix_signal_add(SIGTERM, glib2::Callbacks::_int_mainloop_stop_handler, _int_glib2_mainloop);
    g_main_loop_run(_int_glib2_mainloop);
    g_main_loop_unref(_int_glib2_mainloop);
    _int_glib2_mainloop = nullptr;
}


void MainLoop::Wait()
{
    if (_int_glib2_mainloop)
    {
        g_main_loop_run(_int_glib2_mainloop);
    }
}


bool MainLoop::Running() const
{
    return _int_glib2_mainloop != nullptr;
}


void MainLoop::Stop()
{
    if (!_int_glib2_mainloop)
    {
        throw MainLoop::Exception("No main loop is running");
    }

    g_main_loop_quit(_int_glib2_mainloop);
}


} // namespace DBus
