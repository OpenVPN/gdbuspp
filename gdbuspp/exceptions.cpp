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
 * @brief Misc implementations of basic DBus::Exception methods
 */


#include <string>
#include <sstream>
#include <gio/gio.h>

#include "exceptions.hpp"


namespace DBus {

Exception::Exception(const std::string &classn,
                     const std::string &err,
                     GError *gliberr)
{
    std::ostringstream errbuf;
    errbuf << err;
    if (gliberr)
    {
        errbuf << ": " << std::string(gliberr->message);
        g_error_free(gliberr);
    }
    error = errbuf.str();

    std::ostringstream s;
    s << "[" << classn << "] " << error;
    classerr = s.str();
}


const char *Exception::what() const noexcept
{
    return classerr.c_str();
}


const char *Exception::GetRawError() const noexcept
{
    return error.c_str();
}


void Exception::SetDBusError(GDBusMethodInvocation *invocation,
                             std::string errdomain) const noexcept
{
    std::string qdom = (!errdomain.empty() ? errdomain : "net.openvpn.gdbuspp");
#ifdef GDBUSPP_DEBUG
    GError *dbuserr = g_dbus_error_new_for_dbus_error(qdom.c_str(), classerr.c_str());
#else
    GError *dbuserr = g_dbus_error_new_for_dbus_error(qdom.c_str(), error.c_str());
#endif
    dbuserr->domain = g_quark_from_string(qdom.c_str());
    g_dbus_method_invocation_return_gerror(invocation, dbuserr);
    g_error_free(dbuserr);
}


void Exception::SetDBusError(GError **dbuserror, GQuark domain, gint code) const noexcept
{
    g_set_error(dbuserror, domain, code, "%s", error.c_str());
}

} // namespace DBus
