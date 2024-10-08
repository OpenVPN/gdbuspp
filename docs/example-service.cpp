//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   example-service.cpp
 *
 * @brief  Simple example service.
 *         This example code is referenced to from the README.md document.
 *
 *         To compile this program:
 *
 *  $ c++ -std=c++17 $(pkg-config gdbuspp glib-2.0 gio-2.0 gio-unix-2.0 --cflags --libs) -o example-service example-service.cpp
 *
 */

#include <iostream>
#include <string>
#include <gdbuspp/service.hpp>
#include <gdbuspp/glib2/utils.hpp>



class MySignalGroup : public DBus::Signals::Group
{
  public:
    using Ptr = std::shared_ptr<MySignalGroup>;

    MySignalGroup(DBus::Connection::Ptr connection)
        : DBus::Signals::Group(connection,
                               "/example/myobject",
                               "net.example.myinterface")
    {
        RegisterSignal("MySignal",
                       {{"message", glib2::DataType::DBus<std::string>()}});
    }


    void MySignal(const std::string &message_content)
    {
        // NOTE: D-Bus signals are picky on the data type.  Signals
        // must always be sent as a tuple, even if it's just a single value
        GVariant *message = glib2::Value::CreateTupleWrapped(message_content);
        SendGVariant("MySignal", message);
    }
};



class MyObject : public DBus::Object::Base
{
  public:
    MyObject(DBus::Connection::Ptr connection, DBus::Object::Manager::Ptr obj_mgr)
        : DBus::Object::Base("/example/myobject",
                             "net.example.myinterface"),
          object_manager(obj_mgr)
    {
        // Declaring D-Bus methods
        AddMethod("MyMethod",
                  [](DBus::Object::Method::Arguments::Ptr args)
                  {
                      // Code performed on access
                      std::cout << "MyMethod called: " << args << std::endl;

                      // We have no data to return; so we return nullptr
                      args->SetMethodReturn(nullptr);
                  });

        auto args = AddMethod("MethodWithArgs",
                              [](DBus::Object::Method::Arguments::Ptr args)
                              {
                                  // Extract input arguments
                                  GVariant *params = args->GetMethodParameters();
                                  std::string string1 = glib2::Value::Extract<std::string>(params, 0);
                                  std::string string2 = glib2::Value::Extract<std::string>(params, 1);

                                  // Prepare the response to the caller
                                  std::string result = string1 + " <=> " + string2;
                                  GVariant *ret = glib2::Value::CreateTupleWrapped(result);
                                  args->SetMethodReturn(ret);
                              });
        args->AddInput("string_1", glib2::DataType::DBus<std::string>());
        args->AddInput("string_2", glib2::DataType::DBus<std::string>());
        args->AddOutput("result", glib2::DataType::DBus<std::string>());

        // Declaring a D-Bus property
        bool readwrite = true;
        AddProperty("my_property", my_property, readwrite);

        // Declaring a D-Bus signal
        my_signals = DBus::Signals::Group::Create<MySignalGroup>(connection);
        my_signals->AddTarget("");
        RegisterSignals(my_signals);
    }


    const bool Authorize(const DBus::Authz::Request::Ptr request) override
    {
        // code to authorize a D-Bus proxy client accessing this object

        // Send a simple D-Bus signal whenever this is called
        my_signals->MySignal("Authorize called");

        // By returning 'true', access is granted
        return true;
    }


  private:
    DBus::Object::Manager::Ptr object_manager = nullptr;
    std::string my_property{"My Initial Value"};
    MySignalGroup::Ptr my_signals = nullptr;
};



class MyService : public DBus::Service
{
  public:
    MyService(DBus::Connection::Ptr con)
        : DBus::Service(con, "net.example.myservice")
    {
    }

    void BusNameAcquired(const std::string &busname) override
    {
        // Code run when the name of your service is accepted
        std::cout << "Service registered: " << busname << std::endl;
    }

    void BusNameLost(const std::string &busname) override
    {
        // Code run when your service could not get or keep the bus name
        std::cout << "Could not acquire the bus name: " << busname << std::endl;

        // Here we just stop and exit the service
        DBus::Service::Stop();
    }
};



int main(int argc, char **argv)
{
    try
    {
        // Prepare a connection to the Session D-Bus
        auto connection = DBus::Connection::Create(DBus::BusType::SESSION);

        // Instantiate and prepare the D-Bus service provided
        // via the MyService class
        auto my_service = DBus::Service::Create<MyService>(connection);

        // Get access to the DBus::Object::Manager object
        auto object_manager = my_service->GetObjectManager();

        // Add the service root object
        my_service->CreateServiceHandler<MyObject>(connection, object_manager);

        // Start the service
        my_service->Run();
        return 0;
    }
    catch (const DBus::Exception &excp)
    {
        std::cerr << "EXCEPTION CAUGHT: " << excp.what() << std::endl;
        return 2;
    }
}
