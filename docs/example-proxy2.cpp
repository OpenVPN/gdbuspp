//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   example-proxy2.cpp
 *
 * @brief  Variant of example-proxy.cpp which implements the proxy
 *         as a C++ object
 *
 *         To compile this program:
 *
 *  $ c++ -std=c++17 $(pkg-config gdbuspp glib-2.0 gio-2.0 gio-unix-2.0 --cflags --libs) -o example-proxy2 example-proxy2.cpp
 *
 */

#include <iostream>
#include <string>

#include <gdbuspp/connection.hpp>
#include <gdbuspp/proxy.hpp>
#include <gdbuspp/glib2/utils.hpp>

namespace Examples {

class MyServiceProxy
{
  public:
    /**
     *
     *
     *  Enforce this object to always be set up with a smart pointer
     *
     * @param c
     * @return std::shared_ptr<MyServiceProxy>
     */
    [[nodiscard]] static std::shared_ptr<MyServiceProxy> Create(DBus::Connection::Ptr c)
    {
        return std::shared_ptr<MyServiceProxy>(new MyServiceProxy(c));
    }

    /**
     *  Calls the net.example.myinterface.MethodWithArgs D-Bus method
     *  in the /example/myobject D-Bus object in the net.example.myservice
     *  D-Bus service.
     *
     *  This method takes two string arguments as input and returns a
     *  a single string in return
     *
     * @param string1
     * @param string2
     * @return const std::string
     */
    const std::string MethodWithArgs(const std::string &string1,
                                     const std::string &string2)
    {
        // Create the "argument pack" containing of 2 strings -> (ss)
        GVariantBuilder *args_builder = glib2::Builder::Create("(ss)");
        glib2::Builder::Add(args_builder, string1);
        glib2::Builder::Add(args_builder, string2);
        GVariant *arguments = glib2::Builder::Finish(args_builder);

        // Perform the D-Bus method call
        GVariant *response = proxy->Call(preset,
                                         "MethodWithArgs",
                                         arguments);
        // Extract the response from the object.  This method returns only a
        // single string.
        const auto result = glib2::Value::Extract<std::string>(response, 0);
        g_variant_unref(response);

        return result;
    }


    /**
     *  Calls the net.example.myinterface.MyMethod D-Bus method
     *  in the /example/myobject D-Bus object in the net.example.myservice
     *  D-Bus service.
     *
     *  This method takes no arguments and provides no response.
     */
    void MyMethod()
    {
        GVariant *response = proxy->Call(preset, "MyMethod");
        g_variant_unref(response);
    }

    /**
     *  Generic method to retrieve a property value from the service
     *
     * @tparam T              C++ data type to extract the data as; must
     *                        be the equivalent type of the D-Bus object's
     *                        data type
     * @param property_name   Name of the D-Bus object property
     * @return T
     */
    template <typename T>
    T GetProperty(const std::string &property_name) const
    {
        return proxy->GetProperty<T>(preset, property_name);
    }

    /**
     *  Generic method to modify a property value stored in the service
     *
     * @tparam T              C++ data type to extract the data as; must
     *                        be the equivalent type of the D-Bus object's
     *                        data type
     * @param property_name   Name of the D-Bus object property
     * @param value           The new value for the property
     */
    template <typename T>
    void SetProperty(const std::string &property_name,
                     const T &value) const
    {
        proxy->SetProperty(preset, property_name, value);
    }


  private:
    DBus::Proxy::Client::Ptr proxy = nullptr;
    DBus::Proxy::TargetPreset::Ptr preset = nullptr;

    MyServiceProxy(DBus::Connection::Ptr conn)
    {
        proxy = DBus::Proxy::Client::Create(conn, "net.example.myservice");
        preset = DBus::Proxy::TargetPreset::Create("/example/myobject",
                                                   "net.example.myinterface");
    }
};

} // namespace Examples


int main(int argc, char **argv)
{
    try
    {
        // Get a connection to the Session D-Bus
        auto connection = DBus::Connection::Create(DBus::BusType::SESSION);

        // Setup a client proxy to our example-service
        auto proxy = Examples::MyServiceProxy::Create(connection);

        // Call the net.example.myinterface.MyMethod
        proxy->MyMethod();

        // Call the net.example.myinterface.MethodWithArgs D-Bus method
        auto result = proxy->MethodWithArgs("My first string",
                                            "My Second String");
        std::cout << "Method call result: " << result << std::endl;

        // Retrieve the content of the D-Bus object property, which is a string
        auto my_property = proxy->GetProperty<std::string>("my_property");
        std::cout << "my_property: " << my_property << std::endl;

        // Change this property to a new string
        std::string new_property_value = "A changed property";
        proxy->SetProperty("my_property", new_property_value);

        // Retrieve the same property again to show this value did indeed change
        std::cout << "modified property: "
                  << proxy->GetProperty<std::string>("my_property")
                  << std::endl;
        return 0;
    }
    catch (const DBus::Exception &excp)
    {
        std::cerr << "EXCEPTION CAUGHT: " << excp.what() << std::endl;
        return 2;
    }
}
