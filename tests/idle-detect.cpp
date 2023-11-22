//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file tests/idle-detect.cpp
 *
 * @brief  Simple test of the DBus::Feature::IdleDetect implementation
 *         This will only run some simple tests checking the detection logic
 *         is working.  It is NOT a complete D-Bus service implementation test
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>

#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/mainloop.hpp"
#include "../gdbuspp/object/base.hpp"
#include "../gdbuspp/object/manager.hpp"



class ChildObject : public DBus::Object::Base
{
  public:
    using Ptr = std::shared_ptr<ChildObject>;

    ChildObject(const std::string &name, const std::string &interf)
        : DBus::Object::Base("/tests/idle/childs/" + name, interf)
    {
    }

    const bool Authorize(const DBus::Authz::Request::Ptr req) override
    {
        return true;
    }
};


class IdleCheckDisabled : public DBus::Object::Base
{
  public:
    IdleCheckDisabled(DBus::Object::Manager::Ptr objmgr = nullptr)
        : DBus::Object::Base("/tests/idle/disabled", "test.idle.disabled"),
          object_mgr(objmgr)
    {
        DisableIdleDetector(true);
    }

    const bool Authorize(const DBus::Authz::Request::Ptr req) override
    {
        return true;
    }

    auto CreateChild(const std::string &name)
    {
        if (!object_mgr)
        {
            throw DBus::Object::Exception(this, "No object manager available");
        }
        return object_mgr->CreateObject<ChildObject>(name, GetInterface());
    }

  private:
    DBus::Object::Manager::Ptr object_mgr{nullptr};
};


class IdleCheckEnabled : public DBus::Object::Base
{
  public:
    IdleCheckEnabled()
        : DBus::Object::Base("/tests/idle/enabled", "test.idle.enabled")
    {
        DisableIdleDetector(false);
    }

    const bool Authorize(const DBus::Authz::Request::Ptr req) override
    {
        return true;
    }
};


/**
 *  Very simple conversion of a std::chrono::time_point to std::string
 *
 * @param tp     std::chrono::time_point to convert
 * @return const std::string containing the time point as plain text
 */
const std::string conv_tstamp(std::chrono::time_point<std::chrono::system_clock> &tp)
{
    std::time_t ttp = std::chrono::system_clock::to_time_t(tp);
    return std::string(std::ctime(&ttp));
}


uint32_t success = 0; ///< Global: Count of successfully completed tests
uint32_t failed = 0;  ///< Global: Count of failed tests

/**
 *  Simple wrapper function which runs a lambda and calculates the runtime
 *  and does a simple evealuation based on an expected runtime with a +/-
 *  deviation
 *
 * @tparam FUNC        type template for the function
 * @param descr        std::string with a short test description
 * @param runtime_sec  uint32_t with the expected runtime
 * @param deviation    uint32_t of the allowed +/- deviation in runtime
 * @param testfunc     Lambda function containing the code to run
 */
template <typename FUNC>
void time_execution(const std::string &descr,
                    const uint32_t runtime_sec,
                    const uint32_t deviation,
                    FUNC &&testfunc)
{
    auto start = std::chrono::system_clock::now();
    bool exception = false;
    std::cout << "::: " << descr << ":    start=" << conv_tstamp(start);
    try
    {
        testfunc();
    }
    catch (const std::exception &excp)
    {
        std::cout << "::: " << descr << ": EXCEPTION ## " << excp.what() << std::endl;
        exception = true;
    }
    auto end = std::chrono::system_clock::now();
    std::cout << "::: " << descr << ":      end=" << conv_tstamp(end);

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "::: " << descr << ": duration=" << duration.count() << "s, ";
    if (deviation == 0)
    {
        std::cout << "expected=" << runtime_sec << "s";
    }
    else
    {
        std::cout << "expected " << runtime_sec - deviation << "s "
                  << "to " << runtime_sec + deviation << "s";
    }
    std::cout << " ==> result=";


    bool runtime_pass = (duration.count() >= runtime_sec - deviation)
                        && (duration.count() <= runtime_sec + deviation);
    if (!exception && runtime_pass)
    {
        std::cout << "Pass" << std::endl;
        ++success;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
        ++failed;
    }
}


/// Global: A single additional thread, used to run a some code after
/// a certain time.
std::shared_ptr<std::thread> delayed_run_thread{nullptr};

/**
 *  Run some code after a given time.  Used to stop a main loop out of the
 *  ordinary time_execution() scope.
 *
 * @tparam FUNC      type template for the function to run
 * @param after_sec  Run the lambda function after sleeping for after_sec seconds
 * @param func       The lambda function to run
 */
template <typename FUNC>
void delayed_execution(const int after_sec,
                       FUNC &&func)
{
    delayed_run_thread.reset(
        new std::thread([after_sec, func]()
                        {
                            sleep(after_sec);
                            func();
                        }));
}


/**
 *  std::thread cleanup helper, waits for the thread kicked off by
 *  delayed_execution() and resets the delayed_run_thread pointer.
 */
void delayed_join()
{
    if (delayed_run_thread && delayed_run_thread->joinable())
    {
        delayed_run_thread->join();
    }
    delayed_run_thread.reset();
}



