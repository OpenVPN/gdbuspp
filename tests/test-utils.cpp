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
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <getopt.h>
#include <glib.h>

#include "gdbuspp/glib2/utils.hpp"
#include "test-utils.hpp"

namespace TestUtils {

Exception::Exception(const std::string &group, const std::string &err)
    : errormsg("[" + group + "]: " + err)
{
}

const char *Exception::what() const noexcept
{
    return errormsg.c_str();
}



void OptionParser::help(const std::string argv0, struct option options[])
{
    std::cout << "Usage: " << argv0 << " <options>" << std::endl
              << std::endl
              << "Options:" << std::endl;

    int idx = 0;
    bool done = false;
    while (!done)
    {
        struct option *opt = &options[idx];
        if (opt->name)
        {
            if (opt->val > 0)
            {
                std::cout << "  -" << char(opt->val) << " |";
            }
            else
            {
                std::cout << "      ";
            }
            std::cout << " --" << opt->name;
            switch (opt->has_arg)
            {
            case required_argument:
                std::cout << " <arg>";
                break;
            case optional_argument:
                std::cout << " [arg]";
                break;
            default:
                break;
            }
            std::cout << std::endl;
            ++idx;
        }
        else
        {
            done = true;
        }
    }
    std::cout << std::endl;
}


void dump_gvariant(std::ostringstream &log, const std::string &prefix, GVariant *data)
{
    log << prefix << " type: " << g_variant_get_type_string(data) << std::endl;
    char *dump = g_variant_print(data, false);
    log << prefix << " data: " << std::string(dump) << std::endl;
    free(dump);
}


bool check_data_type(const std::string expect_type, GVariant *data)
{
    std::string gv_type(g_variant_get_type_string(data));
    return gv_type == expect_type;
}


bool check_data_value(const std::string expect_value, GVariant *data)
{
    std::string gv_value(g_variant_print(data, false));
    return gv_value == expect_value;
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
                return glib2::Value::Create("b", boolval);
            }
        case 'd':
            return glib2::Value::Create("d", std::stod(value));
        case 'i':
        case 'h':
        case 'n':
            return glib2::Value::Create(type.substr(0).c_str(), std::stoi(value));
        case 't':
            return glib2::Value::Create("t", stoul(value));
        case 'q':
        case 'u':
        case 'y':
            return glib2::Value::Create(type.substr(0).c_str(), stou(value));
        case 'x':
            return glib2::Value::Create("x", stol(value));
        case 's':
        default:
            return glib2::Value::Create("s", value);
        }
    }
    catch (const std::out_of_range &)
    {
        throw TestUtils::Exception(__func__,
                                   "Type '" + type + "' "
                                       + "with value '" + value + "' "
                                       + "exceeds the range for the data type");
    }
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
                throw TestUtils::Exception("TestUtils::convert_from_gvariant",
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
            throw TestUtils::Exception(__func__, "data values requires data types");
        }
        if (data_type.length() != data_values.size())
        {
            throw TestUtils::Exception(__func__, "data type string does not contain enough field to describe all data values");
        }

        log << "-------------------------" << std::endl;
        log << "Data values:" << std::endl;
        if (!wrap_single_value && data_values.size() == 1)
        {
            data = convert_to_gvariant(data_type, data_values[0]);
        }
        else
        {
            GVariantBuilder *bld = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
            uint32_t pos = 0;
            for (const auto &v : data_values)
            {
                std::string type = data_type.substr(pos, 1);
                g_variant_builder_add_value(bld, convert_to_gvariant(type, v));
                log << "   " << type
                    << " [" << v << "]" << std::endl;
                ++pos;
            }
            log << std::endl;
            data = g_variant_builder_end(bld);
            g_variant_builder_unref(bld);
        }

        dump_gvariant(log, "GVariant data", data);
        log << "-------------------------" << std::endl
            << std::endl;
    }
    return data;
}


}; // namespace TestUtils
