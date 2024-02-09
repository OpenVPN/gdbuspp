//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Charlie Vigue <charlie.vigue@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file object/manager.hpp
 *
 * @brief  Declares a mechanism to create, own and manage
 *         DBus::Object::Base based objects for a D-Bus service
 */

#pragma once

#include <chrono>
#include <memory>
#include <glib.h>

#include "../connection.hpp"
#include "../glib2/callbacks.hpp"
#include "base.hpp"

namespace DBus {

namespace Features {
class IdleDetect; // Forward declaration; see features/idle-detect.hpp
}

class MainLoop; // Forward declaration; see mainloop.hpp

namespace Object {


class CallbackLink; // forward declaration; declared in object/callbacklink.hpp


/**
 *  The DBus::Object::Manager object keeps track of all the D-Bus objects
 *  created in a D-Bus service and the mapping to the corresponding
 *  DBus::Object::Base based objects.  When the object manager is being
 *  torn down, it will also clean up and de-register all the DBus::Object::Base
 *  objects as well.
 */
class Manager : public std::enable_shared_from_this<Manager>
{
  public:
    using Ptr = std::shared_ptr<Manager>;
    using WPtr = std::weak_ptr<Manager>;

    class Exception : public DBus::Exception
    {
      public:
        Exception(const std::string &errmsg, GError *gliberr = nullptr);

        Exception(const Object::Base::Ptr obj,
                  const std::string &errmsg,
                  GError *gliberr = nullptr);
    };


    /**
     *  Constructor for establishing a new Object Manager.
     *
     *  This is designed to only have one object manager per service.
     *  The DBus::Service object will prepare an Object::Manager when
     *  registering a new service on the D-Bus.
     *
     * @param conn   Reference to a DBus::Connection where this service
     *               operates, which this Object::Manager is bound to.
     */
    [[nodiscard]] static Manager::Ptr CreateManager(DBus::Connection::Ptr &conn)
    {
        return Manager::Ptr(new Manager(conn));
    }

    ~Manager() noexcept = default;


    /**
     *  Retrieve a std::weak_ptr<Object::Manager> object to this
     *  instantiated object.  This is used to provide access to this
     *  object via the Object::CallbackLink object.
     *
     * @return WPtr  aka std::weak_ptr<Object::Manager>
     */
    WPtr GetWPtr()
    {
        WPtr ret = shared_from_this();
        return ret;
    }


    /**
     *  Creates a new DBus::Object::Base based object on the D-Bus for
     *  the running service.
     *
     *  This method will instantiate the new object on internally and will
     *  "own it".
     *
     *  The created object will have a Object::CallbackLink setup and
     *  will be registered on the D-Bus.
     *
     *  @code
     *
     *    class MyObject : public DBus::Object::Base
     *    {
     *        MyObject(int arg1, int arg2, int arg3);
     *    }
     *
     *    auto connection = DBus::Connection::Create(DBus::BusType::SESSION);
     *    auto object_mgr = Object::Manager::CreateManager(connection);
     *    auto my_obj = object_mgr->CreateObject<MyObject>(arg1, arg2, arg3);
     *
     *  @endcode
     *
     * @tparam C     Class of the DBus::Object::Base object to create
     * @param args   All the arguments the template class C requires
     *
     * @return std::shared_ptr<C> (aka: C::Ptr) to the newly created
     *              DBus::Object::Base object.  This can be used to access
     *              this new object immediately after creating it.
     */
    template <class C, typename... Args>
    std::shared_ptr<C> CreateObject(Args &&...args)
    {
        // Instantiate and register the new object
        std::shared_ptr<C> object = Object::Base::Create<C>(std::forward<Args>(args)...);
        register_object(object);

        return object;
    }


    /**
     *  Activate the idle-exit mechanism.  If no objects are active in the
     *  service, the service will shutdown after the given timeout value.
     *
     * @param timeout  std::chrono::duration describing the idle timeout
     *                 of the service
     */
    void PrepareIdleDetector(const std::chrono::duration<uint32_t> timeout,
                             std::shared_ptr<DBus::MainLoop> mainloop);

    /**
     *  Control if the idle detector should run or not.
     *
     *  This is normally called by glib2::Callbacks::_int_callback_name_acquired()
     *  and glib2::Callbacks::_int_callback_name_lost() which starts and
     *  stops the idle detector when the service is registered and available on
     *  the D-Bus.
     *
     *  The DBus::Service object can also call this method, as part of the
     *  shutdown logic; to ensure it does not leave any threads behind.
     *
     * @param run   bool flag to start or stop the Features::IdleDetect
     *              functionality.  When true, the idle detector thread is
     *              started, otherwise the thread is stopped.
     */
    void RunIdleDetector(const bool run);

