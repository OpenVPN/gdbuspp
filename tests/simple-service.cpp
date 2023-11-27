//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   simple-service.cpp
 *
 * @brief  A testing D-Bus service using the base functionality of the
 *         GDBus++ API.  This is required for the test suite run via
 *         the Meson build system.
 *
 */

#include <cstring>
#include <iostream>
#include <limits>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>

#include "../gdbuspp/mainloop.hpp"
#include "../gdbuspp/service.hpp"
#include "test-constants.hpp"


using namespace Test;


/**
 *  Very simple implementation of a simple logging mechanism.
 *
 *  This object can be passed to other D-Bus related objects to send
 *  D-Bus signal events with some log information when called.
 *
 *  This class and the DBus::Signals::Group class is designed to be
 *  shared across more users, through the ::Ptr (shared_ptr) to it.
 */
class SimpleLog : public DBus::Signals::Group
{
  public:
    using Ptr = std::shared_ptr<SimpleLog>;

    SimpleLog(DBus::Connection::Ptr conn)
        : DBus::Signals::Group(conn,
                               Constants::GenPath("simple1"),
                               Constants::GenInterface("simple1"))
    {
        RegisterSignal("Log", {{"method", "s"}, {"message", "s"}});
    }

    void Log(const std::string &info, const std::string &details)
    {
        try
        {
            GVariant *p = g_variant_new("(ss)", info.c_str(), details.c_str());
            SendGVariant("Log", p);
        }
        catch (const DBus::Signals::Exception &excp)
        {
            std::cerr << "EXCEPTION :: SimpleLog: " << excp.what() << std::endl;
        }
    }
};


/**
 *  Initialises a D-Bus service and registers the service unto the D-Bus
 *  given D-Bus connection.
 *
 *  Well-known bus name: net.openvpn.gdbuspp.test.simple
 */
class SimpleService : public DBus::Service
{
  public:
    SimpleService(DBus::Connection::Ptr con)
        : DBus::Service(con, Constants::GenServiceName("simple"))
    {
    }

    virtual ~SimpleService() = default;

    //  This is a callback method which is called when this D-Bus service
    //  is registered on the D-Bus connection
    void BusNameAcquired(GDBusConnection *conn, const std::string &busname) override
    {
        std::cout << "Bus name acquired: " << busname << std::endl;
    }

    //  This callback method will be called if the D-Bus service for some reason
    //  does not get the service registered on the D-Bus connection or the
    //  registration is lost (the D-Bus daemon kicks the service off the D-Bus).
    void BusNameLost(GDBusConnection *conn, const std::string &busname) override
    {
        std::cout << "** WARNING ** Bus name lost: " << busname << std::endl;
        Stop();
    }
};


/**
 *  A simple object created on-the-fly via the CreateSimpleObject D-Bus
 *  method in the gdbuspp.test.simple1 interface in the object path
 *  /gdbuspp/tests/simple1/methods
 *
 *  These objects are just used to test creating and destroying child objects
 *  dynamically and to ensure there are no memory leaks or crashes happening
 *  related to object management.
 *
 *  Path:      (set via constructor argument)
 *  Interface: gdbuspp.test.simple1.child
 */
class SimpleObject : public DBus::Object::Base
{
  public:
    SimpleObject(DBus::Object::Manager::Ptr obj_mgr,
                 const std::string &path,
                 const std::string &name)
        : DBus::Object::Base(path, Constants::GenInterface("simple1.child")),
          object_manager(obj_mgr), my_path(path), my_name(name)
    {
        AddProperty("my_path", my_path, false, "o");

        auto getmyname_args = AddMethod("GetMyName",
                                        [this](DBus::Object::Method::Arguments::Ptr args)
                                        {
                                            GVariant *ret = g_variant_new("(s)",
                                                                          this->my_name.c_str());
                                            args->SetMethodReturn(ret);
                                        });
        getmyname_args->AddOutput("name", "s");

        AddMethod("RemoveMe",
                  [this](DBus::Object::Method::Arguments::Ptr args)
                  {
                      std::cout << "---- REMOVING MYSELF: " << this->GetPath() << std::endl;

                      this->object_manager->RemoveObject(this->GetPath());
                      args->SetMethodReturn(nullptr);
                  });
    }

    virtual ~SimpleObject() noexcept
    {
        std::cout << "---- ~SimpleObject() ... path: " << my_path << std::endl;
    };


    const bool Authorize(const DBus::Authz::Request::Ptr req) override
    {
        return true;
    }


