//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file mainloop.hpp
 *
 * @brief  C++ wrapping for the glib2 GMainLoop interface
 */

#pragma once

#include <memory>

#include "exceptions.hpp"


namespace DBus {

/**
 *  This wraps the glib2 MainLoop API into a C++ API, mostly for simplicity
 *
 *  Using this main loop is a requirement for DBus::Service implementations
 *  to run properly.
 *
 *  This runs "on the side" of everything.  But somehow, there are some
 *  internal links under the glib2 hood - where it hooks up the needed
 *  calls to the D-Bus related functions and methods, which ends up
 *  in the D-Bus service handler (the DBus::Object passed to the
 *  DBus::Service::AssignServiceHandler() method).
 *
 *  NOTE: Only one MainLoop can run per process (pid).
 *
 */
class MainLoop
{
  public:
    using Ptr = std::shared_ptr<MainLoop>;

    class Exception : DBus::Exception
    {
      public:
        Exception(const std::string &errm);
    };

    /**
     *  Creates the DBus Mainloop object
     *
     *  @returns DBus::MainLoop::Ptr to the main loop object
     */
    [[nodiscard]] static MainLoop::Ptr Create()
    {
        return MainLoop::Ptr(new MainLoop());
    }

    ~MainLoop() noexcept = default;

    /**
     *  Start the main loop and let it run until the service wants to
     *  be shut down.  This main loop will respond to SIGINT and SIGTERM
     *  signals which will also exit the main loop.
     *
     *  The Run() method will block as long as the main loop is running
     *
     *  @throws DBus::MainLoop::Exception if a main loop is already running
     */
    void Run();


    /**
     *  Stops an already running main loop.  This can be called from a
     *  different process thread to stop the program
     */
    void Stop();


  private:
    MainLoop() = default;
};

}; // namespace DBus
