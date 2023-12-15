//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   test-data-types.cpp
 *
 * @brief  Just a few tests for the glib2 utils functions.  This test
 *         requires the simple-service.cpp program to run in advance
 */

#include <iostream>
#include <string>
#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/glib2/utils.hpp"
#include "../gdbuspp/proxy.hpp"

#include "test-constants.hpp"

/**
 *  Checks the values hardcoded dictionary in net.openvpn.gdbuspp.test.simple
 *
 *  This primarily exercises the glib2::Value::Dict::Lookup() function
 *  via the DBus::Proxy::Client
 *
 *  D-Bus path:       /gdbuspp/tests/simple1/properties
 *  Object interface: gdbuspp.test.simple1
 *  Property name:    dictionary
 * @param prx
 * @return true on errors, otherwise false.
 */
bool test_dictionary(DBus::Proxy::Client::Ptr prx)
{
    GVariant *dict = prx->GetPropertyGVariant(Test::Constants::GenPath("simple1/properties"),
                                              Test::Constants::GenInterface("simple1"),
                                              "dictionary");
    std::string name = glib2::Value::Dict::Lookup<std::string>(dict, "name");
    std::string key = glib2::Value::Dict::Lookup<std::string>(dict, "key");
    int numbers = glib2::Value::Dict::Lookup<int>(dict, "numbers");
    bool true_val = glib2::Value::Dict::Lookup<bool>(dict, "bool");

    try
    {
        auto wrong_type = glib2::Value::Dict::Lookup<int>(dict, "key");
        std::cout << "glib2::Value::Dict::Lookup() test with "
                  << "incorrect data type FAILED" << std::endl
                  << "  Retrieved value: '" << wrong_type << "'" << std::endl;
        return true;
    }
    catch (const glib2::Utils::Exception &excp)
    {
        // pass
    }
    catch (...)
    {
        throw;
    }


    try
    {
        auto non_exist = glib2::Value::Dict::Lookup<std::string>(dict, "no-such-key");
        std::cout << "glib2::Value::Dict::Lookup() test with "
                  << "non-existing key FAILED" << std::endl
                  << "  Retrieved value: '" << non_exist << "'" << std::endl;
        return true;
    }
    catch (const glib2::Utils::Exception &excp)
    {
        // pass
    }
    catch (...)
    {
        throw;
    }
    g_variant_unref(dict);

    if ((name == "dictionary test")
        && (key == "value")
        && (numbers == 123)
        && true_val == true)
    {
        std::cout << "glib2::Value::Dict::Lookup() test PASSED" << std::endl;
        return false;
    }

    std::cout << "glib2::Value::Dict::Lookup() test FAILED" << std::endl;
    std::cout << "--------------------------------" << std::endl
              << "      name='" << name << "'" << std::endl
              << "       key='" << key << "'" << std::endl
              << "   numbers=" << numbers << std::endl
              << "      bool=" << (true_val ? "true" : "false") << std::endl
              << "--------------------------------" << std::endl
              << std::endl;
    return true;
}



int main()
{
    auto conn = DBus::Connection::Create(DBus::BusType::SESSION);
    auto prx = DBus::Proxy::Client::Create(conn,
                                           Test::Constants::GenServiceName("simple"));

    int failures = 0;
    failures += static_cast<int>(test_dictionary(prx));

    if (failures > 0)
    {
        std::cout << "OVERALL TEST RESULT:  FAIL "
                  << "(" << failures << " tests failed) " << std::endl;
        return 2;
    }
    return 0;
}
