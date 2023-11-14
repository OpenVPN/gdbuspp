//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

#pragma once

/**
 * @file features/idle-detect.hpp
 *
 * @brief  Declaration of DBus::Feature::IdleDetect.
 */

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <cstdint>
#include <thread>

#include "../mainloop.hpp"


namespace DBus {

namespace Object {
class Manager; // Forward declaration; see object/manager.hpp
}

namespace Features {

/**
 *  An IdleDetect object is given access to the DBus::Object::Manager
 *  and the DBus::MainLoop used by the DBus::Service object.  It will
 *  regularly check if there are any activity happening in the service and
 *  if there are DBus::Object::Base objects present.  If there are no
 *  activity and no active objects in the service, it will trigger a
 *  shutdown of the service.
 *
 *  The glib2::Callbacks will also have access to the IdleDetect object
 *  via the DBus::Object::CallbackLink, where it will request an activity update
 *
 *  If the IdleDetect object is set up in a DBus::Service object, it will be
 *  started when the service has been registered unto the D-Bus
 *  (see glib2::Callbacks::_int_callback_name_acquired() for details).
 *
 *  All the DBus::Object::Base objects can also disable the object tracking
 *  in the IdleDetect logic, by calling SkipIdleDetection(true).  This
 *  will make the IdleDetect logic exit the program when idling, even if
 *  this DBus::Object::Base object is active and valid.  D-Bus activity towards
 *  this object will still be tracked as activity.
 */
class IdleDetect
{
  public:
    using Ptr = std::shared_ptr<IdleDetect>;

    /**
     *  Prepare and set up the Idle Detection object.  This is used by the
     *  DBus::Service object only.
     *
     * @param mainloop            DBus::MainLoop of the program to control
     * @param timeout             std::chrono::duration<> of how long the idle
     *                            time the program is granted.
     * @param object_mgr          DBus::Object::Manager object, with the
     *                            objects to track activity on
     *
     * @return IdleDetect::Ptr
     */
    [[nodiscard]] static IdleDetect::Ptr Create(MainLoop::Ptr mainloop,
                                                std::chrono::duration<uint32_t> timeout,
                                                std::shared_ptr<Object::Manager> object_mgr)
    {
        return IdleDetect::Ptr(new IdleDetect(mainloop, object_mgr, timeout));
    }

    ~IdleDetect() noexcept;

    /**
     *  Start a thread running the Idle Detection checker logic
     *
     *  This internal worker thread will walk through all the registered
     *  objects in the assigned DBus::Object::Manager, check the "disable idle
     *  check" flag and see if this service has been idling or is holding data
     *  it cannot not release.  If no D-Bus objects needing to be preserved
     *  are in use, no method calls has been performed or no properties been
     *  accessed within the defined timeout duration, it will trigger a service
     *  shutdown
     */
    void Start();

    /**
     *  Stops the internal idle detection worker thread
     */
    void Stop();

    /**
     *  Updates the "last activity" timestamp.  This is used by the idle
     *  detector logic to see if this service has been idling too long without
     *  any acticvity.
     *
     *  This method is typically called by the callback functions in the
     *  glib2::Callbacks namespace.
     *
     */
    void ActivityUpdate() noexcept;


  private:
    /// Service main loop which will be shut-down when idling
    MainLoop::Ptr mainloop{nullptr};

    /// A shared pointer to the DBus::Object::Manager with the objects
    /// to inspect; some objects may be excluded when checking if the service
    /// is running idle.
    std::shared_ptr<Object::Manager> object_manager{nullptr};

    /// The configured idle timeout
    const std::chrono::duration<uint32_t> timeout;

    /// Is the idle detector running?
    bool running = false;

    /// Timestamp of the last time there was some D-Bus activity
    ///  in the service
    std::chrono::time_point<std::chrono::system_clock> last_event{};

    /// Condition variable mutex, used to exit the waiting loop if the
    /// service is shut down manually
    std::mutex stop_condvar_mtx{};
    std::condition_variable stop_condvar{};

    /// The worker thread running the idle detector logic
    std::unique_ptr<std::thread> worker{nullptr};

    /**
     *  Construct a new IdleDetect object

     * @param mainloop     DBus::MainLoop::Ptr to the main loop to control
     * @param object_mgr   DBus::Object::Manager::Ptr holding the D-Bus
     *                     objects of the service
     * @param timeout      std::chrono::duration of the idle timeout threshold
     */
    IdleDetect(MainLoop::Ptr mainloop,
               std::shared_ptr<Object::Manager> object_mgr,
               std::chrono::duration<uint32_t> timeout);


    /**
     *  The worker thread running in parallel to detect if the service is
     *  running idle or not.
     *
     * @return Returns true if the service should be shut down; otherwise false
     */
    bool __idle_detector_thread();
};


} // namespace Features
} // namespace DBus
