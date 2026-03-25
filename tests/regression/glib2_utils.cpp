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
#include <functional>
#include <string>
#include <vector>
#include <fmt/printf.h>
#include <glib.h>
#include "../../gdbuspp/glib2/utils.hpp"

#include "../test-utils.hpp"


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
bool checkParams_no_fail_no_container(Tests::Utils::ResultLog &log)
{
    std::vector<std::uint16_t> test_data_array = {{1, 2, 3, 4, 5}};
    GVariant *data_array = glib2::Value::Create(test_data_array);
    log.RunFunctionTest("Testing std::vector<std::uint16_t>", nullptr, [&data_array, &log]() -> bool
                        {
                            log.AddGVariantDump("data_array", data_array);
                            glib2::Utils::checkParams(__func__, data_array, "aq");
                            glib2::Utils::checkParams(__func__, data_array, "aq", 5);
                            return true;
                        },
                        true);
    log.RunExceptionTest("Testing std::Vector<std::uint16_t> wrong elements",
                         nullptr,
                         [&data_array]()
                         {
                             // Incorrect number of elements; expected to throw an exception
                             glib2::Utils::checkParams(__func__, data_array, "aq", 2);
                         },
                         {"Incorrect parameter format: aq, expected aq (elements expected: 2, received: 5)"});


    GVariant *data_ints = g_variant_new("(qq)", 123, 456);
    log.RunFunctionTest("Testing g_variant_new('qq',...)", nullptr, [&data_ints, &log]() -> bool
                        {
                            log.AddGVariantDump("data_ints", data_ints);
                            glib2::Utils::checkParams(__func__, data_ints, "(qq)");
                            glib2::Utils::checkParams(__func__, data_ints, "(qq)", 2);
                            return true;
                        },
                        true);
    log.RunExceptionTest("Incorrect number of elements (qq)", nullptr, [&data_ints]()
                         {
                             // Incorrect number of elements; expected to throw an exception
                             glib2::Utils::checkParams(__func__, data_ints, "qq", 3);
                         },
                         {"Incorrect parameter format: (qq), expected qq (elements expected: 3, received: 2)"});


    GVariantBuilder *data_dict_b = glib2::Builder::Create("a{ss}");
    glib2::Builder::Add(data_dict_b, g_variant_new("{ss}", "Key", "Value"));
    GVariant *data_dict = glib2::Builder::Finish(data_dict_b);
    log.RunFunctionTest("Testing GVariant Builder dictionary (a{ss})", nullptr, [&data_dict, &log]() -> bool
                        {
                            log.AddGVariantDump("data_dict", data_dict);
                            glib2::Utils::checkParams(__func__, data_dict, "a{ss}");
                            glib2::Utils::checkParams(__func__, data_dict, "a{ss}", 1);
                            return true;
                        },
                        true);
    log.RunExceptionTest("Testing incorrect number of dictionary elements", nullptr, [data_dict]()
                         {
                             // Incorrect number of elements; expected to throw an exception
                             glib2::Utils::checkParams(__func__, data_dict, "a{ss}", 2);
                         },
                         {"Incorrect parameter format: a{ss}, expected a{ss} (elements expected: 2, received: 1)"});


    std::string test_data_string{"Hello tester"};
    GVariant *data_string = glib2::Value::Create(test_data_string);
    log.RunFunctionTest("Testing std::string", nullptr, [&data_string, &log]() -> bool
                        {
                            log.AddGVariantDump("data_string", data_string);
                            glib2::Utils::checkParams(__func__, data_string, "s");
                            return true;
                        },
                        true);
    log.RunExceptionTest("Testing for elements in a non-container GVariant('s')", nullptr, [&data_string]()
                         {
                             // Incorrect number of elements; expected to throw an exception
                             glib2::Utils::checkParams(__func__, data_string, "s", 1);
                         },
                         {"Parameter type is not a container, it has no children"});


    GVariant *data_bool = glib2::Value::Create(true);
    log.RunFunctionTest("Testing bool", nullptr, [&data_bool, &log]() -> bool
                        {
                            log.AddGVariantDump("data_bool", data_bool);
                            glib2::Utils::checkParams(__func__, data_bool, "b");
                            return true;
                        },
                        true);
    log.RunExceptionTest("testing for elements in a non-container GVariant('b')", nullptr, [&data_bool]()
                         {
                             // Incorrect number of elements; expected to throw an exception
                             glib2::Utils::checkParams(__func__, data_bool, "b", 1);
                         },
                         {"Parameter type is not a container, it has no children"});

    log.DumpLogs();
    return log.PrintTestReport();
}


int main()
{
    Tests::Utils::ResultLog log;
    try
    {
        return checkParams_no_fail_no_container(log) ? 0 : 1;
    }
    catch (const DBus::Exception &excp)
    {
        log.DumpLogs();
        fmt::print("\nEXCEPTION:\n     {}\n", excp.what());
        return 2;
    }
}
