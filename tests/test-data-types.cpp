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
#include <iostream>
#include <string>
#include "../gdbuspp/connection.hpp"
#include "../gdbuspp/glib2/utils.hpp"
#include "../gdbuspp/object/path.hpp"
#include "../gdbuspp/proxy.hpp"

#include "test-constants.hpp"
#include "tests/test-utils.hpp"

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
    auto name = glib2::Dict::Lookup<std::string>(dict, "name");
    auto key = glib2::Dict::Lookup<std::string>(dict, "key");
    auto numbers = glib2::Dict::Lookup<int>(dict, "numbers");
    auto nummeric_array = glib2::Dict::LookupVector<uint32_t>(dict, "nummeric_array");
    auto true_val = glib2::Dict::Lookup<bool>(dict, "bool");

    try
    {
        auto wrong_type = glib2::Dict::Lookup<int>(dict, "key");
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
        auto non_exist = glib2::Dict::Lookup<std::string>(dict, "no-such-key");
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


struct TestResult
{
    TestResult(const std::string &msg, const bool res)
        : result(res), message(msg)
    {
    }

    const bool result;
    const std::string message;
};


template <typename T>
TestResult check_data_type_cpp(const std::string &type_str,
                               T &value,
                               const std::string &expect)
{
    return TestResult("glib2::DataType::DBus<" + type_str + ">() - Expects: '" + expect + "'",
                      glib2::DataType::DBus<T>() == expect);
}


TestResult check_data_type_gvariant(const std::string &msg,
                                    GVariant *value,
                                    const std::string &expect)
{
    std::ostringstream message;

    message << msg << " - Expects '" << expect << "'  ";

    if (nullptr == value)
    {
        message << "GVariant == nullptr";
        return TestResult(message.str(), false);
    }


    bool pass = true;
    message << "{ g_variant_get_type_string(...) -> ";
    const char *type_c = g_variant_get_type_string(value);
    std::string type = std::string(type_c ? type_c : "");
    if (type != expect)
    {
        pass = false;
        message << "FAILED, received: '" << type << "'";
    }
    else
    {
        message << "Pass";
    }
    message << " }  ::  ";

    type = glib2::DataType::Extract(value);
    message << "{ glib2::DataType::Exctract(...) -> ";
    if (type != expect)
    {
        pass = false;
        message << "  FAILED, received: " << type;
    }
    else
    {
        message << "Pass";
    }
    message << " } >>> Test";

    g_variant_unref(value);
    return TestResult(message.str(), pass);
}


template <typename T>
auto stream_iterator_value(T t)
{
    if constexpr (std::is_same_v<std::byte, std::decay_t<T>>)
        return std::to_integer<int>(t);
    else
        return t;
}


template <typename T1, typename T2>
TestResult compare_vector(const std::string &msg,
                          T1 &vect1,
                          T2 &vect2)

{

    std::ostringstream message;
    message << "[std::vector check]  " << msg << " - Vector size: ";
    if (vect1.size() != vect2.size())
    {
        message << "Different - FAILED";
        return TestResult(message.str(), false);
    }
    message << "Pass, ";

    message << "Content: ";
    if (!std::equal(vect1.begin(), vect1.end(), vect2.begin(), vect2.end()))
    {
        auto [vect1_it, vect2_it] = std::mismatch(vect1.begin(),
                                                  vect1.end(),
                                                  vect2.begin(),
                                                  vect2.end());
        message << "Differs ('" << stream_iterator_value(*vect1_it) << "', '" << stream_iterator_value(*vect2_it) << "'): FAILED";
        return TestResult(message.str(), false);
    }
    message << "Pass -> Test case";
    return TestResult(message.str(), true);
}


template <typename FUNC>
inline int run_test(FUNC &&testfunc)
{
    TestResult test = testfunc();
    std::cout << test.message << ": " << (test.result ? "Pass" : "FAIL") << std::endl;
    return (test.result ? 0 : 1);
}

int test_base_data_types()
{
    std::cout << ":: Testing base data types ..." << std::endl;
    int failures = 0;

    failures += run_test([]()
                         {
                             std::byte type_y{255};
                             return check_data_type_cpp(
                                 "std::byte",
                                 type_y,
                                 "y");
                         });
    failures += run_test([]()
                         {
                             uint16_t type_q = 9;
                             return check_data_type_cpp(
                                 "uint16_t",
                                 type_q,
                                 "q");
                         });

    failures += run_test([]()
                         {
                             int16_t type_n = -9;
                             return check_data_type_cpp(
                                 "int16_t",
                                 type_n,
                                 "n");
                         });

    failures += run_test([]()
                         {
                             uint32_t type_u = 9;
                             return check_data_type_cpp(
                                 "uint32_t",
                                 type_u,
                                 "u");
                         });

    failures += run_test([]()
                         {
                             int32_t type_i = -9;
                             return check_data_type_cpp(
                                 "int32_t",
                                 type_i,
                                 "i");
                         });


    failures += run_test([]()
                         {
                             uint64_t type_t = 9999999;
                             return check_data_type_cpp(
                                 "uint64_t",
                                 type_t,
                                 "t");
                         });

    failures += run_test([]()
                         {
                             int64_t type_x = -9999999;
                             return check_data_type_cpp(
                                 "int64_t",
                                 type_x,
                                 "x");
                         });


    failures += run_test([]()
                         {
                             double type_d = 77777788888888;
                             return check_data_type_cpp(
                                 "double",
                                 type_d,
                                 "d");
                         });

    failures += run_test([]()
                         {
                             bool type_b = true;
                             return check_data_type_cpp(
                                 "bool",
                                 type_b,
                                 "b");
                         });

    failures += run_test([]()
                         {
                             std::string type_s = "Hello Test!";
                             return check_data_type_cpp(
                                 "std::string",
                                 type_s,
                                 "s");
                         });

    failures += run_test([]()
                         {
                             DBus::Object::Path type_o = "/hello/world";
                             return check_data_type_cpp(
                                 "DBus::Object::Path",
                                 type_o,
                                 "o");
                         });

    failures += run_test([]()
                         {
                             bool res = false;
                             try
                             {
                                 DBus::Object::Path wrong_path = "/trailing/slash/";
                             }
                             catch (const DBus::Object::Exception &excp)
                             {
                                 std::string err(excp.what());
                                 if (err.find("Invalid D-Bus path") != std::string::npos)
                                 {
                                     res = true;
                                 }
                             }
                             return TestResult("DBus::Object::Path [trailing slash]",
                                               res);
                         });

    failures += run_test([]()
                         {
                             bool res = false;
                             try
                             {
                                 DBus::Object::Path wrong_path = "trailing/slash/";
                             }
                             catch (const DBus::Object::Exception &excp)
                             {
                                 std::string err(excp.what());
                                 if (err.find("Invalid D-Bus path") != std::string::npos)
                                 {
                                     res = true;
                                 }
                             }
                             return TestResult("DBus::Object::Path [not absolute path]",
                                               res);
                         });

    failures += run_test([]()
                         {
                             bool res = false;
                             try
                             {
                                 DBus::Object::Path wrong_path = "/path/invalid-chars";
                             }
                             catch (const DBus::Object::Exception &excp)
                             {
                                 std::string err(excp.what());
                                 if (err.find("Invalid D-Bus path") != std::string::npos)
                                 {
                                     res = true;
                                 }
                             }
                             return TestResult("DBus::Object::Path [invalid chars]",
                                               res);
                         });

    //
    // GVariant tests
    //
    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::byte>(...)  ",
                                 glib2::Value::Create(static_cast<std::byte>(255)),
                                 "y");
                         });

    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<uint16_t>(...)   ",
                                 glib2::Value::Create(static_cast<uint16_t>(12345)),
                                 "q");
                         });

    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<int16_t>(...)    ",
                                 glib2::Value::Create(static_cast<int16_t>(-12345)),
                                 "n");
                         });

    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<uint32_t>(...)   ",
                                 glib2::Value::Create(static_cast<uint32_t>(54321)),
                                 "u");
                         });

    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<int32_t>(...)    ",
                                 glib2::Value::Create(static_cast<int32_t>(-54321)),
                                 "i");
                         });


    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<uint64_t>(...)   ",
                                 glib2::Value::Create(static_cast<uint64_t>(12345654321)),
                                 "t");
                         });

    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<int64_t>(...)    ",
                                 glib2::Value::Create(static_cast<int64_t>(-12345654321)),
                                 "x");
                         });

    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<double>(...)     ",
                                 glib2::Value::Create(static_cast<double>(12345654321254321)),
                                 "d");
                         });

    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<bool>(...)       ",
                                 glib2::Value::Create(static_cast<bool>(true)),
                                 "b");
                         });


    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::string>(...)",
                                 glib2::Value::Create(std::string("Hello test!")),
                                 "s");
                         });

    failures += run_test([]()
                         {
                             DBus::Object::Path path = "/hello/world";
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<DBus::Object::Path>(...)",
                                 glib2::Value::Create(path),
                                 "o");
                         });


    failures += run_test([]()
                         {
                             std::vector<std::byte> d = {std::byte(1),
                                                         std::byte(2),
                                                         std::byte(3),
                                                         std::byte(4),
                                                         std::byte(5)};
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::vector<std::byte>>(...)  ",
                                 glib2::Value::Create(d),
                                 "ay");
                         });

    failures += run_test([]()
                         {
                             std::vector<int16_t> d = {-1, 2, -3, 4, -5, 0};
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::vector<int16_t>>(...)    ",
                                 glib2::Value::Create(d),
                                 "an");
                         });

    failures += run_test([]()
                         {
                             std::vector<uint32_t> d = {1, 2, 3, 4, 5};
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::vector<uint32_t>>(...)   ",
                                 glib2::Value::Create(d),
                                 "au");
                         });

    failures += run_test([]()
                         {
                             std::vector<int32_t> d = {-1, 2, -3, 4, -5, 0};
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::vector<int32_t>>(...)    ",
                                 glib2::Value::Create(d),
                                 "ai");
                         });

    failures += run_test([]()
                         {
                             std::vector<uint64_t> d = {1, 2, 3, 4, 5};
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::vector<uint64_t>>(...)   ",
                                 glib2::Value::Create(d),
                                 "at");
                         });

    failures += run_test([]()
                         {
                             std::vector<int64_t> d = {-1, 2, -3, 4, -5, 0};
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::vector<int64_t>>(...)    ",
                                 glib2::Value::Create(d),
                                 "ax");
                         });

    failures += run_test([]()
                         {
                             std::vector<double> d = {-1, 2, -3, 4, -5, 0};
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::vector<double>>(...)     ",
                                 glib2::Value::Create(d),
                                 "ad");
                         });

    failures += run_test([]()
                         {
                             std::vector<bool> d = {true, false, false, true};
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::vector<bool>>(...)       ",
                                 glib2::Value::Create(d),
                                 "ab");
                         });

    failures += run_test([]()
                         {
                             std::vector<std::string> d = {"line 1", "line 2", "line 3"};
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::vector<std::string>>(...)",
                                 glib2::Value::Create(d),
                                 "as");
                         });

    failures += run_test([]()
                         {
                             std::vector<DBus::Object::Path> d = {
                                 "/path_1", "/path_2", "/path/3"};
                             return check_data_type_gvariant(
                                 "glib2::Value::Create<std::vector<DBus::Object::Path>>(...)",
                                 glib2::Value::Create(d),
                                 "ao");
                         });

    failures += run_test([]()
                         {
                             return check_data_type_gvariant(
                                 "g_variant_new(\"(iuqntxdbys)\", ...)  ",
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
                         });

    failures += run_test([]()
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
                             return check_data_type_gvariant(
                                 "glib2::Builder::Create(\"(soibnuty)\")",
                                 glib2::Builder::Finish(b),
                                 "(soibnuty)");
                         });

    failures += run_test([]()
                         {
                             auto str_v = std::vector<std::string>{"string 1", "string B", "string III"};
                             auto int_v = std::vector<int32_t>{-103949501, 304958104, 10, 50, 0, -24, -88};
                             auto uint_v = std::vector<uint32_t>{0, 10293, 5029, 19586, 1};
                             GVariantBuilder *b = glib2::Builder::Create("(asaiau)");
                             glib2::Builder::Add(b, str_v);
                             glib2::Builder::Add(b, int_v);
                             glib2::Builder::Add(b, uint_v);
                             return check_data_type_gvariant(
                                 "glib2::Builder::Create(\"(asaiau)\")  ",
                                 glib2::Builder::Finish(b),
                                 "(asaiau)");
                         });

    failures += run_test([]()
                         {
                             GVariantBuilder *keyval = glib2::Builder::Create("a{qb}");
                             glib2::Builder::AddKeyValue<uint16_t, bool>(keyval, 1, true);
                             glib2::Builder::AddKeyValue<uint16_t, bool>(keyval, 2, false);
                             glib2::Builder::AddKeyValue<uint16_t, bool>(keyval, 3, false);
                             glib2::Builder::AddKeyValue<uint16_t, bool>(keyval, 4, true);

                             GVariant *data = glib2::Builder::Finish(keyval);
                             return TestResult("glib2::Builder::AddKeyValue - a{qb})",
                                               TestUtils::check_data_type("a{qb}", data)
                                                   && TestUtils::check_data_value(
                                                       "{1: true, 2: false, 3: false, 4: true}",
                                                       data));
                         });

    failures += run_test([]()
                         {
                             GVariantBuilder *keyval = glib2::Builder::Create("a{sq}");
                             glib2::Builder::AddKeyValue<std::string, uint16_t>(keyval, "key_one", 123);
                             glib2::Builder::AddKeyValue<std::string, uint16_t>(keyval, "key_two", 6789);
                             glib2::Builder::AddKeyValue<std::string, uint16_t>(keyval, "key_three", 65535);
                             glib2::Builder::AddKeyValue<std::string, uint16_t>(keyval, "key_four", -1039); // This will wrap

                             GVariant *data = glib2::Builder::Finish(keyval);
                             return TestResult("glib2::Builder::AddKeyValue - a{sq})",
                                               TestUtils::check_data_type("a{sq}", data)
                                                   && TestUtils::check_data_value(
                                                       "{'key_one': 123, 'key_two': 6789, 'key_three': 65535, 'key_four': 64497}",
                                                       data));
                         });

    failures += run_test([]()
                         {
                             GVariantBuilder *keyval = glib2::Builder::Create("a{us}");
                             glib2::Builder::AddKeyValue<uint32_t, std::string>(keyval, 10, "Key 10");
                             glib2::Builder::AddKeyValue<uint32_t, std::string>(keyval, 20, "Key 20");
                             glib2::Builder::AddKeyValue<uint32_t, std::string>(keyval, 30, "Key 30");

                             GVariant *data = glib2::Builder::Finish(keyval);
                             return TestResult("glib2::Builder::AddKeyValue - a{us})",
                                               TestUtils::check_data_type("a{us}", data)
                                                   && TestUtils::check_data_value(
                                                       "{10: 'Key 10', 20: 'Key 20', 30: 'Key 30'}",
                                                       data));
                         });

    failures += run_test([]()
                         {
                             GVariantBuilder *keyval = glib2::Builder::Create("a{ss}");
                             glib2::Builder::AddKeyValue<std::string, std::string>(keyval, "key_1", "Value A");
                             glib2::Builder::AddKeyValue<std::string, std::string>(keyval, "key_2", "Value B");
                             glib2::Builder::AddKeyValue<std::string, std::string>(keyval, "key_3", "Value C");

                             GVariant *data = glib2::Builder::Finish(keyval);
                             return TestResult("glib2::Builder::AddKeyValue - a{ss})",
                                               TestUtils::check_data_type("a{ss}", data)
                                                   && TestUtils::check_data_value(
                                                       "{'key_1': 'Value A', 'key_2': 'Value B', 'key_3': 'Value C'}",
                                                       data));
                         });

    failures += run_test([]()
                         {
                             GVariantBuilder *keyval = glib2::Builder::Create("a{so}");
                             glib2::Builder::AddKeyValue<std::string, DBus::Object::Path>(keyval, "pathA", "/example/gdbuspp/path1");
                             glib2::Builder::AddKeyValue<std::string, DBus::Object::Path>(keyval, "pathB", "/example/gdbuspp/path2");
                             glib2::Builder::AddKeyValue<std::string, DBus::Object::Path>(keyval, "pathC", "/example/gdbuspp/path3");

                             GVariant *data = glib2::Builder::Finish(keyval);
                             return TestResult("glib2::Builder::AddKeyValue - a{so})",
                                               TestUtils::check_data_type("a{so}", data)
                                                   && TestUtils::check_data_value(
                                                       "{'pathA': '/example/gdbuspp/path1', 'pathB': '/example/gdbuspp/path2', 'pathC': '/example/gdbuspp/path3'}",
                                                       data));
                         });



    failures += run_test([]()
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
                             return TestResult("glib2::Dict - Generic tests (a{sv})",
                                               TestUtils::check_data_type("a{sv}", data)
                                                   && TestUtils::check_data_value(
                                                       "{'type_bool_false': <false>, 'type_string': <'A dictionary string element'>, 'type_bool_true': <true>, "
                                                       "'type_int32': <uint64 9402038495>, 'type_object_path': <objectpath '/org/example/gdbuspp/object/path'>, "
                                                       "'vector_ui16': <[uint16 1, 2, 4, 8, 16, 32, 65]>}",
                                                       data));
                         });


    failures += run_test([]()
                         {
                             GVariantDict *dict = glib2::Dict::Create();
                             glib2::Dict::Add<int32_t>(dict, "type_int32", 123);
                             glib2::Dict::Add<uint64_t>(dict, "type_uint64", 9402038495);
                             glib2::Dict::Add<bool>(dict, "type_bool_false", false);
                             glib2::Dict::Add<bool>(dict, "type_bool_true", true);
                             glib2::Dict::Add<std::string>(dict, "type_string", "A dictionary string element");
                             glib2::Dict::Add<DBus::Object::Path>(dict, "type_object_path", "/org/example/gdbuspp/object/path");

                             std::vector<uint16_t> vector_ui16{1, 2, 4, 8, 16, 32, 65};
                             glib2::Dict::Add(dict, "vector_ui16", vector_ui16);
                             GVariant *data = glib2::Dict::Finish(dict);

                             bool result = false;
                             try
                             {
                                 glib2::Dict::IterateDictionary(
                                     data,
                                     [&vector_ui16](const std::string &key, GVariant *value)
                                     {
                                         // std::cout << "IterateDictionary: key='" << key << "' value: " << g_variant_print(value, true) << '\n';

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
                                             throw std::runtime_error("Incorrect value for 'type_string'");
                                         }
                                         else if ("vector_ui16" == key
                                                  && vector_ui16 != glib2::Value::ExtractVector<uint16_t>(value))
                                         {
                                             throw std::runtime_error("Incorrect value for 'vector_ui16'");
                                         }
                                     });
                                 result = true;
                             }
                             catch (const std::runtime_error &err)
                             {
                                 std::cerr << "    ** IterateDictionary exception: " << err.what() << '\n';
                             }
                             return TestResult("glib2::Dict::IterateDictionary", result);
                         });
    std::cout << ":: Base data type test failures: " << failures
              << std::endl
              << std::endl;
    return failures;
}