  private:
    DBus::Object::Manager::Ptr object_manager = nullptr;
    std::string my_path;
    const std::string my_name;
};



/**
 *  Separate object to test various property interfaces in
 *  DBus::Object::Property
 *
 *  Path: /gdbuspp/tests/properties
 *  Interface: gdbuspp.test.simple1
 */
class PropertyTests : public DBus::Object::Base
{
  public:
    PropertyTests(SimpleLog::Ptr log_)
        : DBus::Object::Base(Constants::GenPath("simple1/properties"),
                             Constants::GenInterface("simple1")),
          log(log_)
    {
        DisableIdleDetector(true);
        RegisterSignals(log);

        // Test simple data type based properties.  These D-Bus object properties
        // are tied to a local member variable of this class and will be updated
        // and read as a D-Bus caller reads or writes to these properties
        AddProperty("string_val", string_val, true);
        AddProperty("string_array", string_array, true);
        AddProperty("uint_val", uint_val, true);
        AddProperty("int_val", int_val, true);
        AddProperty("uint_array", uint_array, true);
        AddProperty("int_array", int_array, true);
        AddProperty("long_val", long_val, true);
        AddProperty("ulonglong_val", ulonglong_val, true);
        AddProperty("bool_val", bool_val, true);


        //  These two lambda functions wraps the call to methods in this
        //  object to read and update a more complex property data type.
        //
        //  In this implementation this is done through a classic struct
        //  approach, but it is up to the Get/Set methods to build up and
        //  extract the values as appropriate.  It does not need to be a
        //  struct by itself
        auto complex_get = [this](const DBus::Object::Property::BySpec &prop)
        {
            return this->GetComplexProperty(prop);
        };
        auto complex_set = [this](const DBus::Object::Property::BySpec &prop, GVariant *v)
        {
            return this->SetComplexProperty(prop, v);
        };
        AddPropertyBySpec("complex", "(bis)", complex_get, complex_set);
        AddPropertyBySpec("complex_readonly", "(bis)", complex_get);

        Log(__func__, "Initialized");
    }

    ~PropertyTests() noexcept
    {
        Log(__func__, "Removed PropertyTests");
        std::cout << __func__ << " -- ~PropertyTests() called" << std::endl;
    }


    const bool Authorize(const DBus::Authz::Request::Ptr request) override
    {
        std::ostringstream ar;
        ar << request;
        Log(__func__, "Authorization request approved: " + ar.str());
        return true;
    }

  private:
    SimpleLog::Ptr log = nullptr;

    // Property variables -- Property::SingleType
    std::string string_val = "Initial string";
    std::vector<std::string> string_array = {"line 1", "line 2", "line 3"};
    uint32_t uint_val = 123;
    int32_t int_val = -345;
    std::vector<uint32_t> uint_array = {0, 15, 16, 31, 32, 65534, 65535};
    std::vector<int> int_array = {-10, 3, 16, 9388};
    long long_val = std::numeric_limits<long>::max();
    unsigned long int ulonglong_val = std::numeric_limits<unsigned long int>::max();
    bool bool_val = true;

    // Property variables -- Property::BySpec (complex type)
    struct complex_t
    {
        bool boolval;
        int intval;
        std::string stringval;
    };

    complex_t complex = {
        .boolval = true,
        .intval = 98877,
        .stringval = "Initial complex string value"};


    GVariant *GetComplexProperty(const DBus::Object::Property::BySpec &property)
    {
        std::cout << "[Get Complex Property]"
                  << "name=" << property.GetName() << std::endl;
        GVariant *r = g_variant_new("(bis)",
                                    complex.boolval,
                                    complex.intval,
                                    complex.stringval.c_str());

        Log(__func__, "Complex Property retrieved");
        return r;
    }


    DBus::Object::Property::Update::Ptr SetComplexProperty(const DBus::Object::Property::BySpec &property,
                                                           GVariant *value)
    {
        glib2::Utils::checkParams(__func__, value, "(bis)", 3);

        complex.boolval = glib2::Value::Extract<bool>(value, 0);
        complex.intval = glib2::Value::Extract<int>(value, 1);
        complex.stringval = glib2::Value::Extract<std::string>(value, 2);

        std::cout << "[Set Complex Property]"
                  << "name=" << property.GetName() << ", "
                  << "bool=" << complex.boolval << ", "
                  << "int=" << complex.intval << ", "
                  << "string-2='" << complex.stringval << "'"
                  << std::endl;

        auto upd = property.PrepareUpdate();
        upd->AddValue(complex.boolval);
        upd->AddValue(complex.intval);
        upd->AddValue(complex.stringval);
        Log(__func__, "Complex Property updated");

        return upd;
    }

