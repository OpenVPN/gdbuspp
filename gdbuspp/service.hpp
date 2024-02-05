//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

#pragma once

/**
 * @file service.hpp
 *
 * @brief  Declaration of DBus::Service.  This is a class to help build
 *         applications providing a service on the D-Bus.
 */

#include <cstdint>
#include <chrono>
#include <string>
#include <glib.h>

#include "exceptions.hpp"
#include "mainloop.hpp"
#include "object/manager.hpp"


namespace DBus {

/**
 * Main class to setup a new D-Bus service capable of containing multiple
 * DBus::Object objects and providing access to them via the system's D-Bus
 * daemon
 *
 * The DBus::Service object is not intended to be used directly, but via
 * a service implementation class inheriting from this class.
 *
 *  @code
 *
 *   class MyService : public DBus::Service
 *   {
 *     public:
 *       MyService(DBus::Connection::Ptr con)
 *           : DBus::Service(con, "net.example.myservice")
 *       {
 *       }
 *       // ....
 *    };
 *
 *    int main()
 *    {
 *         auto connection = DBus::Connection::Create(DBus::BusType::SESSION);
 *         auto my_service = DBus::Service::Create<MyService>(connection);
 *         // ....
 *    }
 *
 *  @endcode
 */
class Service : public std::enable_shared_from_this<Service>
{
  public:
    using Ptr = std::shared_ptr<Service>;

    class Exception : public DBus::Exception
    {
      public:
        Exception(const std::string &err, GError *gliberr = nullptr);
        virtual ~Exception() = default;
    };

    /**
     *  Prepare a new  D-Bus service with a given well-known bus name.
     *
     * @tparam C   Class name of the object to create.
     *             This must inherit from DBus::Service
     * @tparam T   Template argument for the arguments passed to the
     *             class C constructor
     * @param all  All the argument list used with the constructor
     *
     * @return C::Ptr  Returns a std::shared_ptr<C> to the newly
     *         created object
     */
    template <typename C, typename... T>
    static std::shared_ptr<C> Create(T &&...all)
    {
        return std::shared_ptr<C>(new C(std::forward<T>(all)...));
    }

    virtual ~Service() noexcept;


    /**
     *  All D-Bus services need a "root object" which will manage the
     *  other D-Bus objects in a service.
     *
     *  This method takes a template class which is the class implementation
     *  which must be based on the DBus::Object::Base class.  This will be
     *  the main handler object.  When calling this method, it will pass
     *  this information on to the Object Manager which will instantiate and
     *  own this service handler object for the life time of the service
     *  process.
     *
     *  @code
     *
     *     class MyHandler : public DBus::Object::Base
     *     {
     *          MyHandler(int arg1, int arg2, int arg3)
     *             : DBus::Object::Base("/example/myobject",
     *                                  "net.example.myinterface")
     *          {
     *          }
     *
     *     int main()
     *     {
     *         // ...
     *         my_service->CreateServiceHandler<MyHandler>(arg1, arg2, arg3);
     *         // ....
     *     }
     *
     *  @endcode
     *
     * @tparam C     DBus::Object::Base based class implementing
     *               the service root object
     * @tparam Args  Argument template for arguments to be passed on to
     *               the service handler contstructor
     * @param args   All the arguments required by the service handler
     *               constructor
     */
    template <class C, typename... Args>
    std::shared_ptr<C> CreateServiceHandler(Args... args)
    {
        return object_manager->CreateObject<C>(std::forward<Args>(args)...);
    }


    /**
     *  Retrieve a pointer to the DBus::Connection object used by this
     *  service object
     *
     * @return Connection::Ptr
     */
    Connection::Ptr GetConnection() const noexcept;

    /**
     *  Retrieve the Object::Manager used by this service object.
     *  This is helpful to allow classes managed by this service to
     *  add and remove D-Bus objects directly
     *
     * @return Object::Manager::Ptr
     */
    Object::Manager::Ptr GetObjectManager() const noexcept;


    /**
     *  Activate the idle-exit mechanism.  If no objects are active in the
     *  service, the service will shutdown after the given timeout value.
     *
     *  The service handler object is always excluded, but DBus::Object::Base
     *  objects can control if it should be checked via the
     *  DBus::Object::Base::DisableIdleDetector() method.
     *
     * @param timeout  std::chrono::duration describing the idle timeout
     *                 of the service
     */
    void PrepareIdleDetector(const std::chrono::duration<uint32_t> timeout);


    /**
     *  Control if the idle detector should run or not.
     *
     *  This is typically called by glib2::Callbacks::_int_callback_name_acquired()
     *  and glib2::Callbacks::_int_callback_name_lost() which starts and
     *  stops the idle detector when the service is registered and available on
     *  the D-Bus.
     *
     * @param run   bool flag to start or stop the Features::IdleDetect
     *              functionality
     */
    void RunIdleDetector(const bool run);


    /**
     *  Very simple DBus::MainLoop to get a D-Bus service running
     *
     *  For more advanced main loops, setting up a specialized
     *  DBus::MainLoop will work fine - and in that case, this method
     *  should not be used.
     */
    void Run();

    /**
     *  Stops the DBus::MainLoop managed by this object
     *
     *  Can be called by another thread, to stop the service main loop
     */
    void Stop();

    /**
     *  Called when the requested bus name has been successfully acquired.
     *
     *  @param GDBusConnection*  glib2 connection object to the bus of event
     *  @param busname           std::string of the registered bus name
     */
    virtual void BusNameAcquired(GDBusConnection *conn, const std::string &busname) = 0;

    /**
     *  Called if the bus name could not be acquired or was lost.  Either
     *  this or @BusNameAcquired will be called.
     *
     *  If the program keeps running after this call, it may result in
     *  gaining the bus name later on; in which the @BusNameAcquired() method
     *  will be called.
     *
     *  @param GDBusConnection*  glib2 connection object to the bus of event
     *  @param busname           std::string of the registered bus name
     */
    virtual void BusNameLost(GDBusConnection *conn, const std::string &busname) = 0;


  protected:
    /**
     *  Constructor for the DBus::Service part of the class inheritance
     *
     * @param busc      DBus::Connection::Ptr to use for this service.
     * @param busname   std::string containing the well-known bus name it
     *                  will be visible as on the D-Bus
     */
    Service(Connection::Ptr busc, const std::string &busname);


  private:
    ///  D-Bus connection object
    Connection::Ptr buscon;

    /// Well-known D-Bus name for this service
    std::string busname{};

    /// Bus ID reference assigned by the glib2 GDBus interface
    unsigned int busid = 0;

    /**
     *  The ObjectManager keeps track of all the D-Bus objects created
     *  by this D-Bus service and links them to the appropriate C++
     *  objects.  When the object is removed from the D-Bus, the C++ object
     *  is also removed automatically.
     */
    Object::Manager::Ptr object_manager;


    /**
     *  MainLoop object, only used if the service is started via this
     *  service object
     */
    DBus::MainLoop::Ptr service_mainloop = nullptr;


    //
    // Internal - private methods
    //

    /**
     *  Calls the appropriate glib2 GDBus functions to register a new
     *  service on the D-Bus.  Requires a valid DBus::Connection
     */
    void service_register();

    /**
     *  Calls the right glib2 GDBus functions to remove this service
     *  from the D-Bus.
     */
    void service_unregister() noexcept;
};
} // namespace DBus
