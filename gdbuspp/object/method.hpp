//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Charlie Vigue <charlie.vigue@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   object/method.hpp
 *
 * @brief  Declaration of the DBus::Object::Method related classes
 *         This adds the glue layer for a DBus::Object::Base based object
 *         to track D-Bus exposed methods and their API and the C++
 *         callback function being executed in the running D-Bus service.
 */

#pragma once

#include <functional>
#include <memory>
#include <sstream>

#include "../async-process.hpp"
#include "exceptions.hpp"


namespace DBus {
namespace Object {
namespace Method {

// Internal class used by the Callback class
class CallbackArguments; // Forward declaration; declared in method.cpp


class Exception : public DBus::Exception
{
  public:
    Exception(const std::string &errm, GError *gliberr = nullptr);
};


/**
 *  Internal container keeping a map of argument variables and their
 *  data type
 */
struct _method_argument
{
    /**
     *  Store a new argument and argument type
     *
     * @param argname  std::string with method argument name
     * @param argtype  std::string with method argument data type
     */
    _method_argument(const std::string &argname, const std::string &argtype)
        : name(argname), dbustype(argtype)
    {
    }
    ~_method_argument() noexcept = default;

    const std::string name;     ///< Method argument name
    const std::string dbustype; ///< D-Bus representation of argument data type
};


/**
 *  Used to flag if a D-Bus method is expected to send or receive a
 *  file descriptor to/from the caller
 */
enum class PassFDmode
{
    NONE,
    SEND,
    RECEIVE
};


/**
 *  A collection of input and output arguments a D-Bus method implements
 *  This argument collect will also generate the D-Bus introspection data
 *  based on how the API is setup.
 */
class Arguments
{
  public:
    using Ptr = std::shared_ptr<Arguments>;

    ~Arguments() noexcept = default;


    /**
     *  Registers a new input argument, which is expected to be received
     *  together with the D-Bus method call
     *
     * @param name     std::string containing the name of the argument
     * @param dbustype D-Bus specific variable data type of the argument
     */
    void AddInput(const std::string &name, const std::string &dbustype);

    /**
     *  Registers a new output argument, which the method callback function
     *  will return back to the D-Bus caller.
     *
     * @param name     std::string containing the name of the argument
     * @param dbustype D-Bus specific variable data type of the argument
     */
    void AddOutput(const std::string &name, const std::string &dbustype);

    /**
     *  Sets the file descriptor method mode for this D-Bus method
     *
     *  This enables sending or receiving a file descriptor to/from
     *  the D-Bus method caller.
     *
     *  The default mode is PassFDmode::NONE, which does not allow
     *  any kind of file descriptor passing.
     *
     * @param mode   PassFDmode for the D-Bus method
     */
    void PassFileDescriptor(const PassFDmode &mode);

    /**
     *  Checks if any arguments has been declared or not
     *
     * @returns true if no input or output arguments has been declared;
     *          otherwise false.
     */
    const bool empty() const;


    //
    //  Methods below will only be functional and useful when a
    //  D-Bus proxy client is performing operations against a D-Bus object
    //

    /**
     *  Retrieve the GVariant object containing all the arguments
     *  sent with the method call.
     *
     *  This is only available in the callback functor when a
     *  a D-Bus proxy client has called this method.
     *
     * @return GVariant* to the object with all incoming arguments
     */
    GVariant *GetMethodParameters() const noexcept;

    /**
     *  Retrieve the file descriptor the caller should have sent
     *  with the D-Bus method call.
     *
     *  This requires PassFileDescriptor() to have been called setting
     *  the file descriptor passing mode to PassFDmode::RECEIVE when
     *  declaring the D-Bus method.
     *
     *  This is only available in the callback functor when a
     *  a D-Bus proxy client has called this method.
     *
     * @return int  File descriptor provided by the caller
     */
    int ReceiveFD() const;


    /**
     *  Send a file descriptor back to the D-Bus method caller
     *
     *  This requires PassFileDescriptor() to have been called setting
     *  the file descriptor passing mode to PassFDmode::SEND when
     *  declaring the D-Bus method.
     *
     *  This is only available in the callback functor when a
     *  a D-Bus proxy client has called this method.
     *
     * @param fd  File descriptor to pass back to the caller
     */
    void SendFD(int &fd);


    /**
     *  Provide the method results back to the D-Bus method caller
     *
     *  This is only available in the callback functor when a
     *  a D-Bus proxy client has called this method.
     *
     * @param result  GVariant object with all the value arguments
     *                to be returned to the method caller
     */
    void SetMethodReturn(GVariant *result) noexcept;


    friend std::ostream &operator<<(std::ostream &os, const Arguments::Ptr &args)
    {
        std::ostringstream ret;
        ret << "Arguments("
            << "sender=" << args->sender << ", "
            << "call_params_type='" << g_variant_get_type_string(args->call_params) << "', "
            << "pass_fd_mode=" << args->PassFDmodeStr()
            << ")";

        return os << ret.str();
    }


  protected:
    std::string sender = {};
    GVariant *call_params = nullptr;
    GVariant *return_params = nullptr;
    PassFDmode pass_fd_mode = PassFDmode::NONE;
    int fd_receive = -1;
    int fd_send = -1;


    /**
     *  Validates the D-Bus method call arguments as received by the
     *  glib2 callback function against the list of expected arguments
     *  the method is declared to use.
     *
     * @param params  GVariant * with all the arguments passed by the
     *                D-Bus caller
     */
    void ValidateInputType(GVariant *params) const;

