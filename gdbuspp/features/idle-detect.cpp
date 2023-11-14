//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file features/idle-detect.cpp
 *
 * @brief  Implementation of DBus::Feature::IdleDetect.
 */

#include <ctime>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "../mainloop.hpp"
#include "../object/callbacklink.hpp"
#include "../object/manager.hpp"
#include "idle-detect.hpp"


namespace DBus {
namespace Features {


IdleDetect::~IdleDetect() noexcept
{
    if (worker && worker->joinable())
    {
        worker->join();
        worker.reset();
    }
}


void IdleDetect::Start()
{
    if (worker)
    {
        throw DBus::Exception("IdleDetect", "Idle detector is already running");
    }

    if (std::chrono::duration<uint32_t>(0) == timeout)
    {
        // Timeout set to 0, which disables this feature
        return;
    }

    worker.reset(new std::thread([this]()
                                 {
                                     if (this->__idle_detector_thread())
                                     {
                                         try
                                         {
                                             this->mainloop->Stop();
                                         }
                                         catch (const DBus::MainLoop::Exception &)
                                         {
                                         }
                                     }
                                 }));
}


void IdleDetect::Stop()
{
    if (running)
    {
        running = false;
        stop_condvar.notify_all();
        if (worker && worker->joinable())
        {
            worker->join();
            worker.reset();
        }
    }
}

void IdleDetect::ActivityUpdate() noexcept
{
    last_event = std::chrono::system_clock::now();
}


IdleDetect::IdleDetect(MainLoop::Ptr mainloop_,
                       Object::Manager::Ptr object_mgr_,
                       std::chrono::duration<uint32_t> timeout_)
    : mainloop(mainloop_), object_manager(object_mgr_), timeout(timeout_)
{
}


bool IdleDetect::__idle_detector_thread()
{
    running = true;
    while (running)
    {
        std::unique_lock<std::mutex> stop_lock(stop_condvar_mtx);
        stop_condvar.wait_for(stop_lock,
                              timeout,
                              [this]()
                              {
                                  return !this->running;
                              });

        if (running && std::chrono::system_clock::now() > (last_event + timeout))
        {
            uint32_t active = 0;
            for (const auto &it : object_manager->object_map)
            {
                Object::Base::Ptr obj = it.second->object;
                if (obj->GetIdleDetectorDisabled())
                {
                    continue;
                }
                ++active;
            }
            if (active == 0)
            {
                running = false;
                return true;
            }
        }
    }
    return false;
}


} // namespace Features
} // namespace DBus
