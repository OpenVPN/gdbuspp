//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file object/base.cpp
 *
 * @brief  Implementation of the DBus::Object::Base class
 *         This class is intended to be inherited by a specialized
 *         class representing its own implementation of a D-Bus
 *         object in the service.
 *
 *         This Object::Base class provides the generic basic features
 *         all D-Bus object needs to handle.  This includes keeping an
 *         overview of all D-Bus object methods and properties made
 *         available through the D-Bus service
 */


#include <sstream>
#include <string>
#include <glib.h>

#include "../async-process.hpp"
#include "../glib2/utils.hpp"
#include "../signals/group.hpp"
#include "exceptions.hpp"
#include "property.hpp"
#include "base.hpp"


namespace DBus {


//
//
//   DBus::Object::Base
//
//


Object::Base::Base(const std::string &path, const std::string &interf)
    : object_path(path), interface(interf)
{
    methods = Object::Method::Collection::Create();
    properties = Object::Property::Collection::Create();
}


const std::string &Object::Base::GetPath() const noexcept
{
    return object_path;
}


const std::string &Object::Base::GetInterface() const noexcept
{
    return interface;
}


void Object::Base::AddPropertyBySpec(const std::string name,
                                     const std::string dbustype,
                                     Property::BySpec::GetPropertyCallback get_cb,
                                     Property::BySpec::SetPropertyCallback set_cb)
{
    Property::BySpec::Ptr prop = Property::BySpec::Create(
        interface,
        name,
        true,
        dbustype,
        get_cb,
        set_cb);
    properties->AddBinding(prop);
}


void Object::Base::AddPropertyBySpec(const std::string name,
                                     const std::string dbustype,
                                     Property::BySpec::GetPropertyCallback get_cb)
{
    auto set_readonly_cb{
        [](const DBus::Object::Property::BySpec &prop, GVariant *v) -> Object::Property::Update::Ptr
        {
            throw Object::Exception("Property '" + prop.GetName() + "' is read-only");
        }};

    Property::BySpec::Ptr prop = Property::BySpec::Create(interface,
                                                          name,
                                                          false,
                                                          dbustype,
                                                          get_cb,
                                                          set_readonly_cb);
    properties->AddBinding(prop);
}


bool Object::Base::PropertyExists(const std::string &propname) const
{
    return properties->Exists(propname);
}


GVariant *Object::Base::GetProperty(const std::string &propname) const
{
    return properties->GetValue(propname);
}


Object::Property::Update::Ptr Object::Base::SetProperty(const std::string &propname, GVariant *value)
{
    try
    {
        return properties->SetValue(propname, value);
    }
    catch (const glib2::Utils::Exception &excp)
    {
        throw Property::Exception(this, propname, excp.GetRawError());
    }
}


Object::Method::Arguments::Ptr Object::Base::AddMethod(const std::string &method_name,
                                                       Method::CallbackFnc method_callback)
{
    return methods->AddMethod(std::move(method_name), std::move(method_callback));
}


void Object::Base::RegisterSignals(const Signals::Group::Ptr signal_group)
{
    if (signals)
    {
        throw Object::Exception(this, "Signals already registered");
    }
    signals = signal_group;
}


const std::string Object::Base::GenerateIntrospection() const
{
    std::ostringstream xml;
    xml << "<node name='" << object_path << "'>"
        << "  <interface name='" << interface << "'>"
        << methods->GenerateIntrospection()
        << properties->GenerateIntrospection()
        << (signals ? signals->GenerateIntrospection() : "")
        << "  </interface>"
        << "</node>";
    return std::string(xml.str());
}


const bool Object::Base::MethodExists(const std::string &meth_name) const
{
    return methods->Exists(meth_name);
}


void Object::Base::MethodCall(AsyncProcess::Request::UPtr &req)
{
    try
    {
        methods->Execute(req);
    }
    catch (const Object::Method::Exception &excp)
    {
        throw DBus::Object::Exception(req->object, excp.GetRawError());
    }
}


void Object::Base::DisableIdleDetector(const bool disable)
{
    disable_idle_detection = disable;
}


const bool Object::Base::GetIdleDetectorDisabled() const
{
    return disable_idle_detection;
}

} // namespace DBus