int main(int argc, char **argv)
{
    // First simple test; just have an empty object manager.  It is expected
    // to shutdown automatically after 3 seconds.
    time_execution("No objects", 3, 0, []()
                   {
                       auto buscon = DBus::Connection::Create(DBus::BusType::SESSION);
                       auto mainloop = DBus::MainLoop::Create();
                       auto objmgr = DBus::Object::Manager::CreateManager(buscon);
                       objmgr->PrepareIdleDetector(std::chrono::seconds(3), mainloop);
                       objmgr->RunIdleDetector(true);
                       mainloop->Run();
                   });

    // A similar test to the test above, but here a DBus::Object::Base object
    // has been created via the DBus::Object::Manager.  This object has
    // disabled the idle detection, which means the presence of this object
    // will not be considered by the idle detector.  Thus, since the timeout
    // is set to 2 seconds in this test, the expected runtime is 2 seconds.
    time_execution("IdleCheckDisabled", 2, 0, []()
                   {
                       auto buscon = DBus::Connection::Create(DBus::BusType::SESSION);
                       auto mainloop = DBus::MainLoop::Create();
                       auto objmgr = DBus::Object::Manager::CreateManager(buscon);
                       objmgr->CreateObject<IdleCheckDisabled>();
                       objmgr->PrepareIdleDetector(std::chrono::seconds(2), mainloop);
                       objmgr->RunIdleDetector(true);
                       mainloop->Run();
                       objmgr->RemoveObject("/tests/idle/disabled");
                   });


    // This is the same test as above, but here the idle detector is considering
    // the presence of the IdleCheckEnabled object.  As long as this object
    // exists, it will continue to run.  The delayed execution will forcefully
    // stop the main loop after the idle detector should has run at least 2 times.
    // The idle timeout is set to 2 seconds and the delayed forceful shutdown at
    // 5 seconds, thus the expected runtime is roughly 5 seconds.
    time_execution("IdleCheckEnabled", 5, 0, []()
                   {
                       auto buscon = DBus::Connection::Create(DBus::BusType::SESSION);
                       auto mainloop = DBus::MainLoop::Create();
                       auto objmgr = DBus::Object::Manager::CreateManager(buscon);
                       objmgr->CreateObject<IdleCheckEnabled>();
                       objmgr->PrepareIdleDetector(std::chrono::seconds(2), mainloop);
                       delayed_execution(5, [mainloop]()
                                         {
                                             try
                                             {
                                                 mainloop->Stop();
                                             }
                                             catch (const DBus::MainLoop::Exception &)
                                             {
                                             }
                                         });
                       objmgr->RunIdleDetector(true);
                       mainloop->Run();
                       delayed_join();
                       objmgr->RemoveObject("/tests/idle/enabled");
                   });


    // This is an extension of the previous "IdleCheckEnabled", using the
    // the "IdleCheckDisabled" with a child object active and then removed.
    //
    // The "IdleCheckDisabled" object will not be considered, but the child
    // object has not disabled the idle detector.  This means that as long as
    // there is a child object present, the main loop will be running.
    //
    // The first test in this block will have the IdleCheckDisabled and the
    // ChildObject present.  The idle detection is set at 2 seconds but the
    // delayed execution stopping the main loop is set at 5 seconds, thus
    // this step will take 5 seconds to run.
    //
    // The next step removes the ChildObject and then re-runs the main loop.
    // Now the expected runtime is 2 seconds - as there are no objects needed
    // to be considered for the idle detector.
    //
    // Finally, we measure the time it takes before the delayed_execution()
    // thread stops.  The measurement here isn't too precise,so it is expected
    // to run roughly 2-4 seconds.  With a total expected runtime of 5 seconds,
    // the 2-4 seconds of waiting + the runtime of 2 seconds for ChildObject-2,
    // that is roughly 4-6 seconds of runtime - which is close enough.
    //
    {
        auto mainloop = DBus::MainLoop::Create();

        auto buscon = DBus::Connection::Create(DBus::BusType::SESSION);
        auto objmgr = DBus::Object::Manager::CreateManager(buscon);
        objmgr->PrepareIdleDetector(std::chrono::seconds(2), mainloop);

        auto rootobj = objmgr->CreateObject<IdleCheckDisabled>(objmgr);
        auto child = objmgr->CreateObject<ChildObject>("test1", rootobj->GetInterface());

        time_execution("ChildObject-1", 5, 0, [mainloop, objmgr]()
                       {
                           delayed_execution(5, [mainloop]()
                                             {
                                                 try
                                                 {
                                                     mainloop->Stop();
                                                 }
                                                 catch (const DBus::MainLoop::Exception &)
                                                 {
                                                 }
                                             });
                           objmgr->RunIdleDetector(true);
                           mainloop->Run();
                           delayed_join();
                           objmgr->RunIdleDetector(false);
                       });
        objmgr->RemoveObject(child->GetPath());

        delayed_execution(5, [mainloop]()
                          {
                              try
                              {
                                  mainloop->Stop();
                              }
                              catch (const DBus::MainLoop::Exception &)
                              {
                              }
                          });
        time_execution("ChildObject-2", 2, 0, [mainloop, objmgr]()
                       {
                           objmgr->RunIdleDetector(true);
                           mainloop->Run();
                       });
        time_execution("ChildObject-cleanup", 3, 1, []()
                       {
                           delayed_join();
                       });
    }

    // Summarize the test results
    std::cout << ">> Passed tests: " << std::to_string(success) << std::endl
              << ">> Failed tests: " << std::to_string(failed) << std::endl
              << ">> OVERALL TEST RESULT: "
              << (success > 0 && failed == 0 ? "PASS" : "FAIL")
              << std::endl;
    return (success > 0 && failed == 0) ? 0 : 2;
}
