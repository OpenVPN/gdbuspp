//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   example-proxy.cpp
 *
 * @brief  Simple example proxy client.
 *         This example code is referenced to from the README.md document.
 *
 *         To compile this program:
 *
 *  $ c++ -std=c++17 $(pkg-config gdbuspp glib-2.0 gio-2.0 gio-unix-2.0 --cflags --libs) -o example-proxy example-proxy.cpp
 *
 */

#include <iostream>
#include <string>

#include <gdbuspp/connection.hpp>
#include <gdbuspp/proxy.hpp>
#include <gdbuspp/glib2/utils.hpp>


int main(int argc, char **argv)
{
    try
    {
        // Get a connection to the Session D-Bus
        auto connection = DBus::Connection::Create(DBus::BusType::SESSION);

        // Setup a client proxy to our example-service
        auto proxy = DBus::Proxy::Client::Create(connection, "net.example.myservice");

        // Prepare an object and interface target we want to access.
        // This consists of a D-Bus object path and the interface scope inside
        // that object
        auto preset = DBus::Proxy::TargetPreset::Create("/example/myobject",
                                                        "net.example.myinterface");

        // Doing a method call; prepare the argument values required for the
        // 'MethodWithArgs'  D-Bus method, which takes two strings.  We use
        // the glib2::Builder APIs for creating the complete "argument package"
        // for the method call
        GVariantBuilder *args_builder = glib2::Builder::Create("(ss)");
        glib2::Builder::Add(args_builder, std::string("My first string"));
        glib2::Builder::Add(args_builder, std::string("My Second String"));
        GVariant *arguments = glib2::Builder::Finish(args_builder);

        // Perform the D-Bus method call
        GVariant *response = proxy->Call(preset, "MethodWithArgs", arguments);

        // Extract the response from the object.  This method returns only a
        // single string.
        auto result = glib2::Value::Extract<std::string>(response, 0);
        g_variant_unref(response);
        std::cout << "Method call result: " << result << std::endl;

        // Retrieve the content of the D-Bus object property, which is a string
        auto my_property = proxy->GetProperty<std::string>(preset, "my_property");
        std::cout << "my_property: " << my_property << std::endl;

        // Change this property to a new string
        std::string new_property_value = "A changed property";
        proxy->SetProperty(preset, "my_property", new_property_value);

        // Retrieve the same property again to show this value did indeed change
        std::cout << "modified property: "
                  << proxy->GetProperty<std::string>(preset, "my_property")
                  << std::endl;
        return 0;
    }
    catch (const DBus::Exception &excp)
    {
        std::cerr << "EXCEPTION CAUGHT: " << excp.what() << std::endl;
        return 2;
    }
}