    void Log(const std::string &func, const std::string &msg) const
    {
        log->Log("PropertyTests::" + func, msg);
    }
};


/**
 *  Separate object to test various D-Bus methods with functionality which
 *  must succeed and work correctly
 *
 *  Path:      /gdbuspp/tests/methods
 *  Interface: gdbuspp.test.simple1
 */
class MethodTests : public DBus::Object::Base
{
  public:
    MethodTests(DBus::Object::Manager::Ptr obj_mgr_, SimpleLog::Ptr log_)
        : DBus::Object::Base(Constants::GenPath("simple1/methods"),
                             Constants::GenInterface("simple1")),
          object_manager(obj_mgr_), log(log_)
    {
        DisableIdleDetector(true);
        RegisterSignals(log);

        // Just a simple D-Bus method not requiring any input arguments nor
        // giving anything back to the caller.
        AddMethod("MethodNoArgs",
                  [](DBus::Object::Method::Arguments::Ptr args)
                  {
                      std::cout << "[Method call: MethodNoArgs] " << args << std::endl;
                      args->SetMethodReturn(nullptr);
                  });


        //  A simple string lenght function, taking a string as input
        //  and returning an integer back to the caller with the length of
        //  the input string
        auto stringlen_args = AddMethod("StringLength",
                                        [this](DBus::Object::Method::Arguments::Ptr args)
                                        {
                                            this->StringLength(args);
                                        });
        stringlen_args->AddInput("string", "s");
        stringlen_args->AddOutput("length", "i");


        //  This creates a new child object, as defined in the SimpleObject class
        //  The "name" of the object, becomes part of the D-Bus path which is
        //  given as an input string.  It returns the full D-Bus object path
        //  to this new object
        auto createobj_args = AddMethod("CreateSimpleObject",
                                        [this](DBus::Object::Method::Arguments::Ptr args)
                                        {
                                            this->CreateSimpleObject(args);
                                        });
        createobj_args->AddInput("name", "s");
        createobj_args->AddOutput("path", "o");


        //  This removes a child object based on the "name" of the object, as
        //  given when creating it.
        //
        //  This approach deletes the object without the object itself being
        //  involved in the deletion, but the destructor should be called.
        //  The SimpleObject also implements a method where it can delete
        //  itself as well.
        auto rmobj_args = AddMethod("RemoveSimpleObject",
                                    [this](DBus::Object::Method::Arguments::Ptr args)
                                    {
                                        this->RemoveSimpleObject(args);
                                    });
        rmobj_args->AddInput("name", "s");


        //  This method will open a file with the file name provided as an
        //  input argument and will return a file descriptor to the opened file.
        //  This is for testing sending file descriptors from the service to the
        //  caller of this method.
        auto openfile_args = AddMethod("OpenFilePassFD",
                                       [this](DBus::Object::Method::Arguments::Ptr args)
                                       {
                                           this->OpenFile(args);
                                       });
        openfile_args->AddInput("file", "s");
        openfile_args->AddOutput("success", "b");
        openfile_args->PassFileDescriptor(DBus::Object::Method::PassFDmode::SEND);

        //  This method will receive a file descriptor to a file from the caller
        //  and this method will do a fstat() call on that file descriptor and
        //  extract a few details it will send back to the caller.
        //  This is for testing receiving file descriptors via a D-Bus
        //  method call
        auto fstat_args = AddMethod("fstatFileFromFD",
                                    [this](DBus::Object::Method::Arguments::Ptr args)
                                    {
                                        this->fstatFile(args);
                                    });
        fstat_args->AddOutput("owner_uid", "u");
        fstat_args->AddOutput("owner_gid", "u");
        fstat_args->AddOutput("size", "t");
        fstat_args->PassFileDescriptor(DBus::Object::Method::PassFDmode::RECEIVE);

        Log(__func__, "Initialized");
    }

    ~MethodTests() noexcept
    {
        Log(__func__, "Removed MethodTests");
        std::cout << __func__ << " -- ~MethodTests() called" << std::endl;
    }

    const bool Authorize(const DBus::Authz::Request::Ptr request) override
    {
        std::ostringstream ar;
        ar << request;
        Log(__func__, "Authorization request approved: " + ar.str());
        return true;
    }

  private:
    DBus::Object::Manager::Ptr object_manager = nullptr;
    SimpleLog::Ptr log = nullptr;


