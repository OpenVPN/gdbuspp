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
#include "../gdbuspp/object/path.hpp"
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


TestResult check_data_type_gvariant(const std::string msg,
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

    return TestResult(message.str(), pass);
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
                                 "glib2::Value::CreateVector<std::vector<std::byte>>(...)  ",
                                 glib2::Value::CreateVector(d),
                                 "ay");
                         });

    failures += run_test([]()
                         {
                             std::vector<int16_t> d = {-1, 2, -3, 4, -5, 0};
                             return check_data_type_gvariant(
                                 "glib2::Value::CreateVector<std::vector<int16_t>>(...)    ",
                                 glib2::Value::CreateVector(d),
                                 "an");
                         });

    failures += run_test([]()
                         {
                             std::vector<uint32_t> d = {1, 2, 3, 4, 5};
                             return check_data_type_gvariant(
                                 "glib2::Value::CreateVector<std::vector<uint32_t>>(...)   ",
                                 glib2::Value::CreateVector(d),
                                 "au");
                         });

    failures += run_test([]()
                         {
                             std::vector<int32_t> d = {-1, 2, -3, 4, -5, 0};
                             return check_data_type_gvariant(
                                 "glib2::Value::CreateVector<std::vector<int32_t>>(...)    ",
                                 glib2::Value::CreateVector(d),
                                 "ai");
                         });

    failures += run_test([]()
                         {
                             std::vector<uint64_t> d = {1, 2, 3, 4, 5};
                             return check_data_type_gvariant(
                                 "glib2::Value::CreateVector<std::vector<uint64_t>>(...)   ",
                                 glib2::Value::CreateVector(d),
                                 "at");
                         });

    failures += run_test([]()
                         {
                             std::vector<int64_t> d = {-1, 2, -3, 4, -5, 0};
                             return check_data_type_gvariant(
                                 "glib2::Value::CreateVector<std::vector<int64_t>>(...)    ",
                                 glib2::Value::CreateVector(d),
                                 "ax");
                         });

    failures += run_test([]()
                         {
                             std::vector<double> d = {-1, 2, -3, 4, -5, 0};
                             return check_data_type_gvariant(
                                 "glib2::Value::CreateVector<std::vector<double>>(...)     ",
                                 glib2::Value::CreateVector(d),
                                 "ad");
                         });

    failures += run_test([]()
                         {
                             std::vector<bool> d = {true, false, false, true};
                             return check_data_type_gvariant(
                                 "glib2::Value::CreateVector<std::vector<bool>>(...)       ",
                                 glib2::Value::CreateVector(d),
                                 "ab");
                         });

    failures += run_test([]()
                         {
                             std::vector<std::string> d = {"line 1", "line 2", "line 3"};
                             return check_data_type_gvariant(
                                 "glib2::Value::CreateVector<std::vector<std::string>>(...)",
                                 glib2::Value::CreateVector(d),
                                 "as");
                         });

    failures += run_test([]()
                         {
                             std::vector<DBus::Object::Path> d = {
                                 "/path_1", "/path_2", "/path/3"};
                             return check_data_type_gvariant(
                                 "glib2::Value::CreateVector<std::vector<DBus::Object::Path>>(...)",
                                 glib2::Value::CreateVector(d),
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

    std::cout << ":: Base data type test failures: " << failures
              << std::endl
              << std::endl;
    return failures;
}



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
