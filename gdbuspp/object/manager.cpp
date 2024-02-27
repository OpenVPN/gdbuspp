//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Charlie Vigue <charlie.vigue@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file object/manager.cpp
 *
 * @brief  Implements the non-template based methods for the DBus::ObjectManager
 */

#include <iostream>
#include <string>
#include <map>
#include <mutex>
#include <gio/gio.h>

#include "../async-process.hpp"
#include "../features/idle-detect.hpp"
#include "../glib2/callbacks.hpp"
#include "manager.hpp"
#include "callbacklink.hpp"


namespace DBus {
namespace Object {

namespace _private::Manager {
std::mutex objectmgr_update_mtx;
}

Manager::Exception::Exception(const std::string &errmsg, GError *gliberr)
    : DBus::Exception("ObjectManager", errmsg, gliberr)
{
}


Manager::Exception::Exception(const Object::Base::Ptr obj,
                              const std::string &errmsg,
                              GError *gliberr)
    : DBus::Exception("ObjectManager[path=" + obj->GetPath()
                          + ", interface=" + obj->GetInterface() + "]",
                      errmsg,
                      gliberr)
{
}



Manager::Manager(DBus::Connection::Ptr conn)
    : connection(conn)
{
    _int_dbusobj_interface_vtable = {
        glib2::Callbacks::_int_dbusobject_callback_method_call,
        glib2::Callbacks::_int_dbusobject_callback_get_property,
        glib2::Callbacks::_int_dbusobject_callback_set_property};

    request_pool = AsyncProcess::Pool::Create();
}


void Manager::PrepareIdleDetector(const std::chrono::duration<uint32_t> timeout,
                                  std::shared_ptr<DBus::MainLoop> mainloop)
{
    if (idle_detector)
    {
        throw Manager::Exception("EnableIdleDetector: "
                                 "An idle detector is already setup");
    }
    if (!mainloop)
    {
        throw Manager::Exception("EnableIdleDetector: "
                                 "A valid DBus::Mainloop object is required");
    }
    if (std::chrono::duration<uint32_t>(0) == timeout)
    {
        // Timeout set to 0, which disables this feature
        return;
    }

    try
    {
        idle_detector = Features::IdleDetect::Create(mainloop,
                                                     timeout,
                                                     shared_from_this());
    }
    catch (const std::bad_weak_ptr &)
    {
        throw Manager::Exception("EnableIdleDetector: "
                                 "Could not create the internal Idle "
                                 "Detection object (bad_weak_ptr)");
    }
}


void Manager::RunIdleDetector(const bool run)
{
    if (idle_detector)
    {
        if (run)
        {
            idle_detector->Start();
        }
        else
        {
            idle_detector->Stop();
        }
    }
}


void Manager::IdleActivityUpdate() const noexcept
{
    if (idle_detector)
    {
        idle_detector->ActivityUpdate();
    }
}


void Manager::RemoveObject(const Object::Path &path)
{
    try
    {
        unsigned int obj_id = path_index.at(path);
        g_dbus_connection_unregister_object(connection->ConnPtr(), obj_id);
    }
    catch (const std::out_of_range &)
    {
        throw Manager::Exception("RemoveObject: "
                                 "Object path not found: "
                                 + path);
    }
}


const std::map<Object::Path, Object::Base::Ptr> Manager::GetAllObjects() const
{
    // Loop through all object managed by this Object::Manager
    std::map<Object::Path, Object::Base::Ptr> ret{};
    for (const auto &[obj_id, cbl] : object_map)
    {
        // Look up the D-Bus object path from the object path/object ID index
        for (const auto &[map_path, map_id] : path_index)
        {
            // When there is a match between the object ID in the object_map
            // with the object ID in the path_index, the path_index will point
            // at the correct D-Bus path ...
            if (map_id == obj_id)
            {
                // And we can extract the Base::Ptr to the D-Bus object.
                ret[map_path] = cbl->object;
            }
        }
    }
    return ret;
}


void Manager::_destructObjectCallback(const Object::Path &path)
{
    std::lock_guard<std::mutex> lg(_private::Manager::objectmgr_update_mtx);

    const auto path_it = path_index.find(path);
    if (path_index.end() == path_it)
    {
        throw Manager::Exception("DestructObject: Object path not found: " + path);
    }

    const auto obj_it = object_map.find(path_it->second);
    if (object_map.end() == obj_it)
    {
        throw Manager::Exception("DestructObject: Object index "
                                 + std::to_string(path_it->second)
                                 + " not found for path: " + path);
    }
    object_map.erase(obj_it);
    path_index.erase(path_it);
}


void Manager::register_object(const DBus::Object::Base::Ptr object)
{
    // Parse the XML introepsection data each D-Bus object must provide
    GError *error = nullptr;
    GDBusNodeInfo *introsp = nullptr;
    try
    {
        std::string xml = object->GenerateIntrospection();
        introsp = g_dbus_node_info_new_for_xml(xml.c_str(), &error);

        if (nullptr == introsp || error)
        {
            throw Manager::Exception(object,
                                     "Failed to parse introspection XML",
                                     error);
        }
    }
    catch (...)
    {
        throw;
    }


    // Prepare a CallbackLink which provides access to this new object,
    // this object manager and the AsyncProcess based request pool
    std::lock_guard<std::mutex> lg(_private::Manager::objectmgr_update_mtx);
    CallbackLink::Ptr cblink = CallbackLink::Create(object, GetWPtr(), request_pool);

    // Register the new object, via the CallbackLink object, on the D-Bus.
    //
    // This will register all the needed C based callback functions
    // to respond to D-Bus proxy requests on this object.
    //
    // A destruction callback function is also setup, which will be
    // used when the D-Bus object is requested deleted from the D-Bus.
    //
    // Since glib2 is C based, there are a few jumps back and forth
    // before the _destructObjectCallback() method in the ObjectManager
    // is called to release and delete object
    //
    unsigned int oid = 0;
    oid = g_dbus_connection_register_object(connection->ConnPtr(),
                                            object->GetPath().c_str(),
                                            introsp->interfaces[0],
                                            &_int_dbusobj_interface_vtable,
                                            cblink.get(),
                                            glib2::Callbacks::_int_dbusobject_callback_destruct,
                                            &error);
    g_dbus_node_info_unref(introsp);
    if (oid < 1)
    {
        throw Manager::Exception(object,
                                 "Failed registering object",
                                 error);
    }

    // Put this object into our internal object container.  This will
    // be used when a D-Bus object wants to be removed from the D-Bus service.
    object_map[oid] = cblink;
    path_index[object->GetPath()] = oid;
}


Object::Base::Ptr Manager::get_object(const Object::Path &path) const
{
    try
    {
        unsigned int id = path_index.at(path);
        CallbackLink::Ptr cbl = object_map.at(id);
        return cbl->object;
    }
    catch (const std::out_of_range &)
    {
        return nullptr;
    }
}

} // namespace Object
} // namespace DBus
