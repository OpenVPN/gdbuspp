//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file tests/regression/glib2_utils.cpp
 *
 * @brief  Tests bug fixes in glib2::Utils, to avoid regressions later on
 */

#include <cinttypes>
#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <glib.h>
#include "gdbuspp/glib2/utils.hpp"


bool expect_throw(std::function<void()> &&testfunc, const std::string &error_match)
{
    try
    {
        testfunc();
        return false;
    }
    catch (const glib2::Utils::Exception &ex)
    {
        std::string err(ex.what());
        if (err.find(error_match) != std::string::npos)
        {
            return true;
        }
        throw;
    }
}

/**
 *  glib2::Utils::checkParams() should not fail if the data type
 *  of the input variable is not a container type.  The bug discovered
 *  would result in g_variant_n_children() to make this function misbehave
 *  and later on potentially crash the program.
 *
 *  If the data type is not a container type, it cannot have any children.
 *
 *  Returns true on errors, otherwise false on success.
 */
bool checkParams_no_fail_no_container()
{
    std::vector<std::uint16_t> test_data_array = {{1, 2, 3, 4, 5}};
    GVariant *data_array = glib2::Value::CreateVector(test_data_array);
    std::cout << "data_array=" << g_variant_print(data_array, true)
              << std::endl;
    glib2::Utils::checkParams(__func__, data_array, "aq");
    glib2::Utils::checkParams(__func__, data_array, "aq", 5);
    expect_throw([data_array]()
                 {
                     // Incorrect number of elements; expected to throw an exception
                     glib2::Utils::checkParams(__func__, data_array, "aq", 2);
                 },
                 "Incorrect parameter format: aq, expected aq (elements expected: 2, received: 5)");

    GVariant *data_ints = g_variant_new("(qq)", 123, 456);
    std::cout << "data_int=" << g_variant_print(data_ints, true) << std::endl;
    glib2::Utils::checkParams(__func__, data_ints, "(qq)");
    glib2::Utils::checkParams(__func__, data_ints, "(qq)", 2);
    expect_throw([data_ints]()
                 {
                     // Incorrect number of elements; expected to throw an exception
                     glib2::Utils::checkParams(__func__, data_ints, "qq", 3);
                 },
                 "Incorrect parameter format: (qq), expected qq (elements expected: 3, received: 2)");

    GVariantBuilder *data_dict_b = glib2::Builder::Create("a{ss}");
    glib2::Builder::Add(data_dict_b, g_variant_new("{ss}", "Key", "Value"));
    GVariant *data_dict = glib2::Builder::Finish(data_dict_b);
    std::cout << "data_dict=" << g_variant_print(data_dict, true) << std::endl;
    glib2::Utils::checkParams(__func__, data_dict, "a{ss}");
    glib2::Utils::checkParams(__func__, data_dict, "a{ss}", 1);
    expect_throw([data_dict]()
                 {
                     // Incorrect number of elements; expected to throw an exception
                     glib2::Utils::checkParams(__func__, data_dict, "a{ss}", 2);
                 },
                 "Incorrect parameter format: a{ss}, expected a{ss} (elements expected: 2, received: 1)");

    std::string test_data_string{"Hello tester"};
    GVariant *data_string = glib2::Value::Create(test_data_string);
    std::cout << "data_string=" << g_variant_print(data_string, true) << std::endl;
    glib2::Utils::checkParams(__func__, data_string, "s");
    expect_throw([data_string]()
                 {
                     // Incorrect number of elements; expected to throw an exception
                     glib2::Utils::checkParams(__func__, data_string, "s", 1);
                 },
                 "Parameter type is not a container, it has no children");

    GVariant *data_bool = glib2::Value::Create(true);
    std::cout << "data_bool=" << g_variant_print(data_bool, true) << std::endl;
    glib2::Utils::checkParams(__func__, data_bool, "b");
    expect_throw([data_bool]()
                 {
                     // Incorrect number of elements; expected to throw an exception
                     glib2::Utils::checkParams(__func__, data_bool, "b", 1);
                 },
                 "Parameter type is not a container, it has no children");

    return false;
}


int main()
{
    try
    {
        return checkParams_no_fail_no_container() ? 1 : 0;
    }
    catch (const DBus::Exception &excp)
    {
        std::cout << std::endl
                  << "EXCEPTION: " << std::endl
                  << "     " << excp.what() << std::endl;
        return 2;
    }
}