    void Log(const std::string &func, const std::string &msg) const
    {
        log->Log("MethodTests::" + func, msg);
    }


    void StringLength(DBus::Object::Method::Arguments::Ptr args)
    {
        Log(__func__, "StringLength called");
        std::cout << "[StringLength call] " << args << std::endl;

        GVariant *params = args->GetMethodParameters();
        std::string in = glib2::Value::Extract<std::string>(params, 0);
        size_t l = in.length();
        std::cout << "Input: '" << in << "'  "
                  << "length: " << std::to_string(l) << std::endl;

        args->SetMethodReturn(g_variant_new("(i)", l));
    }


    void CreateSimpleObject(DBus::Object::Method::Arguments::Ptr args)
    {
        std::cout << "[CreateSimpleObject call] " << args << std::endl;

        GVariant *params = args->GetMethodParameters();
        std::string name = glib2::Value::Extract<std::string>(params, 0);

        const std::string child_path = Constants::GenPath("simple1/childs/") + name;
        auto obj = object_manager->CreateObject<SimpleObject>(object_manager,
                                                              child_path,
                                                              name);
        std::cout << ">>>> NEW OBJECT: " << obj->GetPath() << std::endl;
        Log(__func__, "New child object created: " + obj->GetPath());

        args->SetMethodReturn(g_variant_new("(o)", obj->GetPath().c_str()));
    }


    void RemoveSimpleObject(DBus::Object::Method::Arguments::Ptr args)
    {
        std::cout << "[RemoveSimpleObject call] " << args << std::endl;

        GVariant *params = args->GetMethodParameters();
        std::string name = glib2::Value::Extract<std::string>(params, 0);
        if (name.empty() || "/" == name)
        {
            throw DBus::Object::Exception("Path cannot be empty");
        }

        const std::string path = Constants::GenPath("simple1/childs/") + name;
        object_manager->RemoveObject(path);
        std::cout << ">>> DELETED OBJECT: " << path << std::endl;
        Log(__func__, "Child object removed: " + path);

        args->SetMethodReturn(nullptr);
    }



    void OpenFile(DBus::Object::Method::Arguments::Ptr args)
    {
        std::cout << "[OpenFile - Read] " << args << std::endl;

        GVariant *params = args->GetMethodParameters();
        std::string fname = glib2::Value::Extract<std::string>(params, 0);

        int fd = -1;
        try
        {
            fd = open(fname.c_str(), O_RDONLY);
            args->SendFD(fd);
            Log(__func__, "File '" + fname + "' opened, fd=" + std::to_string(fd));
        }
        catch (const DBus::AsyncProcess::Exception &excp)
        {
            Log(__func__, "** ERROR **  " + std::string(excp.GetRawError()));
            std::cerr << "** ERROR **  " << excp.what() << std::endl;
        }

        args->SetMethodReturn(g_variant_new("(b)", (fd > 0)));
    }


    void fstatFile(DBus::Object::Method::Arguments::Ptr args)
    {
        std::cout << "[fstatFile] " << args << std::endl;

        int fd = args->ReceiveFD();
        struct stat fileinfo = {};
        GVariant *ret = nullptr;
        if (fstat(fd, &fileinfo) >= 0)
        {
            std::cout << "fstat() success: "
                      << "uid=" << std::to_string(fileinfo.st_uid) << ", "
                      << "gid=" << std::to_string(fileinfo.st_gid) << ", "
                      << "size=" << std::to_string(fileinfo.st_size)
                      << std::endl;
            ret = g_variant_new("(uut)", fileinfo.st_uid, fileinfo.st_gid, fileinfo.st_size);
            Log(__func__, "fstat(" + std::to_string(fd) + "') successful");
        }
        else
        {
            Log(__func__, "fstat(" + std::to_string(fd) + "') failed!");
            throw DBus::Object::Exception(this,
                                          "fstat() call failed: "
                                              + std::string(std::strerror(errno)));
        }

        args->SetMethodReturn(ret);
    }
};


/**
 *  These D-Bus methods in this D-Bus object is expected to all fail
 *
 *  This is used to test the error handling and how errors are reported back
 *  to the caller as well as ensuring the service itself does not crash
 *
 *  Path:      /gdbuspp/tests/method_failures
 *  Interface: gdbuspp.test.simple1
 */
