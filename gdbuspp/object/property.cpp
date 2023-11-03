//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Antonio Quartulli <antonio@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//  Copyright (C)  Arne Schwabe <arne@openvpn.net>
//

/**
 * @file object/property.cpp
 *
 * @brief Implements the infrastructure needed to manage D-Bus object
 *        exposed properties and keep them under control of a
 *        DBus::Object::Base object
 */

#include <memory>
#include <glib.h>

#include "base.hpp"
#include "exceptions.hpp"
#include "property.hpp"


namespace DBus {


//
//  DBus::Object::Property::Update
//



GVariant *Object::Property::Update::Finalize()
{
    // Finalize all the collected value elements
    GVariant *vals = nullptr;
    if (updated_vals.size() == 1)
    {
        vals = updated_vals[0];
    }
    else if (updated_vals.size() == 0)
    {
        vals = g_variant_new_variant(nullptr);
    }
    else
    {
        GVariantBuilder *arvals = g_variant_builder_new(G_VARIANT_TYPE(property.GetDBusType()));
        for (auto &val : updated_vals)
        {
            g_variant_builder_add_value(arvals, val);
        }
        vals = g_variant_builder_end(arvals);
        g_variant_builder_unref(arvals);
    }

    // Build a single element array containing all updates
    // for this single property
    GVariantBuilder *msg = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
    g_variant_builder_add(msg, "{sv}", property.GetName().c_str(), vals);

    // Wrap this up into the needed format needed when sending the
    // org.freedesktop.DBus.Properties.PropertiesChanged signal
    GVariant *update_msg = g_variant_new("(sa{sv}as)",
                                         property.GetInterface().c_str(),
                                         msg,
                                         nullptr);
    g_variant_builder_unref(msg);
    updated_vals = {};

    // Return this to the glib2 set-property callback method
    // which will emit the signal and do the needed error handling
    return update_msg;
}



//
//  DBus::Object::PropertyBySpec
//



Object::Property::BySpec::BySpec(const std::string &interface_arg,
                                 const std::string &name_arg,
                                 const bool readwr_arg,
                                 const std::string &dbustype_arg,
                                 const GetPropertyCallback get_cb,
                                 const SetPropertyCallback set_cb)
    : interface(interface_arg),
      name(name_arg),
      readwrite(readwr_arg),
      dbustype(dbustype_arg),
      get_callback(get_cb),
      set_callback(set_cb)
{
}


const std::string Object::Property::BySpec::GenerateIntrospection() const
{
    return std::string("<property") + " type='" + dbustype + "'"
           + " name ='" + name + "' "
           + " access='" + (readwrite ? "readwrite" : "read") + "' "
           + "/>";
}


const char *Object::Property::BySpec::GetDBusType() const noexcept
{
    return dbustype.c_str();
}


GVariant *Object::Property::BySpec::GetValue() const noexcept
{
    return get_callback(*this);
}


Object::Property::Update::Ptr Object::Property::BySpec::SetValue(GVariant *value)
{
    return set_callback(*this, value);
}


const std::string &Object::Property::BySpec::GetName() const noexcept
{
    return name;
}


const std::string &Object::Property::BySpec::GetInterface() const noexcept
{
    return interface;
}



//
//   DBus::Object::PropertyCollection
//



Object::Property::Collection::Collection()
{
}


void Object::Property::Collection::AddBinding(Property::Interface::Ptr prop)
{
    properties.insert(std::pair<std::string, Property::Interface::Ptr>(prop->GetName(), prop));
}

bool Object::Property::Collection::Exists(const std::string &name) const noexcept
{
    auto prop = properties.find(name);
    return prop != properties.end();
}

const std::string Object::Property::Collection::GenerateIntrospection() const noexcept
{
    if (properties.empty())
    {
        return "";
    }
    std::string xml = "";
    for (auto &prop : properties)
        xml += prop.second->GenerateIntrospection();

    return xml;
}


GVariant *Object::Property::Collection::GetValue(const std::string &property_name) const noexcept
{
    auto prop = properties.find(property_name);
    if (prop == properties.end())
    {
        return nullptr;
    }

    return prop->second->GetValue();
}


Object::Property::Update::Ptr Object::Property::Collection::SetValue(const std::string &property_name,
                                                                     GVariant *value)
{
    auto prop = properties.find(property_name);
    if (prop == properties.end())
    {
        return nullptr;
    }

    return prop->second->SetValue(value);
}

} // namespace DBus