    /**
     *  Validates the result data types the method is expected to return
     *  back to the D-Bus method caller.
     *
     * @param params  GVariant * pointing at the data being sent back to
     *                the D-Bus caller
     */
    void ValidateOutputType(GVariant *params) const;

    /**
     *  Generates the XML snippet tags of all declared arguments to a method
     *
     * @return std::string containing the XML framgment
     */
    const std::string GenerateIntrospection() const;


    /**
     *  Arguments constructor;
     */
    Arguments() = default;

  private:
    std::vector<struct _method_argument> input = {};  //<< Collection of all input arguments
    std::vector<struct _method_argument> output = {}; //<< Collection of all output arguments


    /**
     *  Provide a readable string of the Object::Method::PassFDmode value
     *
     * @return const std::string representation of PassFDmode
     */
    const std::string PassFDmodeStr() const
    {
        switch (pass_fd_mode)
        {
        case PassFDmode::NONE:
            return "PassFDmode::NONE";
        case PassFDmode::SEND:
            return "PassFDmode::SEND";
        case PassFDmode::RECEIVE:
            return "PassFDmode::RECEIVE";
        default:
            return "[INVALID]";
        }
    }
};



/**
 *  This is the API of callback functions when they are being triggered
 *  and executed by the D-Bus service
 */
using CallbackFnc = std::function<void(Arguments::Ptr args)>;



/**
 *  Declaration object for a single D-Bus method
 */
class Callback
{
  public:
    using Ptr = std::shared_ptr<Callback>;

    /**
     *  Create a new Method::Callback object mapping a method name with
     *  a callback functor
     *
     * @param method_name     std::string containing the D-Bus exposed method name
     * @param callback        CallbackFnc code being executed through the D-Bus service
     *
     * @return Callback::Ptr  Returns a shared_ptr<Callback> to the new callback mapping
     */
    static Callback::Ptr Create(const std::string &method_name,
                                CallbackFnc callback)
    {
        return Callback::Ptr(new Callback(method_name,
                                          callback));
    }

    ~Callback() noexcept = default;


    /**
     *  Retrieve access to the Arguments object related to this callback.
     *  This is needed to add input/output arguments to the callback method
     *
     * @return Arguments::Ptr
     */
    Arguments::Ptr GetArgsList() const;

    /**
     *  Generate the XML introspection fragment describing this callback
     *  method with all it's arguments
     *
     * @return std::string
     */
    const std::string GenerateIntrospection() const;

    /**
     *  Retrieve the D-Bus method name this callback is declared with
     *
     * @return const std::string
     */
    const std::string GetMethodName() const;

    /**
     *  Execute this callback method with all the needed arguments
     *  provided via the AsyncProcess::Request object
     *
     *  This method will validate both the incoming and outgoing arguments
     *  against the declared list of input and output arguments.
     *
     *  The AsyncProcess::Request object contains the method name and input
     *  arguments which is used to call the right callback functor with the
     *  D-Bus proxy caller's input arguments.
     *
     *  The callback functor will place any response back to the caller
     *  into the same request object.
     *
     * @param req         ASyncProcess::Request::Ptr containing all the
     *                    needed method call information
     *
     * @throws Method::Exception if argument validation fails
     */
    void Execute(AsyncProcess::Request::UPtr &req);


  private:
    const std::string method_name;                  ///< D-Bus exposed method name
    std::shared_ptr<CallbackArguments> method_args; ///< Argument collection for the method
    CallbackFnc callback_fn;                        ///< Callback function being executed

    Callback(const std::string &method_name_, CallbackFnc callback);
};



/**
 *  Collection of DBus::Method::Callback objects
 *
 *  All methods being declared for a DBus::Object::Base will be collected
 *  here and this will also have the main dispatch logic to execute the
 *  right callback method.
 */
class Collection
{
  public:
    using Ptr = std::shared_ptr<Collection>;

    /**
     *  Create a new DBus::Object::Method::Collection object
     *
     * @return Collection::Ptr
     */
    static Collection::Ptr Create()
    {
        return Collection::Ptr(new Collection);
    }
    ~Collection() noexcept = default;


    /**
     *  Add a new D-Bus method and map it with a callback function which
     *  will be executed when a D-Bus caller requests this method
     *
     * @param method_name      std::string with the D-Bus exposed method name
     * @param method_callback  Method::CallbackFnc callback function to execute
     *
     * @return Arguments::Ptr  Returns an Arguments object to be used to add
     *                         input and output arguments this method needs
     *                         and will provide back to the D-Bus caller
     */
    Arguments::Ptr AddMethod(const std::string &method_name,
                             Method::CallbackFnc method_callback);

    /**
     *  Generate the XML Introspection fragment containing all the
     *  declared D-Bus methods and their arguments in this collection
     *  object
     *
     * @return const std::string
     */
    const std::string GenerateIntrospection() const;

    /**
     *  Check if a method is declared or not
     *
     * @param method_name  std::string of the D-Bus exposed method name
     *
     * @return true if method is declared, otherwise false
     */
    const bool Exists(const std::string &method_name) const;

    /**
     *  Execute the D-Bus method based on the information provided in the
     *  AsyncProcess::Request object.
     *
     *  The AsyncProcess::Request object contains the method name and input
     *  arguments which is used to call the right callback functor with the
     *  D-Bus proxy caller's input arguments.
     *
     *  The callback functor will place any response back to the caller
     *  into the same request object.
     *
     * @param req    AsyncProcess::Request::Ptr with the method call details.
     */
    void Execute(AsyncProcess::Request::UPtr &req);

  private:
    Collection() = default;

    std::vector<Method::Callback::Ptr> methods;
};

} // namespace Method
} // namespace Object
} // namespace DBus
