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

#include <algorithm>
#include <functional>
#include <string>
#include <fmt/printf.h>
#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/glib2/utils.hpp"
#include "../gdbuspp/object/path.hpp"
#include "../gdbuspp/proxy.hpp"

#include "test-constants.hpp"
#include "tests/test-utils.hpp"



namespace Tests::Program {

/**
 *  Checks the values hardcoded dictionary in net.openvpn.gdbuspp.test.simple
 *
 *  This primarily exercises the glib2::Value::Dict::Lookup() function
 *  via the DBus::Proxy::Client
 *
 *  D-Bus path:       /gdbuspp/tests/simple1/properties
 *  Object interface: gdbuspp.test.simple1
 *  Property name:    dictionary
 *
 * @param testlog
 * @param prx
 */
void test_dictionary(Tests::Utils::ResultLog &testlog, DBus::Proxy::Client::Ptr prx)
{
    GVariant *dict = prx->GetPropertyGVariant(Tests::Constants::GenPath("simple1/properties"),
                                              Tests::Constants::GenInterface("simple1"),
                                              "dictionary");
    auto name = glib2::Dict::Lookup<std::string>(dict, "name");
    auto key = glib2::Dict::Lookup<std::string>(dict, "key");
    auto numbers = glib2::Dict::Lookup<int>(dict, "numbers");
    auto nummeric_array = glib2::Dict::LookupVector<uint32_t>(dict, "nummeric_array");
    auto true_val = glib2::Dict::Lookup<bool>(dict, "bool");

    testlog.RunFunctionTest("glib2::Value::Dict::Lookup() test, incorrect type", prx, [&dict, &testlog]() -> bool
                            {
                                try
                                {
                                    auto wrong_type = glib2::Dict::Lookup<int>(dict, "key");
                                    testlog.AddLogLine(fmt::format("Retrieved unexpected value: '{}'", wrong_type));
                                    return false;
                                }
                                catch (const glib2::Utils::Exception &excp)
                                {
                                    return true;
                                }
                                catch (...)
                                {
                                    throw;
                                }
                            },
                            true);

    testlog.RunFunctionTest("glib2::Value::Dict::Lookup('no-such-key') test, no-existing key", prx, [&dict, &testlog]() -> bool
                            {
                                try
                                {
                                    auto non_exist = glib2::Dict::Lookup<std::string>(dict, "no-such-key");
                                    testlog.AddLogLine(fmt::format("Retrieved key which should not exist, value: '{}", non_exist));
                                    return false;
                                }
                                catch (const glib2::Utils::Exception &excp)
                                {
                                    return true;
                                }
                                catch (...)
                                {
                                    throw;
                                }
                            },
                            true);

    testlog.RunFunctionTest("glib2::Value::Dict::Lookup() checks", nullptr, [&name, &key, &numbers, &true_val, &dict, &testlog]() -> bool
                            {
                                if ((name == "dictionary test")
                                    && (key == "value")
                                    && (numbers == 123)
                                    && true_val == true)
                                {
                                    return true;
                                }

                                testlog.AddLogLine(fmt::format(
                                    "--------------------------------\n"
                                    "              name = '{}'\n"
                                    "               key = '{}'\n"
                                    "           numbers = {}\n"
                                    "              bool = {}\n"
                                    "         --------------------------------",
                                    name,
                                    key,
                                    numbers,
                                    true_val));
                                testlog.AddGVariantDump("Received dictionary", dict);
                                return false;
                            },
                            true);
    g_variant_unref(dict);
}


/**
 *  Extract the iterator value by the native C++ type
 *  of the value.
 *
 *  This is needed for fmt::format() to be able to
 *  understand which data type it is going to process
 *
 * @tparam ITER  Iterator type, "wrapping" the real value
 * @param itv    iterator carrying the real value
 * @return auto  Returns the real value with the proper type,
 *               "unwrapping" the iterator container.
 */
template <typename ITER>
auto fmt_iterator_value(ITER itv)
{
    using T = typename std::iterator_traits<ITER>::value_type;

    if constexpr (std::is_same_v<std::byte, std::decay_t<T>>)
    {
        return std::to_integer<int>(*itv);
    }
    else
    {
        return static_cast<T>(*itv);
    }
}


template <typename T1, typename T2>
bool compare_vector(Tests::Utils::ResultLog &testlog,
                    const std::string &msg,
                    T1 &vect1,
                    T2 &vect2)

{
    std::string result = fmt::format(
        "{} - Vector size: {}",
        msg,
        (vect1.size() == vect2.size() ? "Pass" : "Different - FAILED"));
    if (vect1.size() != vect2.size())
    {
        testlog.AddLogLine(result);
        testlog.AddLogLine(fmt::format(FMT_COMPILE(
                                           "vector1 size: {}, vector2 size: {}"),
                                       vect1.size(),
                                       vect2.size()));
        return false;
    }

    result.append(", content: ");
    if (!std::equal(vect1.begin(), vect1.end(), vect2.begin(), vect2.end()))
    {
        auto [vect1_it, vect2_it] = std::mismatch(vect1.begin(),
                                                  vect1.end(),
                                                  vect2.begin(),
                                                  vect2.end());
        result.append("Differs - FAILED");
        testlog.AddLogLine(result);
        testlog.AddLogLine(fmt::format("Content vector 1: '{}'",
                                       fmt_iterator_value(vect1_it)));
        testlog.AddLogLine(fmt::format("Content vector 2: '{}'",
                                       fmt_iterator_value(vect2_it)));

        return false;
    }
    result.append(" ==> PASS ");
    testlog.AddLogLine(result);
    return true;
}


void test_base_data_types(Tests::Utils::ResultLog &testlog)
{
    testlog.AddLogLine(":: Testing base data types ...");

    {
        std::byte type_y{255};
        testlog.CheckNativeDataType("std::byte", type_y, "y");
    }

    {
        uint16_t type_q = 9;
        testlog.CheckNativeDataType("uint16_t", type_q, "q");
    }

    {
        int16_t type_n = -9;
        testlog.CheckNativeDataType("int16_t", type_n, "n");
    }

    {
        uint32_t type_u = 9;
        testlog.CheckNativeDataType("uint32_t", type_u, "u");
    }

    {
        int32_t type_i = -9;
        testlog.CheckNativeDataType("int32_t", type_i, "i");
    }

    {
        uint64_t type_t = 9999999;
        testlog.CheckNativeDataType("uint64_t", type_t, "t");
    }

    {
        int64_t type_x = -9999999;
        testlog.CheckNativeDataType("int64_t", type_x, "x");
    }

    {
        double type_d = 77777788888888;
        testlog.CheckNativeDataType("double", type_d, "d");
    }

    {
        bool type_b = true;
        testlog.CheckNativeDataType("bool", type_b, "b");
    }

    {
        std::string type_s = "Hello Test!";
        testlog.CheckNativeDataType("std::string", type_s, "s");
    }

    {
        DBus::Object::Path type_o = "/hello/world";
        testlog.CheckNativeDataType("DBus::Object::Path", type_o, "o");
    }


    testlog.RunExceptionTest("DBus::Object::Path [trailing slash]", nullptr, []()
                             {
                                 DBus::Object::Path wrong_path = "/trailing/slash/";
                             },
                             {"Invalid D-Bus path: /trailing/slash/"});

    testlog.RunExceptionTest("DBus::Object::Path [not absolute path]", nullptr, []()
                             {
                                 DBus::Object::Path wrong_path = "no/leading/slash/";
                             },
                             {"Invalid D-Bus path: no/leading/slash/"});

    testlog.RunExceptionTest("DBus::Object::Path [invalid chars]", nullptr, []()
                             {
                                 DBus::Object::Path wrong_path = "/path/invalid-chars";
                             },
                             {"Invalid D-Bus path: /path/invalid-chars"});


    //
    // GVariant tests
    //
    testlog.CheckGVariantDataType("glib2::Value::Create<std::byte>(...)  ",
                                  glib2::Value::Create(static_cast<std::byte>(255)),
                                  "y");

    testlog.CheckGVariantDataType("glib2::Value::Create<uint16_t>(...)   ",
                                  glib2::Value::Create(static_cast<uint16_t>(12345)),
                                  "q");

    testlog.CheckGVariantDataType("glib2::Value::Create<int16_t>(...)    ",
                                  glib2::Value::Create(static_cast<int16_t>(-12345)),
                                  "n");

    testlog.CheckGVariantDataType("glib2::Value::Create<uint32_t>(...)   ",
                                  glib2::Value::Create(static_cast<uint32_t>(54321)),
                                  "u");

    testlog.CheckGVariantDataType("glib2::Value::Create<int32_t>(...)    ",
                                  glib2::Value::Create(static_cast<int32_t>(-54321)),
                                  "i");

    testlog.CheckGVariantDataType("glib2::Value::Create<uint64_t>(...)   ",
                                  glib2::Value::Create(static_cast<uint64_t>(12345654321)),
                                  "t");

    testlog.CheckGVariantDataType("glib2::Value::Create<int64_t>(...)    ",
                                  glib2::Value::Create(static_cast<int64_t>(-12345654321)),
                                  "x");

    testlog.CheckGVariantDataType("glib2::Value::Create<double>(...)     ",
                                  glib2::Value::Create(static_cast<double>(12345654321254321)),
                                  "d");

    testlog.CheckGVariantDataType("glib2::Value::Create<bool>(...)       ",
                                  glib2::Value::Create(static_cast<bool>(true)),
                                  "b");

    testlog.CheckGVariantDataType("glib2::Value::Create<std::string>(...)",
                                  glib2::Value::Create(std::string("Hello test!")),
                                  "s");

    testlog.CheckGVariantDataType("glib2::Value::Create<DBus::Object::Path>(...)",
                                  glib2::Value::Create(DBus::Object::Path("/hellow/world")),
                                  "o");

    {
        std::vector<std::byte> d = {std::byte(1),
                                    std::byte(2),
                                    std::byte(3),
                                    std::byte(4),
                                    std::byte(5)};
        testlog.CheckGVariantDataType("glib2::Value::Create<std::vector<std::byte>>(...)  ",
                                      glib2::Value::Create(d),
                                      "ay");
    }

    {
        std::vector<int16_t> d = {-1, 2, -3, 4, -5, 0};
        testlog.CheckGVariantDataType("glib2::Value::Create<std::vector<int16_t>>(...)    ",
                                      glib2::Value::Create(d),
                                      "an");
    }

    {
        std::vector<uint32_t> d = {1, 2, 3, 4, 5};
        testlog.CheckGVariantDataType("glib2::Value::Create<std::vector<uint32_t>>(...)   ",
                                      glib2::Value::Create(d),
                                      "au");
    }

    {
        std::vector<int32_t> d = {-1, 2, -3, 4, -5, 0};
        testlog.CheckGVariantDataType("glib2::Value::Create<std::vector<int32_t>>(...)    ",
                                      glib2::Value::Create(d),
                                      "ai");
    }

    {
        std::vector<uint64_t> d = {1, 2, 3, 4, 5};
        testlog.CheckGVariantDataType("glib2::Value::Create<std::vector<uint64_t>>(...)   ",
                                      glib2::Value::Create(d),
                                      "at");
    }

    {
        std::vector<int64_t> d = {-1, 2, -3, 4, -5, 0};
        testlog.CheckGVariantDataType("glib2::Value::Create<std::vector<int64_t>>(...)    ",
                                      glib2::Value::Create(d),
                                      "ax");
    }

    {
        std::vector<double> d = {-1, 2, -3, 4, -5, 0};
        testlog.CheckGVariantDataType("glib2::Value::Create<std::vector<double>>(...)     ",
                                      glib2::Value::Create(d),
                                      "ad");
    }

    {
        std::vector<bool> d = {true, false, false, true};
        testlog.CheckGVariantDataType("glib2::Value::Create<std::vector<bool>>(...)       ",
                                      glib2::Value::Create(d),
                                      "ab");
    }

    {
        std::vector<std::string> d = {"line 1", "line 2", "line 3"};
        testlog.CheckGVariantDataType("glib2::Value::Create<std::vector<std::string>>(...)",
                                      glib2::Value::Create(d),
                                      "as");
    }

    {
        std::vector<DBus::Object::Path> d = {
            "/path_1", "/path_2", "/path/3"};
        testlog.CheckGVariantDataType("glib2::Value::Create<std::vector<DBus::Object::Path>>(...)",
                                      glib2::Value::Create(d),
                                      "ao");
    }

    // The purpose of this test is more to test that ResultLog::CheckGVariantDataType()
    // and the underlying tests there works as expected.
    testlog.CheckGVariantDataType("g_variant_new(\"(iuqntxdbys)\", ...)  ",
                                  g_variant_new("(iuqntxdbys)",
                                                static_cast<int32_t>(-4),
                                                static_cast<uint32_t>(4),
                                                static_cast<uint16_t>(111),
                                                static_cast<int16_t>(-222),
                                                static_cast<uint64_t>(333),
                                                static_cast<int64_t>(-444),
                                                static_cast<double>(-55555),
                                                static_cast<bool>(false),
                                                static_cast<std::byte>(66),
                                                "Large test"),
                                  "(iuqntxdbys)");

    {
        GVariantBuilder *b = glib2::Builder::Create("(soibnuty)");
        auto s = std::string("string");
        glib2::Builder::Add(b, s);
        glib2::Builder::Add(b, DBus::Object::Path("/struct/test"));
        glib2::Builder::Add(b, static_cast<int32_t>(22));
        glib2::Builder::Add(b, static_cast<bool>(true));
        glib2::Builder::Add(b, static_cast<int16_t>(-4444));
        glib2::Builder::Add(b, static_cast<uint32_t>(55555));
        glib2::Builder::Add(b, static_cast<uint64_t>(666666));
        glib2::Builder::Add(b, static_cast<std::byte>(77));
        testlog.CheckGVariantDataType("glib2::Builder::Create(\"(soibnuty)\")",
                                      glib2::Builder::Finish(b),
                                      "(soibnuty)");
    }

    {
        auto str_v = std::vector<std::string>{"string 1", "string B", "string III"};
        auto int_v = std::vector<int32_t>{-103949501, 304958104, 10, 50, 0, -24, -88};
        auto uint_v = std::vector<uint32_t>{0, 10293, 5029, 19586, 1};
        GVariantBuilder *b = glib2::Builder::Create("(asaiau)");
        glib2::Builder::Add(b, str_v);
        glib2::Builder::Add(b, int_v);
        glib2::Builder::Add(b, uint_v);

        testlog.CheckGVariantDataType("glib2::Builder::Create(\"(asaiau)\")  ",
                                      glib2::Builder::Finish(b),
                                      "(asaiau)");
    }

    testlog.RunFunctionTest("glib2::Builder::AddKeyValue - a{qb})", nullptr, [&testlog]() -> bool
                            {
                                GVariantBuilder *keyval = glib2::Builder::Create("a{qb}");
                                glib2::Builder::AddKeyValue<uint16_t, bool>(keyval, 1, true);
                                glib2::Builder::AddKeyValue<uint16_t, bool>(keyval, 2, false);
                                glib2::Builder::AddKeyValue<uint16_t, bool>(keyval, 3, false);
                                glib2::Builder::AddKeyValue<uint16_t, bool>(keyval, 4, true);
                                GVariant *data = glib2::Builder::Finish(keyval);

                                return testlog.CheckDataTypeValue(
                                    "a{qb}",
                                    "{1: true, 2: false, 3: false, 4: true}",
                                    data);
                            },
                            true);

    testlog.RunFunctionTest("glib2::Builder::AddKeyValue - a{sq})", nullptr, [&testlog]() -> bool
                            {
                                GVariantBuilder *keyval = glib2::Builder::Create("a{sq}");
                                glib2::Builder::AddKeyValue<std::string, uint16_t>(keyval, "key_one", 123);
                                glib2::Builder::AddKeyValue<std::string, uint16_t>(keyval, "key_two", 6789);
                                glib2::Builder::AddKeyValue<std::string, uint16_t>(keyval, "key_three", 65535);
                                glib2::Builder::AddKeyValue<std::string, uint16_t>(keyval, "key_four", -1039); // This will wrap
                                GVariant *data = glib2::Builder::Finish(keyval);

                                return testlog.CheckDataTypeValue(
                                    "a{sq}",
                                    "{'key_one': 123, 'key_two': 6789, 'key_three': 65535, 'key_four': 64497}",
                                    data);
                            },
                            true);

    testlog.RunFunctionTest("glib2::Builder::AddKeyValue - a{us})", nullptr, [&testlog]() -> bool
                            {
                                GVariantBuilder *keyval = glib2::Builder::Create("a{us}");
                                glib2::Builder::AddKeyValue<uint32_t, std::string>(keyval, 10, "Key 10");
                                glib2::Builder::AddKeyValue<uint32_t, std::string>(keyval, 20, "Key 20");
                                glib2::Builder::AddKeyValue<uint32_t, std::string>(keyval, 30, "Key 30");
                                GVariant *data = glib2::Builder::Finish(keyval);

                                return testlog.CheckDataTypeValue(
                                    "a{us}",
                                    "{10: 'Key 10', 20: 'Key 20', 30: 'Key 30'}",
                                    data);
                            },
                            true);


    testlog.RunFunctionTest("glib2::Builder::AddKeyValue - a{ss})", nullptr, [&testlog]() -> bool
                            {
                                GVariantBuilder *keyval = glib2::Builder::Create("a{ss}");
                                glib2::Builder::AddKeyValue<std::string, std::string>(keyval, "key_1", "Value A");
                                glib2::Builder::AddKeyValue<std::string, std::string>(keyval, "key_2", "Value B");
                                glib2::Builder::AddKeyValue<std::string, std::string>(keyval, "key_3", "Value C");
                                GVariant *data = glib2::Builder::Finish(keyval);

                                return testlog.CheckDataTypeValue("a{ss}",
                                                                  "{'key_1': 'Value A', 'key_2': 'Value B', 'key_3': 'Value C'}",
                                                                  data);
                            },
                            true);

    testlog.RunFunctionTest("glib2::Builder::AddKeyValue - a{so})", nullptr, [&testlog]() -> bool
                            {
                                GVariantBuilder *keyval = glib2::Builder::Create("a{so}");
                                glib2::Builder::AddKeyValue<std::string, DBus::Object::Path>(keyval, "pathA", "/example/gdbuspp/path1");
                                glib2::Builder::AddKeyValue<std::string, DBus::Object::Path>(keyval, "pathB", "/example/gdbuspp/path2");
                                glib2::Builder::AddKeyValue<std::string, DBus::Object::Path>(keyval, "pathC", "/example/gdbuspp/path3");
                                GVariant *data = glib2::Builder::Finish(keyval);

                                return testlog.CheckDataTypeValue(
                                    "a{so}",
                                    "{'pathA': '/example/gdbuspp/path1', 'pathB': '/example/gdbuspp/path2', 'pathC': '/example/gdbuspp/path3'}",
                                    data);
                            },
                            true);

    testlog.RunFunctionTest("glib2::Builder::AddKeyValue - a{sv})", nullptr, [&testlog]() -> bool
                            {
                                GVariantBuilder *keyval = glib2::Builder::Create("a{sv}");
                                glib2::Builder::AddKeyValue<std::string, GVariant *>(keyval, "pathA", glib2::Value::Create<DBus::Object::Path>("/example/gdbuspp/path1"));
                                glib2::Builder::AddKeyValue<std::string, GVariant *>(keyval, "a_string", glib2::Value::Create<std::string>("some random string"));
                                glib2::Builder::AddKeyValue<std::string, GVariant *>(keyval, "an_int", glib2::Value::Create<int32_t>(12345));
                                glib2::Builder::AddKeyValue<std::string, GVariant *>(keyval, "null_value", nullptr);
                                GVariant *data = glib2::Builder::Finish(keyval);

                                return testlog.CheckDataTypeValue(
                                    "a{sv}",
                                    "{'pathA': <objectpath '/example/gdbuspp/path1'>, 'a_string': <'some random string'>, 'an_int': <12345>, 'null_value': <@mv nothing>}",
                                    data);
                            },
                            true);

    testlog.RunFunctionTest("glib2::Value::IterateArray", nullptr, [&testlog]() -> bool
                            {
                                std::vector<std::string> data = {"Element number 1", "Element B", "Element Three"};
                                GVariant *array = glib2::Value::Create(data);

                                try
                                {
                                    int array_counter = 0;
                                    glib2::Value::IterateArray(array,
                                                               [&array_counter, &data](GVariant *element)
                                                               {
                                                                   auto val = glib2::Value::Get<std::string>(element);
                                                                   if (val != data[array_counter])
                                                                   {
                                                                       throw std::runtime_error(
                                                                           fmt::format("Unxpected data: {:?} != {:?}",
                                                                                       val,
                                                                                       data[array_counter]));
                                                                   }
                                                                   ++array_counter;
                                                               });
                                    g_variant_unref(array);
                                    return true;
                                }
                                catch (const std::runtime_error &err)
                                {
                                    testlog.AddLogLine(fmt::format(
                                        FMT_COMPILE("    ** IterateArray exception: {}"),
                                        err.what()));
                                    g_variant_unref(array);
                                    return false;
                                }
                            },
                            true);

    testlog.RunFunctionTest("glib2::Dict - Generic tests (a{sv})", nullptr, [&testlog]() -> bool
                            {
                                GVariantDict *dict = glib2::Dict::Create();
                                glib2::Dict::Add<int32_t>(dict, "type_int32", 123);
                                glib2::Dict::Add<uint64_t>(dict, "type_int32", 9402038495);
                                glib2::Dict::Add<bool>(dict, "type_bool_false", false);
                                glib2::Dict::Add<bool>(dict, "type_bool_true", true);
                                glib2::Dict::Add<std::string>(dict, "type_string", "A dictionary string element");
                                glib2::Dict::Add<DBus::Object::Path>(dict, "type_object_path", "/org/example/gdbuspp/object/path");
                                std::vector<uint16_t> vector_ui16{1, 2, 4, 8, 16, 32, 65};
                                glib2::Dict::Add(dict, "vector_ui16", vector_ui16);
                                GVariant *data = glib2::Dict::Finish(dict);

                                return testlog.CheckDataTypeValue(
                                    "a{sv}",
                                    "{'type_bool_false': <false>, 'type_string': <'A dictionary string element'>, 'type_bool_true': <true>, "
                                    "'type_int32': <uint64 9402038495>, 'type_object_path': <objectpath '/org/example/gdbuspp/object/path'>, "
                                    "'vector_ui16': <[uint16 1, 2, 4, 8, 16, 32, 65]>}",
                                    data);
                            },
                            true);

    testlog.RunFunctionTest("glib2::Dict::IterateDictionary", nullptr, [&testlog]() -> bool
                            {
                                GVariantDict *dict = glib2::Dict::Create();
                                glib2::Dict::Add<int32_t>(dict, "type_int32", 123);
                                glib2::Dict::Add<uint64_t>(dict, "type_uint64", 9402038495);
                                glib2::Dict::Add<bool>(dict, "type_bool_false", false);
                                ;
                                glib2::Dict::Add<bool>(dict, "type_bool_true", true);
                                glib2::Dict::Add<std::string>(dict, "type_string", "A dictionary string element");
                                glib2::Dict::Add<DBus::Object::Path>(dict, "type_object_path", "/org/example/gdbuspp/object/path");

                                std::vector<uint16_t> vector_ui16{1, 2, 4, 8, 16, 32, 65};
                                glib2::Dict::Add(dict, "vector_ui16", vector_ui16);
                                GVariant *data = glib2::Dict::Finish(dict);

                                try
                                {
                                    glib2::Dict::IterateDictionary(
                                        data,
                                        [&vector_ui16](const std::string &key, GVariant *value)
                                        {
                                            if ("type_int32" == key
                                                && 123 != glib2::Value::Get<int32_t>(value))
                                            {
                                                throw std::runtime_error("Incorrect value for 'type_int32'");
                                            }
                                            else if ("type_uint64" == key
                                                     && static_cast<uint64_t>(9402038495) != glib2::Value::Get<uint64_t>(value))
                                            {
                                                throw std::runtime_error("Incorrect value for 'type_uint32'");
                                            }
                                            else if ("type_bool_false" == key
                                                     && false != glib2::Value::Get<bool>(value))
                                            {
                                                throw std::runtime_error("Incorrect value for 'type_bool_false'");
                                            }
                                            else if ("type_bool_true" == key
                                                     && true != glib2::Value::Get<bool>(value))
                                            {
                                                throw std::runtime_error("Incorrect value for 'type_bool_true'");
                                            }
                                            else if ("type_string" == key
                                                     && "A dictionary string element" != glib2::Value::Get<std::string>(value))
                                            {
                                                throw std::runtime_error("Incorrect value for 'type_string'");
                                            }
                                            else if ("type_object_path" == key
                                                     && "/org/example/gdbuspp/object/path" != glib2::Value::Get<std::string>(value))
                                            {
                                                throw std::runtime_error("Incorrect value for 'type_object_path'");
                                            }
                                            else if ("vector_ui16" == key
                                                     && vector_ui16 != glib2::Value::ExtractVector<uint16_t>(value))
                                            {
                                                throw std::runtime_error("Incorrect value for 'vector_ui16'");
                                            }
                                        });
                                    return true;
                                }
                                catch (const std::runtime_error &err)
                                {
                                    testlog.AddLogLine(fmt::format(
                                        FMT_COMPILE("    ** IterateDictionary exception: {}"),
                                        err.what()));
                                    return false;
                                }
                            },
                            true);
}



void test_base_vector(Tests::Utils::ResultLog &testlog)
{
    // testlog.AddLogLine(":: Testing vectorized base data types ...");

    // std::byte
    testlog.RunFunctionTest("std::vector<std::byte>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<std::byte> d = {
                                    std::byte{1},
                                    std::byte{8},
                                    std::byte{0},
                                    std::byte{128},
                                    std::byte{15},
                                    std::byte{16},
                                    std::byte{127},
                                    std::byte{255}};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<std::byte>(vector);
                                return compare_vector(testlog, "std::byte", d, res);
                            },
                            true);

    // uint16_t
    testlog.RunFunctionTest("std::vector<uint16_t>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<uint16_t> d = {430, 10439, 5039, 102, 0, 994};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<uint16_t>(vector);
                                return compare_vector(testlog, "uint16_t", d, res);
                            },
                            true);

    // int16_t
    testlog.RunFunctionTest("std::vector<int16_t>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<int16_t> d = {304, -103, 4, -32405, 304, 506};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<int16_t>(vector);
                                return compare_vector(testlog, "int16_t", d, res);
                            },
                            true);

    // uint32_t
    testlog.RunFunctionTest("std::vector<uint32_t>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<uint32_t> d = {95817395, 103945850, 40913, 0, 59, 958274958};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<uint32_t>(vector);
                                return compare_vector(testlog, "uint32_t", d, res);
                            },
                            true);

    // int32_t
    testlog.RunFunctionTest("std::vector<int32_t>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<int32_t> d = {3049, 0, -1049581045, -309185019, 6610495, 86371, 9483, 6610495};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<int32_t>(vector);
                                return compare_vector(testlog, "int32_t", d, res);
                            },
                            true);

    // uint64_t
    testlog.RunFunctionTest("std::vector<uint64_t>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<uint64_t> d = {8476164390, 1034985710104950195, 0, 10495871};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<uint64_t>(vector);
                                return compare_vector(testlog, "uint64_t", d, res);
                            },
                            true);

    // int64_t
    testlog.RunFunctionTest("std::vector<int64_t>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<int64_t> d = {-1043958104856, 309858106037, -5098282759120942699, 999999999999999999, 19485601895, 0, 2984585, 104959185, -999999999999999999};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<int64_t>(vector);
                                return compare_vector(testlog, "int64_t", d, res);
                            },
                            true);

    // double
    testlog.RunFunctionTest("std::vector<double>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<double> d = {1.0, -0.4, 3.14159267, 0, 4958194920958193.9471683, -9482748750.22593};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<double>(vector);
                                return compare_vector(testlog, "double", d, res);
                            },
                            true);

    // bool
    testlog.RunFunctionTest("std::vector<bool>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<bool> d = {false, true, 0, 1, false, false, true};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<bool>(vector);
                                return compare_vector(testlog, "bool", d, res);
                            },
                            true);

    // std::string
    testlog.RunFunctionTest("std::vector<std::string>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<std::string> d = {"line 1", "line 2", "line 3"};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<std::string>(vector);
                                return compare_vector(testlog, "std::string", d, res);
                            },
                            true);

    // DBus::Object::Path
    testlog.RunFunctionTest("std::vector<DBus::Object::Path>", nullptr, [&testlog]() -> bool
                            {
                                std::vector<DBus::Object::Path> d = {
                                    "/net/openvpn/gdbus/object_path/test1",
                                    "/net/openvpn/gdbus/object_path/test2",
                                    "/net/openvpn/gdbus/object_path/test3",
                                    "/net/openvpn/gdbus/object_path/test4",
                                    "/net/openvpn/gdbus/object_path/test5"};
                                GVariant *vector = glib2::Value::Create(d);
                                auto res = glib2::Value::ExtractVector<DBus::Object::Path>(vector);
                                return compare_vector(testlog, "DBus::Object::Path", d, res);
                            },
                            true);
};

int test_data_types_main(int argc, char **argv)
{
    DBus::Proxy::Client::Ptr prx = nullptr;
    if ((argc > 1 && (std::string(argv[1]) == "--service-simple")))
    {
        auto conn = DBus::Connection::Create(DBus::BusType::SESSION);
        prx = DBus::Proxy::Client::Create(conn,
                                          Tests::Constants::GenServiceName("simple"));
    }

    Tests::Utils::ResultLog testlog(true);

    if (!prx)
    {
        test_base_data_types(testlog);
        test_base_vector(testlog);
    }

    if (prx)
    {
        test_dictionary(testlog, prx);
    }

    return testlog.PrintTestReport() ? 0 : 2;
}

} // namespace Tests::Program


int main(int argc, char **argv)
{
    return Tests::Program::test_data_types_main(argc, argv);
}