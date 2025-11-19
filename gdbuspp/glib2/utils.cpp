
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
                      gliberr) {};
Exception::Exception::Exception(const std::string &callfunc,
                                const std::string &err,
                                GError *gliberr)
    : DBus::Exception(std::string("glib2::Utils::" + std::string(__func__)
                                  + " [" + callfunc + "]"),
                      err,
                      gliberr) {};


void CheckCapabilityFD(GDBusConnection *dbuscon)
{
    auto capabilities = g_dbus_connection_get_capabilities(static_cast<GDBusConnection *>(dbuscon));
    if (!(capabilities & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING))
    {
        throw Utils::Exception("D-Bus connection does not support file descriptor passing");
    }
}


void unref_fdlist(GUnixFDList *fdlist)
{
    g_object_unref(reinterpret_cast<GVariant *>(fdlist));
}


void checkParams(const char *func,
                 GVariant *params,
                 const char *format,
                 unsigned int num)
{
    std::string typestr{};
    size_t nchildren = 0;
    bool container = false;
    if (params)
    {
        typestr = std::string(g_variant_get_type_string(params));
        const GVariantType *g_type = g_variant_get_type(params);
        container = g_variant_type_is_container(g_type);
        if (container)
        {
            nchildren = g_variant_n_children(params);
        }
    }

    if (!container && num > 0)
    {
        throw glib2::Utils::Exception(func, "Parameter type is not a container, it has no children");
    }

    if (format != typestr || (container && num > 0 && num != nchildren))
    {
        std::ostringstream err;
        err << "Incorrect parameter format: "
            << (params ? typestr : "<null>")
            << ", expected " << format;
        if (nchildren > 0 && num > 0)
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
        throw glib2::Utils::Exception(func, err.str());
    }
}

} // namespace Utils



namespace DataType {

std::string Extract(GVariant *value) noexcept
{
    return std::string(g_variant_get_type_string(value));
}

} // namespace DataType


namespace Value {

void IterateArray(GVariant *array, std::function<void(GVariant *data)> parser)
{
    if (!array)
    {
        return;
    }

    if (!g_variant_is_container(array))
    {
        throw glib2::Utils::Exception("Input data is not a container [" + DataType::Extract(array) + "]");
    }

    // If the array is stored inside a tuple, extract the array from
    // the tuple before starting iterate the elements.
    bool is_tuple = g_variant_type_is_tuple(g_variant_get_type(array));
    GVariant *iterate_values = (is_tuple ? g_variant_get_child_value(array, 0) : array);
    if (!g_variant_type_is_array(g_variant_get_type(iterate_values)))
    {
        throw glib2::Utils::Exception("Input data is not an array [" + DataType::Extract(iterate_values) + "]");
    }

    GVariantIter array_iterator;
    g_variant_iter_init(&array_iterator, iterate_values);

    GVariant *element = nullptr;
    while ((element = g_variant_iter_next_value(&array_iterator)))
    {
        parser(element);
        g_variant_unref(element);
    }

    if (is_tuple)
    {
        // If the array was extracted from a tuple,
        // the extracted values need to be released
        g_variant_unref(iterate_values);
    }
}


GVariant *NullVariant()
{
    return g_variant_new_variant(nullptr);
}


GVariant *TupleWrap(GVariant *data)
{
    if (!data)
    {
        return nullptr;
    }

    if (g_variant_type_is_tuple(g_variant_get_type(data)))
    {
        return data;
    }

    GVariantBuilder *bld = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
    if (!bld)
    {
        throw glib2::Utils::Exception("Builder::TupleWrap()",
                                      "Failed allocating new GVariantBuilder");
    }
    g_variant_builder_add_value(bld, data);

    return glib2::Builder::Finish(bld);
}

} // namespace Value


namespace Builder {

GVariantBuilder *Create(const char *type)
{
    GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE(type));
    if (!b)
    {
        throw glib2::Utils::Exception("Builder::Create()",
                                      "Failed allocating new GVariantBuilder");
    }
    return b;
}


GVariantBuilder *EmptyTuple()
{
    return g_variant_builder_new(G_VARIANT_TYPE(DataType::DBUS_TYPE_TUPLE));
}


void OpenChild(GVariantBuilder *builder, const char *type)
{
    g_variant_builder_open(builder, G_VARIANT_TYPE(type));
}


void CloseChild(GVariantBuilder *builder)
{
    g_variant_builder_close(builder);
}


GVariant *CreateEmpty(const char *type)
{
    GVariantBuilder *b = Builder::Create(type);
    GVariant *ret = g_variant_builder_end(b);
    g_variant_builder_unref(b);
    if (!ret)
    {
        throw glib2::Utils::Exception("Builder::CreateEmpty()",
                                      "Failed allocating new GVariantBuilder");
    }
    return ret;
}


GVariant *Finish(GVariantBuilder *builder) noexcept
{
    GVariant *ret = g_variant_builder_end(builder);
    g_variant_builder_unref(builder);
    return ret;
}

} // namespace Builder


namespace Dict {

void IterateDictionary(GVariant *dict, std::function<void(const std::string &key, GVariant *value)> extractor)
{
    Utils::checkParams(__func__, dict, "a{sv}");

    GVariantIter element_iter;
    g_variant_iter_init(&element_iter, dict);

    char *key = nullptr;
    GVariant *value = nullptr;
    while (g_variant_iter_loop(&element_iter, "{sv}", &key, &value))
    {
        extractor(std::string(key), value);
    }
}

} // namespace Dict

} // namespace glib2
