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

#include <cinttypes>
#include <functional>
#include <string>
#include <vector>
#include <glib.h>

#include "../gdbuspp/proxy.hpp"

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
 *  Provide an infrastructure to run test cases
 *  and log the results.
 *
 *  WARNING: This object is not thread-safe and CANNOT be shared across
 *           multiple threads
 */
class ResultLog
{
  public:
    /**
     *  Initialise the result logger
     *
     * @param verbose  If true, log output will happen as the test runs.
     *                 Otherwise the log output may be collected uses
     *                 DumpLogs();  The DumpLogs() call will not dump
     *                 any data if the verbose flag is true.
     */
    ResultLog(bool verbose = false);

    /**
     *  Runs a functional test.
     *
     *  This will execute a provided test function which should not
     *  throw any exceptions for a successful test run.
     *
     *  If the test function depends on a DBus::Proxy::Client, the
     *  proxy client can be given as an extra argument, which is
     *  used to extract details for logging purposes only.
     *
     * @param descr       String containing a description of the test (for logging only)
     * @param proxy       DBus::Proxy::Client object, if used. May be nullptr. (for logging only)
     * @param test_func   std::function containing the code to run as a test. This function must
     *                    return true on success, otherwise false.
     * @param expect      Indicates the expected return value of the test function.  If this value
     *                    does not match the return value of test_func(), this test is logged as
     *                    failed.
     */
    void RunFunctionTest(const std::string &descr,
                         DBus::Proxy::Client::Ptr proxy,
                         std::function<bool()> &&test_func,
                         bool expect);

    /**
     *  Runs an exception test
     *
     *  This tests scenarios where the test function is expected to throw
     *  an exception.
     *
     * @param descr           String containing a description of the test (for logging only)
     * @param proxy           DBus::Proxy::Client object, if used. May be nullptr. (for logging only)
     * @param test_func       std::function containing the code to run as a test. This function must
     *                        throw an exception where the output of the std::exception::what() method
     *                        matches a list of expected error strings.
     * @param exception_list  std::vector<std::string> if error messages expected in the exception
     */
    void RunExceptionTest(const std::string &descr,
                          DBus::Proxy::Client::Ptr proxy,
                          std::function<void()> &&testfunc,
                          const std::vector<std::string> &exception_list);

    /**
     *   Adds a single log line to the log
     *
     * @param line        string contianing the log line to append
     */
    void AddLogLine(const std::string &line);


    /**
     *   Print the final test report summary.
     *
     *  This will print a summary of number of successful and failed tests
     *  which has been run.  In addition it will add an "overall test result"
     *  which will only get a "PASSED" flag if there were no errors.
     *
     * @return bool  Returns the overall test result, if true - all tests were
     *               successful and no errors were logged.
     */
    bool PrintTestReport();

    /**
     *   Dump all the collected log lines to stdout.
     *
     *   Will only provide output if the verbose flag to the
     *   constructor was false
     */
    void DumpLogs();

  private:
    struct TestResult;

    bool verbose = false;
    std::vector<std::string> testlog;
    std::vector<std::string> testcase_log;
    uint32_t success = 0;
    uint32_t failed = 0;
    bool running_test = false;

    TestResult run_test(bool expected, std::function<bool()> &&test_func);

    void log_result(TestResult &result,
                    DBus::Proxy::Client::Ptr proxy,
                    const std::string &msg);
};


/**
 *  Dumps a human readable string of both the data type and value
 *  contained in a GVariant object.  This is just a wrapper around
 *  a couple of helper functions found in the glib2 library
 *
 * @param log     ResultLog object where to the generated output is logged
 * @param prefix  std::string containing a prefix before each written line of text
 * @param data    GVariant * to object to parse
 */
void dump_gvariant(ResultLog &log, const std::string &prefix, GVariant *data);
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
 * @param log    ResultLog object where the results are logged
 * @param type   std::string with the expected data type string
 * @param value  std::string with the expected data value
 * @param data   GVariant * to object to validate
 * @returns On successful validation, true is returned. Otherwise false
 */
bool log_data_type_value_check(ResultLog &log,
                               const std::string &type,
                               const std::string &value,
                               GVariant *data);

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
 *  Parse a GVariant result into a single string with some very simple
 *  type of content formatting.
 *
 *  Dictionary content are enclosed in '{' and '}', arrays in '[' and ']'
 *  and structured data in '(' and ')'.
 *
 * @param value          GVariant object ro parse
 * @return std::string
 */
std::string parse_gvariant_to_string(GVariant *value);

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
 * @param log                ResultLog object where to put log data
 * @param data_type          std::string of the complete data type of the data set
 * @param data_values        std::vector<string> with all the data to parse
 * @param wrap_single_value  bool flag, enables tuple wrapping if only a single
 *                           variable is being processed
 * @return GVariant* with all the data values converted in proper D-Bus values
 */
GVariant *generate_gvariant(ResultLog &log,
                            const std::string &data_type,
                            const std::vector<std::string> &data_values,
                            bool wrap_single_value);

GVariant *generate_gvariant(std::ostringstream &log,
                            const std::string &data_type,
                            const std::vector<std::string> &data_values,
                            bool wrap_single_value);

}; // namespace Tests::Utils
