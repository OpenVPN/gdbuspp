//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file exceptions.hpp
 *
 * @brief Definition of the generic DBus::Exception interface
 */


#pragma once

#include <exception>
#include <string>
#include <glib.h>

namespace DBus {

class Exception : public std::exception
{
  public:
    /**
     *  Base DBus exception class
     *
     * @param classn   std::string with class/group name of the event
     * @param err      std::string with error to provide to the end-user
     * @param gliberr  GError object with more details; optional value
     */
    Exception(const std::string &classn,
              const std::string &err,
              GError *gliberr = nullptr);

    virtual ~Exception() = default;

    /**
     *  Provides the full detailed error message, which includes
     *  C++ class information.  Other exception classes inheriting this
     *  class may add even more details.
     * @returns char *
     */
    virtual const char *what() const noexcept;

    /**
     *  Provides only the error message, stripping more debug information.
     *  This is more useful
     *
     * @return const char*
     */
    virtual const char *GetRawError() const noexcept;


#ifdef __G_IO_H__
    /**
     *  Wrapper for more easily returning a DBusException exception
     *  back to an on going D-Bus method call.  This will transport the
     *  error back to the D-Bus caller.
     *
     *  If the GDBUSPP_DEBUG macro is enabled at build time, more
     *  debug information will be provided to the D-Bus caller
     *
     * @param invocation Pointer to a invocation object of the on-going
     *                   method call
     */
    virtual void SetDBusError(GDBusMethodInvocation *invocation,
                              const std::string &errdomain) const noexcept;


    /**
     *  Wrapper for more easily returning a DBusException back
     *  to an on going D-Bus set/get property call.  This will transport
     *  the error back to the D-Bus caller
     *
     * @param dbuserror  Pointer to a GError pointer where the error will
     *                   be saved
     * @param domain     Error Quark domain used to classify the
     *                   authentication failure
     * @param code       A GIO error code, used for further error
     *                   error classification.  Look up G_IO_ERROR_*
     *                   entries in glib-2.0/gio/gioenums.h for details.
     */
    virtual void SetDBusError(GError **dbuserror,
                              GQuark domain,
                              gint code) const noexcept;

#endif

  private:
    std::string classerr{}; ///< Full error message with class/group details
    std::string error{};    ///< Only the error message for the end-user
};

}; // namespace DBus