int test_base_vector()
{
    std::cout << ":: Testing vectorized base data types ..." << std::endl;
    int failures = 0;

    // std::byte
    failures += run_test([]()
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
                             return compare_vector("std::byte", d, res);
                         });

    // uint16_t
    failures += run_test([]()
                         {
                             std::vector<uint16_t> d = {430, 10439, 5039, 102, 0, 994};
                             GVariant *vector = glib2::Value::Create(d);
                             auto res = glib2::Value::ExtractVector<uint16_t>(vector);
                             return compare_vector("uint16_t", d, res);
                         });

    // int16_t
    failures += run_test([]()
                         {
                             std::vector<int16_t> d = {304, -103, 4, -32405, 304, 506};
                             GVariant *vector = glib2::Value::Create(d);
                             auto res = glib2::Value::ExtractVector<int16_t>(vector);
                             return compare_vector("int16_t", d, res);
                         });

    // uint32_t
    failures += run_test([]()
                         {
                             std::vector<uint32_t> d = {95817395, 103945850, 40913, 0, 59, 958274958};
                             GVariant *vector = glib2::Value::Create(d);
                             auto res = glib2::Value::ExtractVector<uint32_t>(vector);
                             return compare_vector("uint32_t", d, res);
                         });

    // int32_t
    failures += run_test([]()
                         {
                             std::vector<int32_t> d = {3049, 0, -1049581045, -309185019, 6610495, 86371, 9483, 6610495};
                             GVariant *vector = glib2::Value::Create(d);
                             auto res = glib2::Value::ExtractVector<int32_t>(vector);
                             return compare_vector("int32_t", d, res);
                         });

    // uint64_t
    failures += run_test([]()
                         {
                             std::vector<uint64_t> d = {8476164390, 1034985710104950195, 0, 10495871};
                             GVariant *vector = glib2::Value::Create(d);
                             auto res = glib2::Value::ExtractVector<uint64_t>(vector);
                             return compare_vector("uint64_t", d, res);
                         });

    // int64_t
    failures += run_test([]()
                         {
                             std::vector<int64_t> d = {-1043958104856, 309858106037, -5098282759120942699, 999999999999999999, 19485601895, 0, 2984585, 104959185, -999999999999999999};
                             GVariant *vector = glib2::Value::Create(d);
                             auto res = glib2::Value::ExtractVector<int64_t>(vector);
                             return compare_vector("int64_t", d, res);
                         });

    // double
    failures += run_test([]()
                         {
                             std::vector<double> d = {1.0, -0.4, 3.14159267, 0, 4958194920958193.9471683, -9482748750.22593};
                             GVariant *vector = glib2::Value::Create(d);
                             auto res = glib2::Value::ExtractVector<double>(vector);
                             return compare_vector("double", d, res);
                         });

    // bool
    failures += run_test([]()
                         {
                             std::vector<bool> d = {false, true, 0, 1, false, false, true};
                             GVariant *vector = glib2::Value::Create(d);
                             auto res = glib2::Value::ExtractVector<bool>(vector);
                             return compare_vector("bool", d, res);
                         });

    // std::string
    failures += run_test([]()
                         {
                             std::vector<std::string> d = {"line 1", "line 2", "line 3"};
                             GVariant *vector = glib2::Value::Create(d);
                             auto res = glib2::Value::ExtractVector<std::string>(vector);
                             return compare_vector("std::string", d, res);
                         });

    // DBus::Object::Path
    failures += run_test([]()
                         {
                             std::vector<DBus::Object::Path> d = {
                                 "/net/openvpn/gdbus/object_path/test1",
                                 "/net/openvpn/gdbus/object_path/test2",
                                 "/net/openvpn/gdbus/object_path/test3",
                                 "/net/openvpn/gdbus/object_path/test4",
                                 "/net/openvpn/gdbus/object_path/test5"};
                             GVariant *vector = glib2::Value::Create(d);
                             auto res = glib2::Value::ExtractVector<DBus::Object::Path>(vector);
                             return compare_vector("DBus::Object::Path", d, res);
                         });


    std::cout << ":: Vectorized data type test failures: " << failures
              << std::endl
              << std::endl;
    return failures;
};

int main(int argc, char **argv)
{
    DBus::Proxy::Client::Ptr prx = nullptr;
    if ((argc > 1 && (std::string(argv[1]) == "--service-simple")))
    {
        auto conn = DBus::Connection::Create(DBus::BusType::SESSION);
        prx = DBus::Proxy::Client::Create(conn,
                                          Test::Constants::GenServiceName("simple"));
    }

    int failures = 0;
    if (!prx)
    {
        failures += test_base_data_types();
        failures += test_base_vector();
    }

    if (prx)
    {
        failures += static_cast<int>(test_dictionary(prx));
    }

    if (failures > 0)
    {
        std::cout << "OVERALL TEST RESULT:  FAIL "
                  << "(" << failures << " tests failed) " << std::endl;
        return 2;
    }
    return 0;
}
