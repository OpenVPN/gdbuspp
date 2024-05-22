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
        g_dbus_error_strip_remote_error(gliberr);
        errbuf << (err.empty() ? "" : " ") << std::string(gliberr->message);
        g_error_free(gliberr);
    }

    // If the error is prefixed with a glib2 "GDBUs.Error:", it can be
    // split up into an error domain and an error message
    std::string tmperror = errbuf.str();
    auto gdbuserr_start = tmperror.find("GDBus.Error:");
    if (std::string::npos != gdbuserr_start)
    {
        auto domain_end = tmperror.find(":", gdbuserr_start + 13);
        if (std::string::npos != domain_end)
        {
            error = tmperror.substr(domain_end + 1);
            auto gdbuserr_end = tmperror.find(":") + 1;
            error_domain = tmperror.substr(gdbuserr_end,
                                           domain_end - gdbuserr_end);
        }
        else
        {
            error = tmperror;
        }
    }
    else
    {
        error = tmperror;
    }


    std::ostringstream s;
    s << "[" << classn << "] " << errbuf.str();
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


const char *Exception::DBusErrorDomain() const noexcept
{
    return error_domain.c_str();
}


void Exception::SetDBusError(GDBusMethodInvocation *invocation) const noexcept
{
#ifdef GDBUSPP_DEBUG
    GError *dbuserr = g_dbus_error_new_for_dbus_error(error_domain.c_str(),
                                                      classerr.c_str());
#else
    GError *dbuserr = g_dbus_error_new_for_dbus_error(error_domain.c_str(),
                                                      error.c_str());
#endif
    dbuserr->domain = g_quark_from_string(error_domain.c_str());
    g_dbus_method_invocation_return_gerror(invocation, dbuserr);
    g_error_free(dbuserr);
}


void Exception::SetDBusError(GError **dbuserror, GQuark domain, gint code) const noexcept
{
    g_set_error(dbuserror, domain, code, "%s", error.c_str());
}

} // namespace DBus
