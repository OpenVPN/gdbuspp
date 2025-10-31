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

#include "../glib2/utils.hpp"
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
        vals = glib2::Value::NullVariant();
    }
    else
    {
        GVariantBuilder *arvals = glib2::Builder::Create(property.GetDBusType());
        for (const auto &value : updated_vals)
        {
            glib2::Builder::Add(arvals, value);
        }
        vals = glib2::Builder::Finish(arvals);
    }

    // Build a single element array containing all updates
    // for this single property.
    //
    // This information will be used to send the
    // org.freedesktop.Properties.PropertiesChanged D-Bus
    // signal. This should be emitted each time an object
    // property is updated.
    //
    // The complete signal D-Bus type is: (sa{sv}as)
    // This is a structured object with the following fields:
    //
    //     s - interface name where the properties belongs to
    // a{sv} - key/value dictionary of all changed properties
    //         where the key holds the property name.
    //    as - array of properties which has been invalidated.
    //         This implementation always sends an empty array
    //         in this case. (This may change in the future)

    // Create the property/value dictionary
    GVariantDict *property_values = glib2::Dict::Create();
    glib2::Dict::Add(property_values,
                     property.GetName(),
                     vals);

    // Wrap it up as the final update message to be emitted as the
    // org.freedesktop.DBus.Properties.PropertiesChanged signal
    GVariantBuilder *update_msg = glib2::Builder::Create("(sa{sv}as)");
    glib2::Builder::Add(update_msg, property.GetInterface());
    glib2::Builder::Add(update_msg, glib2::Dict::Finish(property_values));
    glib2::Builder::Add(update_msg, glib2::Builder::CreateEmpty("as"));

    // Reset the list of of updated values
    updated_vals = {};

    // Return this to the glib2 set-property callback method
    // which will emit the signal and do the needed error handling
    return glib2::Builder::Finish(update_msg);
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


GVariant *Object::Property::BySpec::GetValue() const
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


GVariant *Object::Property::Collection::GetValue(const std::string &property_name) const
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

    auto property = prop->second;
    std::string value_type(g_variant_get_type_string(value));
    if (property->GetDBusType() != value_type)
    {
        throw Object::Exception("Invalid data type for the property value");
    }
    return property->SetValue(value);
}

} // namespace DBus
