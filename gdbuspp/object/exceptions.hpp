//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file object/exceptions.hpp
 *
 * @brief Declaration of DBus::Object::Exception
 *        These exceptions are typically used within the DBus::Object
 *        scope and will often provide more details related to the
 *        DBus::Object::Base where an issue occurred.
 */

#pragma once

#include <memory>
#include <string>
#include <gio/gio.h>

#include "../exceptions.hpp"


namespace DBus {
namespace Object {

class Base;

class Exception : public DBus::Exception
{
  public:
    Exception(const std::string &path,
              const std::string &interface,
              const std::string &errmsg,
              GError *gliberr = nullptr,
              const std::string &object_info = "");


    Exception(const std::shared_ptr<Object::Base> obj,
              const std::string &errmsg,
              GError *gliberr = nullptr,
              const std::string &object_info = "");

    Exception(const Object::Base *obj,
              const std::string &errmsg,
              GError *gliberr = nullptr,
              const std::string &object_info = "");

    Exception(const std::string &errmsg,
              GError *gliberr = nullptr);
};


namespace Property {

class Exception : public DBus::Object::Exception
{
  public:
    Exception(const std::shared_ptr<Object::Base> object,
              const std::string &property,
              const std::string &errmsg,
              GError *gliberr = nullptr);

    Exception(const Object::Base *object,
              const std::string &property,
              const std::string &errmsg,
              GError *gliberr = nullptr);

    void SetDBusErrorProperty(GError **error) const noexcept;
};

} // namespace Property

} // namespace Object
} // namespace DBus