class FailingMethodTests : public DBus::Object::Base
{
  public:
    FailingMethodTests()
        : DBus::Object::Base(Constants::GenPath("simple1/method_failures"),
                             Constants::GenInterface("simple1"))
    {
        DisableIdleDetector(true);
        auto wrong_args1 = AddMethod("NoReceive123",
                                     [](DBus::Object::Method::Arguments::Ptr args)
                                     {
                                         args->SetMethodReturn(g_variant_new("(i)", 123));
                                     });
        wrong_args1->AddOutput("length", "s");

        auto wrong_args2 = AddMethod("InputMismatch",
                                     [](DBus::Object::Method::Arguments::Ptr args)
                                     {
                                         GVariant *p = args->GetMethodParameters();
                                         glib2::Utils::checkParams(__func__, p, "(i)", 1);
                                         (void)glib2::Value::Extract<int>(p, 0);
                                         args->SetMethodReturn(g_variant_new("(b)", false));
                                     });
        wrong_args2->AddInput("not_an_int", "s");
        wrong_args2->AddOutput("failed", "b");

        auto wrong_args3 = AddMethod("OutputMismatch",
                                     [](DBus::Object::Method::Arguments::Ptr args)
                                     {
                                         args->SetMethodReturn(g_variant_new("(i)", 123));
                                     });
        wrong_args3->AddOutput("failed", "b");


        AddMethod("NoReceiveFD",
                  [](DBus::Object::Method::Arguments::Ptr args)
                  {
                      args->ReceiveFD();
                      args->SetMethodReturn(nullptr);
                  });


        auto no_send_fd = AddMethod("NoSendFD",
                                    [](DBus::Object::Method::Arguments::Ptr args)
                                    {
                                        int fd = open("/dev/null", O_RDONLY);
                                        args->SendFD(fd);
                                        args->SetMethodReturn(nullptr);
                                    });
        no_send_fd->PassFileDescriptor(DBus::Object::Method::PassFDmode::RECEIVE);
    }

    const bool Authorize(const DBus::Authz::Request::Ptr request) override
    {
        return true;
    }
};


class SimpleHandler : public DBus::Object::Base
{
  public:
    SimpleHandler(SimpleService::Ptr service)
        : DBus::Object::Base(Constants::GenPath("simple1"),
                             Constants::GenInterface("simple1"))
    {
        DisableIdleDetector(true);
        log = DBus::Signals::Group::Create<SimpleLog>(service->GetConnection());
        RegisterSignals(log);
        log->AddTarget("");

        auto object_mgr = service->GetObjectManager();
        property_tests = object_mgr->CreateObject<PropertyTests>(log);
        method_tests = object_mgr->CreateObject<MethodTests>(object_mgr, log);
        failing_meths = object_mgr->CreateObject<FailingMethodTests>();

        AddProperty("version", version, false);

        log->Log("SimpleHandler", "Handler is initialized");
    }


    ~SimpleHandler()
    {
        try
        {
            log->Log("SimpleHandler", "Handler is shutting down");
        }
        catch (const DBus::Exception &excp)
        {
            std::cerr << "** ERROR **  Failed to send Log signal: "
                      << excp.what() << std::endl;
        }

        std::cerr << std::endl
                  << "DESTRUCTOR ~SimpleHandler" << std::endl;
    }


    const bool Authorize(const DBus::Authz::Request::Ptr request) override
    {
        std::ostringstream ar;
        ar << request;
        log->Log("SimpleHandler::Authorize", "Authorization request approved: " + ar.str());
        return true;
    }


  private:
    SimpleLog::Ptr log = nullptr;
    PropertyTests::Ptr property_tests = nullptr;
    MethodTests::Ptr method_tests = nullptr;
    FailingMethodTests::Ptr failing_meths = nullptr;
    std::string version = "0.1.2.3";
};


int main(int argc, char **argv)
{
    try
    {
        // Create a connection to the session D-Bus
        auto dbuscon = DBus::Connection::Create(DBus::BusType::SESSION);

        // Create a new service object - SimpleService
        auto simple_service = DBus::Service::Create<SimpleService>(dbuscon);

        // Create a new "root object", handling all the initial requests
        // This root object is the ServiceHandler; which can create child
        // objects with different functionality
        simple_service->CreateServiceHandler<SimpleHandler>(simple_service);

        simple_service->PrepareIdleDetector(std::chrono::seconds(60));

        // Start the service
        simple_service->Run();
    }
    catch (const DBus::Exception &excp)
    {
        std::cout << "EXCEPTION (DBus): " << excp.what() << std::endl;
        return 9;
    }
    catch (const std::exception &excp)
    {
        std::cout << "EXCEPTION: " << excp.what() << std::endl;
        return 8;
    }
    return 0;
}
