
//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Arne Schwabe <arne@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//  Copyright (C)  Lev Stipakov <lev@openvpn.net>
//

/**
 * @file   glib2/utils.cpp
 *
 * @brief  Implementation of glib2::Utils and related functionality
 */

#include <string>
#include <glib.h>
#include <gio/gio.h>

#include "../exceptions.hpp"
#include "utils.hpp"

namespace glib2 {
namespace Utils {

Exception::Exception(const std::string &err,
                     GError *gliberr)
    : DBus::Exception(std::string("glib2::Utils::" + std::string(__func__)),
                      err,
                      gliberr){};
Exception::Exception::Exception(const std::string &callfunc,
                                const std::string &err,
                                GError *gliberr)
    : DBus::Exception(std::string("glib2::Utils::" + std::string(__func__)
                                  + " [" + callfunc + "]"),
                      err,
                      gliberr){};


void CheckCapabilityFD(const GDBusConnection *dbuscon)
{
    if (!(g_dbus_connection_get_capabilities((GDBusConnection *)dbuscon) & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING))
    {
        throw Utils::Exception("D-Bus connection does not support file descriptor passing");
    }
}


void unref_fdlist(GUnixFDList *fdlist)
{
    g_object_unref((GVariant *)fdlist);
}


void checkParams(const char *func,
                 GVariant *params,
                 const char *format,
                 unsigned int num)
{
    std::string typestr{};
    size_t nchildren = 0;
    if (params)
    {
        typestr = std::string(g_variant_get_type_string(params));
        nchildren = g_variant_n_children(params);
    }

    if (format != typestr || (num > 0 && num != nchildren))
    {
        std::ostringstream err;
        err << "Incorrect parameter format: "
            << (params ? typestr : "<null>")
            << ", expected " << format;
        if (num > 0)
        {
            err << " (elements expected: " << std::to_string(num);
            if (params)
            {
                err << ", received: " << std::to_string(nchildren);
            }
            else
            {
                err << ", received none";
            }
            err << ")";
        }
        throw Exception(func, err.str());
    }
}
} // namespace Utils



namespace Builder {

GVariant *TupleWrap(GVariantBuilder *bld)
{
    GVariantBuilder *wrapper = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
    g_variant_builder_add_value(wrapper, g_variant_builder_end(bld));
    GVariant *ret = g_variant_builder_end(wrapper);
    g_variant_builder_unref(wrapper);
    g_variant_builder_unref(bld);
    return ret;
}

GVariant *CreateEmpty(const char *type)
{
    GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE(type));
    GVariant *ret = g_variant_builder_end(b);
    g_variant_builder_unref(b);
    return ret;
}

} // namespace Builder

} // namespace glib2
