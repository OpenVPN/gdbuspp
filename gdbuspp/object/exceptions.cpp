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
#include <sstream>
#include <string>
#include <glib.h>

#include "base.hpp"
#include "exceptions.hpp"
#include "path.hpp"

namespace _private::exception {
static const std::string comppse_object_descr(const DBus::Object::Path &path,
                                              const std::string &interface,
                                              const std::string &info)
{
    std::ostringstream ret;
    ret << "DBus::Object('" << path << "', '" << interface << "'";
    if (!info.empty())
    {
        ret << ", " << info;
    }
    ret << ")";
    return ret.str();
}
} // namespace _private::exception


namespace DBus {

Object::Exception::Exception(const Object::Path &path,
                             const std::string &interface,
                             const std::string &errmsg,
                             GError *gliberr,
                             const std::string &object_info)
    : DBus::Exception(_private::exception::comppse_object_descr(
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
    : DBus::Exception(_private::exception::comppse_object_descr(
                          obj->GetPath(), obj->GetInterface(), object_info),
                      errmsg,
                      gliberr)
{
}


Object::Exception::Exception(const Object::Base *obj,
                             const std::string &errmsg,
                             GError *gliberr,
                             const std::string &object_info)
    : DBus::Exception(_private::exception::comppse_object_descr(
                          obj->GetPath(), obj->GetInterface(), object_info),
                      errmsg,
                      gliberr)
{
}


Object::Exception::Exception(const std::string &errmsg,
                             GError *gliberr)
    : DBus::Exception(std::string("DBus::Object()"),
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
    : Object::Exception(object, errmsg, gliberr, "property='" + property + "'")
{
}


Object::Property::Exception::Exception(const Object::Base *object,
                                       const std::string &property,
                                       const std::string &errmsg,
                                       GError *gliberr)
    : Object::Exception(object, errmsg, gliberr, "property='" + property + "'")
{
}


void Object::Property::Exception::SetDBusErrorProperty(GError **error) const noexcept
{
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", GetRawError());
}


} // namespace DBus
