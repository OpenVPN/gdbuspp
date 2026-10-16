//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file object/exceptions.cpp
 *
 * @brief Implementation of DBus::Object::Exception
 *        These exceptions are typically used within the DBus::Object
 *        scope and will often provide more details related to the
 *        DBus::Object::Base where an issue occurred.
 */

#include <memory>
#include <string>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <glib.h>

#include "base.hpp"
#include "exceptions.hpp"
#include "path.hpp"

/**
 * Local anonymous namespace
 */
namespace {

/**
 *  Compose a string based on details referring to a DBus::Object
 *
 * @param path         DBus::Object::Path of the referenced object
 * @param interface    std::string with the interface part of the object
 * @param info         std::string which is added when not empty
 *
 * @return std::string
 */
static std::string compose_object_descr(const DBus::Object::Path &path,
                                        const std::string &interface,
                                        const std::string &info)
{
    return fmt::format(FMT_COMPILE("DBus::Object('{}', '{}'{})"),
                       path,
                       interface,
                       (!info.empty() ? ", " + info : ""));
}
} // namespace


namespace DBus {

Object::Exception::Exception(const Object::Path &path,
                             const std::string &interface,
                             const std::string &errmsg,
                             GError *gliberr,
                             const std::string &object_info)
    : DBus::Exception(compose_object_descr(
                          path,
                          interface,
                          object_info),
                      errmsg,
                      gliberr)
{
}


Object::Exception::Exception(const std::shared_ptr<Object::Base> obj,
                             const std::string &errmsg,
                             GError *gliberr,
                             const std::string &object_info)
    : DBus::Exception(compose_object_descr(
                          obj->GetPath(), obj->GetInterface(), object_info),
                      errmsg,
                      gliberr)
{
}


Object::Exception::Exception(const Object::Base *obj,
                             const std::string &errmsg,
                             GError *gliberr,
                             const std::string &object_info)
    : DBus::Exception(compose_object_descr(
                          obj->GetPath(), obj->GetInterface(), object_info),
                      errmsg,
                      gliberr)
{
}


Object::Exception::Exception(const std::string &errmsg,
                             GError *gliberr)
    : DBus::Exception("DBus::Object()",
                      errmsg,
                      gliberr)
{
}



//
//  DBus::Object::Property::Exception
//



Object::Property::Exception::Exception(const std::shared_ptr<Object::Base> object,
                                       const std::string &property,
                                       const std::string &errmsg,
                                       GError *gliberr)
    : Object::Exception(object,
                        errmsg,
                        gliberr,
                        fmt::format(FMT_COMPILE("property='{}'"), property))
{
}


Object::Property::Exception::Exception(const Object::Base *object,
                                       const std::string &property,
                                       const std::string &errmsg,
                                       GError *gliberr)
    : Object::Exception(object, errmsg, gliberr, fmt::format("property='{}'", property))
{
}


void Object::Property::Exception::SetDBusErrorProperty(GError **error) const noexcept
{
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", GetRawError());
}


} // namespace DBus
