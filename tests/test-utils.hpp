//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   test-utils.hpp
 *
 * @brief  Shared classes and functions used by various test utilities
 *         and tools
 */

#pragma once

#include <string>
#include <vector>
#include <glib.h>

namespace Tests::Utils {

class Exception : public std::exception
{
  public:
    Exception(const std::string &group, const std::string &err);

    virtual const char *what() const noexcept;

  private:
    const std::string errormsg;
};


/**
 *  Provide a simple tool for generating --help screen via
 *  getopt() based implementations
 */
class OptionParser
{
  public:
    OptionParser() = default;
    virtual ~OptionParser() noexcept = default;

  protected:
    void help(const std::string &argv0, struct option options[]);
};


/**
 *  Dumps a human readable string of both the data type and value
 *  contained in a GVariant object.  This is just a wrapper around
 *  a couple of helper functions found in the glib2 library
 *
 * @param log     std::ostringstream where to the generated output is put
 * @param prefix  std::string containing a prefix before each written line of text
 * @param data    GVariant * to object to parse
 */
void dump_gvariant(std::ostringstream &log, const std::string &prefix, GVariant *data);

/**
 *  Checks the data type of a GVariant object against an expected data type
 *
 * @param expect_type  std::string with the complete representation of the
 *                     D-Bus data type which is expected to be found
 * @param  data        GVariant * to object to check
 * @returns Returns true when the expected type string is found, otherwise false
 */
bool check_data_type(const std::string &expect_type, GVariant *data);

/**
 *  Checks the data contents of a GVariant object against an expected
 *  value.  The format of this string must be formatted in the same way
 *  glib2::Utils::RetrieveContent() (g_variant_print()) formats the data.
 *
 * @param expect_value std::string with the complete representation of the
 *                     value which is expected to be found
 * @param  data        GVariant * to object to check
 * @returns Returns true when the expected value is found, otherwise false
 */
bool check_data_value(const std::string &expect_value, GVariant *data);

/**
 *  Test and log the data type and value of a GVariant object.
 *  This calls check_data_type() and check_data_value() under the hood.
 *  See those functions for details on the expected format of the type and
 *  value arguments.
 *
 * @param log    std::ostringstream where the results are written
 * @param type   std::string with the expected data type string
 * @param value  std::string with the expected data value
 * @param data   GVariant * to object to validate
 * @returns On successful validation, true is returned. Otherwise false
 */
bool log_data_type_value_check(std::ostringstream &log,
                               const std::string &type,
                               const std::string &value,
                               GVariant *data);

/**
 *  Helper function to generate GVariant object of specific types where the
 *  value is passed to this function as a std::string.
 *
 *  This function will "cast" the std::string value content into the
 *  appropriate D-Bus type and the related data format for the value.
 *
 * @param type        D-Bus data type to generate
 * @param value       std::string value to "cast" to the D-Bus type
 * @return GVariant*
 */
GVariant *convert_to_gvariant(const std::string &type, const std::string &value);

/**
 *  Convert a GVariant result of more values into a basic std::vector<std::string>
 *  All data types are converted into a std::string reprentation, regardless
 *  of their initial data type.
 *
 * @param values   GVariant with all the values to process
 * @return std::vector<std::string>
 */
std::vector<std::string> convert_from_gvariant(GVariant *values);

/**
 *  Generate a full GVariant object from a set of input strings and list
 *  of data types.
 *
 *  The data_values are provided through an array of strings and the
 *  number of elements must match the data type list (a single string)
 *
 *  If just a single value is present in the list, the output will be
 *  a single value in a GVariant * unless the wrap_single_value flag is
 *  true, then the single value will be inside a tuple container.
 *
 * @param log                std::ostringstream where to put log data
 * @param data_type          std::string of the complete data type of the data set
 * @param data_values        std::vector<string> with all the data to parse
 * @param wrap_single_value  bool flag, enables tuple wrapping if only a single
 *                           variable is being processed
 * @return GVariant* with all the data values converted in proper D-Bus values
 */
GVariant *generate_gvariant(std::ostringstream &log,
                            const std::string &data_type,
                            const std::vector<std::string> &data_values,
                            bool wrap_single_value);

}; // namespace Tests::Utils
