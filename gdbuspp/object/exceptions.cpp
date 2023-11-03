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
#include <glib.h>

#include "base.hpp"
#include "exceptions.hpp"


namespace DBus {

Object::Exception::Exception(const std::string &path,
                             const std::string &interface,
                             const std::string &errmsg,
                             GError *gliberr)
    : DBus::Exception(std::string("DBus::Object('" + path + "', '" + interface + "')"),
                      errmsg,
                      gliberr)
{
}


Object::Exception::Exception(const std::shared_ptr<Object::Base> obj,
                             const std::string &errmsg,
                             GError *gliberr)
    : DBus::Exception(std::string("DBus::Object('" + obj->GetPath() + "', '" + obj->GetInterface() + "')"),
                      errmsg,
                      gliberr)
{
}


Object::Exception::Exception(const Object::Base *obj,
                             const std::string &errmsg,
                             GError *gliberr)
    : DBus::Exception(std::string("DBus::Object('" + obj->GetPath() + "', '" + obj->GetInterface() + "')"),
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
    : Object::Exception(object, "::Property('" + property + "')" + errmsg, gliberr)
{
}


Object::Property::Exception::Exception(const Object::Base *object,
                                       const std::string &property,
                                       const std::string &errmsg,
                                       GError *gliberr)
    : Object::Exception(object, "::Property('" + property + "')" + errmsg, gliberr)
{
}


void Object::Property::Exception::SetDBusErrorProperty(GError **error) const noexcept
{
    g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s", GetRawError());
}


} // namespace DBus
