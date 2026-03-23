//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   test-utils.cpp
 *
 * @brief  Shared classes and functions used by various test utilities
 *         and tools
 */

#include <exception>
#include <limits>
#include <sstream>
#include <string>
#include <getopt.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <glib.h>

#include "gdbuspp/glib2/utils.hpp"
#include "test-utils.hpp"

namespace Tests::Utils {

Exception::Exception(const std::string &group, const std::string &err)
    : errormsg("[" + group + "]: " + err)
{
}

const char *Exception::what() const noexcept
{
    return errormsg.c_str();
}



void OptionParser::help(const std::string &argv0, struct option options[])
{
    fmt::print("Usage: {} <options>\n\nOptions:\n", argv0);

    int idx = 0;
    bool done = false;
    while (!done)
    {
        const struct option *opt = &options[idx];
        if (opt->name)
        {
            fmt::print("{}",
                       (opt->val > 0
                            ? fmt::format("  -{:c} |", opt->val)
                            : "       "));


            fmt::println(" --{}{}",
                         opt->name,
                         (opt->has_arg == required_argument
                              ? " <arg>"
                              : (opt->has_arg == optional_argument
                                     ? " [arg]"
                                     : "")));
            ++idx;
        }
        else
        {
            done = true;
        }
    }
    fmt::println("");
}


void dump_gvariant(std::ostringstream &log, const std::string &prefix, GVariant *data)
{
    log << prefix << " type: " << glib2::DataType::Extract(data) << std::endl;
    log << prefix << " data: " << glib2::Utils::DumpToString(data) << std::endl;
}


bool check_data_type(const std::string &expect_type, GVariant *data)
{
    std::string gv_type = glib2::DataType::Extract(data);
    bool ret = gv_type == expect_type;
    if (!ret)
    {
        fmt::print(stderr,
                   FMT_COMPILE("    ** Expected data type: |{}|\n"
                               "    ** Received data type: |{}|\n"),
                   expect_type,
                   gv_type);
    }
    return ret;
}


bool check_data_value(const std::string &expect_value, GVariant *data)
{
    std::string gv_value = glib2::Utils::DumpToString(data, false);
    bool ret = gv_value == expect_value;
    if (!ret)
    {
        fmt::print(stderr,
                   FMT_COMPILE("    ** Expected value: |{}|\n"
                               "    **  Data contents: |{}|\n"),
                   expect_value,
                   gv_value);
    }
    return ret;
}


bool log_data_type_value_check(std::ostringstream &log,
                               const std::string &type,
                               const std::string &value,
                               GVariant *data)
{
    bool ret = true;
    if (!type.empty())
    {
        bool r = check_data_type(type, data);
        log << "Checking expected data type: "
            << (r ? "Pass" : "Fail");
        if (!r)
        {
            log << "   Expected: " << type;
            ret = false;
        }
        log << std::endl;
    }
    if (!value.empty())
    {
        bool r = check_data_value(value, data);
        log << "Checking expected data value: "
            << (r ? "Pass" : "Fail");
        if (!r)
        {
            log << "   Expected: " << value;
            ret = false;
        }
        log << std::endl;
    }
    return ret;
}


/**
 *   A stou implementation missing in C++, based on stoul()
 */
unsigned stou(std::string const &str, size_t *idx = 0, int base = 10)
{
    unsigned long result = std::stoul(str, idx, base);
    if (result > std::numeric_limits<unsigned>::max())
    {
        throw std::out_of_range("stou");
    }
    return result;
}


GVariant *convert_to_gvariant(const std::string &type, const std::string &value)
{
    try
    {
        switch (type[0])
        {
        case 'b':
            {
                bool boolval = (value == "1" || value == "yes" || value == "true");
                return glib2::Value::CreateType("b", boolval);
            }
        case 'd':
            return glib2::Value::CreateType("d", std::stod(value));
        case 'i':
        case 'h':
        case 'n':
            return glib2::Value::CreateType(type.substr(0).c_str(), std::stoi(value));
        case 'o':
            return glib2::Value::CreateType(glib2::DataType::DBUS_TYPE_OBJECTPATH, value);
        case 't':
            return glib2::Value::CreateType("t", stoul(value));
        case 'q':
        case 'u':
        case 'y':
            return glib2::Value::CreateType(type.substr(0).c_str(), stou(value));
        case 'x':
            return glib2::Value::CreateType("x", stol(value));
        case 's':
        default:
            return glib2::Value::CreateType("s", value);
        }
    }
    catch (const std::out_of_range &)
    {
        throw Tests::Utils::Exception(__func__,
                                      "Type '" + type + "' "
                                          + "with value '" + value + "' "
                                          + "exceeds the range for the data type");
    }
}


std::string parse_gvariant_to_string(GVariant *value)
{
    std::string ret;
    std::string type = glib2::DataType::Extract(value);

    if ("a{sv}" == type)
    {
        ret = "{";
        glib2::Dict::IterateDictionary(
            value,
            [&ret](const std::string &key, GVariant *val)
            {
                ret += key + "='" + parse_gvariant_to_string(val) + "', ";
            });
        ret += "}";
        return ret;
    }


    switch (type[0])
    {
    case '(':
        ret = "(";
        for (const auto &val : convert_from_gvariant(value))
        {
            ret += val + ", ";
        }
        ret += ")";
        break;
    case ')':
        break;
    case 'a':
        ret = "[";
        glib2::Value::IterateArray(value, [&ret](GVariant *val)
                                   {
                                       ret += "'" + parse_gvariant_to_string(val) + "', ";
                                   });
        ret += "]";
        break;
    case 'b':
        ret = (glib2::Value::Get<bool>(value) ? "true" : "false");
        break;
    case 'd':
        ret = std::to_string(glib2::Value::Get<double>(value));
        break;
    case 'i':
    case 'h':
        ret = std::to_string(glib2::Value::Get<int32_t>(value));
        break;
    case 'n':
        ret = std::to_string(glib2::Value::Get<int16_t>(value));
        break;
    case 'o':
        ret = glib2::Value::Get<DBus::Object::Path>(value);
        break;
    case 't':
        ret = std::to_string(glib2::Value::Get<uint64_t>(value));
        break;
    case 'q':
        ret = std::to_string(glib2::Value::Get<uint16_t>(value));
        break;
    case 'u':
        ret = std::to_string(glib2::Value::Get<uint32_t>(value));
        break;
    case 'x':
        ret = std::to_string(glib2::Value::Get<int64_t>(value));
        break;
    case 'y':
        ret = std::to_string(std::to_integer<uint8_t>(glib2::Value::Get<std::byte>(value)));
        break;
    case 's':
        ret = glib2::Value::Get<std::string>(value);
        break;
    default:
        ret = "<UNKNOWN D-BUS DATA TYPE: " + type + ">";
        break;
    }
    return ret;
}


std::vector<std::string> convert_from_gvariant(GVariant *values)
{
    std::vector<std::string> ret{};
    std::string type(g_variant_get_type_string(values));
    int field = 0;
    for (size_t pos = 0; pos < type.length(); pos++)
    {
        std::string v{};
        switch (type[pos])
        {
        case '(':
        case ')':
            continue;
        case 'b':
            v = (glib2::Value::Extract<bool>(values, field) ? "true" : "false");
            break;
        case 'd':
            v = std::to_string(glib2::Value::Extract<double>(values, field));
            break;
        case 'i':
        case 'h':
            v = std::to_string(glib2::Value::Extract<int32_t>(values, field));
            break;
        case 'n':
            v = std::to_string(glib2::Value::Extract<int16_t>(values, field));
            break;
        case 'o':
            v = glib2::Value::Extract<DBus::Object::Path>(values, field);
            break;
        case 't':
            v = std::to_string(glib2::Value::Extract<uint64_t>(values, field));
            break;
        case 'q':
            v = std::to_string(glib2::Value::Extract<uint16_t>(values, field));
            break;
        case 'u':
            v = std::to_string(glib2::Value::Extract<uint32_t>(values, field));
            break;
        case 'x':
            v = std::to_string(glib2::Value::Extract<int64_t>(values, field));
            break;
        case 's':
            v = glib2::Value::Extract<std::string>(values, field);
            break;
        default:
            {
                std::ostringstream err;
                err << "D-Bus data type '" << type[pos + 1] << "'"
                    << " has not been implemented.";
                throw Tests::Utils::Exception("Tests::Utils::convert_from_gvariant",
                                              err.str());
            }
        }

        if (!v.empty())
        {
            ret.push_back(v);
            ++field;
        }
    }
    return ret;
}



GVariant *generate_gvariant(std::ostringstream &log,
                            const std::string &data_type,
                            const std::vector<std::string> &data_values,
                            bool wrap_single_value)
{
    GVariant *data = nullptr;
    if (data_values.size() > 0)
    {
        if (data_type.empty())
        {
            throw Tests::Utils::Exception(__func__, "data values requires data types");
        }

        if (data_type[0] != 'a' && data_type.length() != data_values.size())
        {
            throw Tests::Utils::Exception(__func__, "data type string does not contain enough field to describe all data values");
        }

        log << "-------------------------" << std::endl;
        log << "Data values:" << std::endl;
        if (!wrap_single_value && data_values.size() == 1)
        {
            data = convert_to_gvariant(data_type, data_values[0]);
        }
        else if (data_type[0] == 'a')
        {
            if (data_type.length() > 2)
            {
                throw Tests::Utils::Exception(__func__, "only arrays with a single data type is supported");
            }

            log << "   " << data_type << " [";
            GVariantBuilder *bld = glib2::Builder::Create(data_type.c_str());
            for (const auto &v : data_values)
            {
                glib2::Builder::Add(bld, convert_to_gvariant(data_type.substr(1, 1), v));
                log << "'" << v << "',";
            }
            log << "]" << std::endl;
            data = glib2::Builder::Finish(bld);
        }
        else
        {
            GVariantBuilder *bld = glib2::Builder::EmptyTuple();
            uint32_t pos = 0;
            for (const auto &v : data_values)
            {
                std::string type = data_type.substr(pos, 1);
                glib2::Builder::Add(bld, convert_to_gvariant(type, v));
                log << "   " << type
                    << " [" << v << "]" << std::endl;
                ++pos;
            }
            log << std::endl;
            data = glib2::Builder::Finish(bld);
        }

        dump_gvariant(log, "GVariant data", data);
        log << "-------------------------" << std::endl
            << std::endl;
    }
    return data;
}


}; // namespace Tests::Utils