    /**
     *  This is primarily called via functions in the glib2::Callbacks scope,
     *  to just update the internal IdleDetect last activity timestamp.  As
     *  long as this timestamp + the configured timeout is behind the current
     *  host system's current timestamp, this service should be kept running.
     */
    void IdleActivityUpdate() const noexcept;

    /**
     *  This method is can be called by any DBus::Object::Base object
     *  to remove an object from this D-Bus service.
     *
     *  Calling this will also result in deleting the C++ object from
     *  memory.
     *
     * @param path  DBus::Object::Path to the D-Bus object to remove
     */
    void RemoveObject(const Object::Path &path);

    /**
     *  Retrieve a shared_ptr to the DBus::Object::Base object with the
     *  real implementation class used when registering the object.
     *
     * @tparam C    Class implementation of registered object
     * @param path  DBus::Object::Path to the D-Bus object
     *
     * @return std::shared_ptr<C>
     */
    template <typename C>
    std::shared_ptr<C> GetObject(const Object::Path &path) const
    {
        Object::Base::Ptr obj_ptr = get_object(path);
        if (obj_ptr)
        {
            std::shared_ptr<C> obj = std::static_pointer_cast<C>(obj_ptr);
            return obj;
        }
        return nullptr;
    }

    /**
     *  Retrieve a std::map<> of all the DBus::Object::Base managed objects
     *  with the registered D-Bus path to the object as the key.
     *
     * @return const std::map<DBus::Object::Path, Object::Base::Ptr>
     */

    const std::map<Object::Path, Object::Base::Ptr> GetAllObjects() const;


  private:
    //
    //  Private variable members
    //

    /**
     * Reference to the DBus::Connection where the service is hosted
     */
    DBus::Connection::Ptr connection;

    /**
     *  An thread pool to handle some D-Bus calls asynchronously
     */
    AsyncProcess::Pool::Ptr request_pool{};

    /**
     *  The Idle Detector object.  This is activated via the
     *  PrepareIdleDetector() method.
     */
    std::shared_ptr<Features::IdleDetect> idle_detector{nullptr};

    /**
     *  The main object map, which owns the DBus::Object::Base objects.
     *  Via the Object::CallbackLink object, this map keeps a link to
     *  the C++ object (Object::Base) of the D-Bus object together
     *  with a link to the AsyncProcess::Pool and this Object::Manager object.
     */
    std::map<unsigned int, std::shared_ptr<CallbackLink>> object_map = {};

    /**
     *  Lookup index to quickly find the glib2 GDBus object id for
     *  a specific D-Bus path
     */
    std::map<Object::Path, unsigned int> path_index = {};

    /**
     *  Callback function table for D-Bus; used by the private
     *  Object::Manager::register_object() method
     */
    GDBusInterfaceVTable _int_dbusobj_interface_vtable;

    //
    //  private methods
    //

    /**
     *  The real constructor, only to be accessed via @CreateManager()
     *
     * @param conn  DBus::Connection object with the D-Bus connection
     */
    Manager(DBus::Connection::Ptr conn);

    /**
     *  Internal method to register the DBus::Object::Base object on the
     *  D-Bus for this service
     *
     * @param object  DBus::Object::Base::Ptr to the new object to register
     */
    void register_object(const DBus::Object::Base::Ptr object);

    /**
     *  Internal method to retrieve the shared_ptr to a DBus::Object::Base
     *  object by D-Bus path
     *
     * @param path                DBus::Object::Path to the D-Bus object
     * @return Object::Base::Ptr  shared_ptr to the DBus::Object::Base object.
     *         Returns nullptr if the object path is not found.
     */
    Object::Base::Ptr get_object(const Object::Path &path) const;

    /**
     *  Internal callback method which deletes the DBus::Object::Base object
     *  from memory.  The object will be removed from the internal object
     *  container and path index.
     *
     *  This is only exposed like this for the
     *  @glib2::Calllbacks::_int_dbusobject_callback_destruct function to
     *  be able to access it.  That callback function is triggered via the
     *  Object::Manager::RemoveObject() method above.  This will call the
     *  g_dbus_connection_unregister_object() (glib2 C library) function.
     *
     *  This method is not intended to be called via any other call chains.
     *
     * @param path  DBus::Object::Path to the D-Bus object to remove
     */
    void _destructObjectCallback(const Object::Path &path);

    /// glib2 callback function granted access to this private section,
    /// used to delete an Object::Base object via _destructObjectCallback()
    friend void glib2::Callbacks::_int_dbusobject_callback_destruct(void *this_ptr);

    /**
     *  The DBus::Features::IdleDetect::__idle_detector_thread() method
     *  will be running in a separate thread and need access to loop through
     *  all the available objects and check if they should be considered in the
     *  idle detection logic.
     *
     *  The IdleDetect() class is an internal object, not to be exposed to
     *  any external users.
     */
    friend class DBus::Features::IdleDetect;
};


} // namespace Object
} // namespace DBus
