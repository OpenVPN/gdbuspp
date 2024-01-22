//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Charlie Vigue <charlie.vigue@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   object/method.cpp
 *
 * @brief  Implementation of the DBus::Object::Method scope
 *         This adds the glue layer for a DBus::Object::Base based object
 *         to track D-Bus exposed methods and their API and the C++
 *         callback function being executed in the running D-Bus service.
 */

#include "../features/debug-log.hpp"
#include "../glib2/utils.hpp"
#include "method.hpp"

namespace DBus {
namespace Object {
namespace Method {



///////////////////////////////////////////////////////////////////////////
//
//  class Method::CallbackArguments - internal class
//
///////////////////////////////////////////////////////////////////////////



/**
 *  This is a internal class wrapping the public interface of Arguments.
 *  This is used to extend the internal processing with access to some
 *  members and methods only to be used internally in GDBus++
 */
class CallbackArguments : public Arguments
{
  public:
    using Ptr = std::shared_ptr<CallbackArguments>;
    /**
     *  Creates a new Arguments collection used by one specific method
     *  declaration.  This is normally called by
     *  @DBus::Object::Method::Collection::AddMethod()
     *
     * @return Arguments::Ptr - aka std::shared_ptr<Arguments> object
     */
    static CallbackArguments::Ptr Create()
    {
        return CallbackArguments::Ptr(new CallbackArguments());
    }

    ~CallbackArguments() noexcept = default;

    void SetRequestInfo(AsyncProcess::Request::UPtr &req);

    void PrepareResponse(AsyncProcess::Request::UPtr &req);

    GVariant *GetReturnArgs() const noexcept;

    /**
     *  Generates the XML snippet tags of all declared arguments to a method
     *
     * @return std::string containing the XML framgment
     */
    const std::string GenerateIntrospection() const;

  private:
    CallbackArguments() = default;
};



///////////////////////////////////////////////////////////////////////////
//
//  class Method::Exception
//
///////////////////////////////////////////////////////////////////////////



Exception::Exception(const std::string &errm, GError *gliberr)
    : DBus::Exception("DBus::Object::Method", errm, gliberr)
{
}


///////////////////////////////////////////////////////////////////////////
//
//  class Method::Arguments - internal structs and functions
//
///////////////////////////////////////////////////////////////////////////

/**
 *  Generate a D-Bus data type format string based on the argument list
 *  being provided
 *
 * @param arglist
 * @return const std::string
 */
static const std::string _arguments_gen_dbus_type(std::vector<struct _method_argument> arglist)
{
    std::ostringstream ret;
    ret << "(";
    for (const auto &arg : arglist)
    {
        ret << arg.dbustype;
    }
    ret << ")";
    return ret.str();
}


/**
 *  Internal generic argument list data type validation against the
 *  data variables provided in a GVariant * object.
 *
 * @param arglist
 * @param params
 */
static void _arguments_validate_arguments(std::vector<struct _method_argument> arglist, GVariant *params)
{
    if (0 == arglist.size() && nullptr == params)
    {
        // This is fine; if no input arguments are expected
        // AND the input parameters are nullptr - no input arguments
        // are passed
        return;
    }
    std::string expect = _arguments_gen_dbus_type(arglist);
    glib2::Utils::checkParams(__func__, params, expect.c_str(), arglist.size());
}



///////////////////////////////////////////////////////////////////////////
//
//  class Method::Arguments
//
///////////////////////////////////////////////////////////////////////////



void Arguments::AddInput(const std::string &name, const std::string &dbustype)
{
    input.push_back({name, dbustype});
}


void Arguments::AddOutput(const std::string &name, const std::string &dbustype)
{
    output.push_back({name, dbustype});
}


void Arguments::PassFileDescriptor(const PassFDmode &mode)
{
    if (PassFDmode::NONE != pass_fd_mode)
    {
        throw Method::Exception("File descriptor passing mode cannot be modified");
    }
    pass_fd_mode = mode;
}


void Arguments::ValidateInputType(GVariant *params) const
{
    try
    {
        _arguments_validate_arguments(input, params);
    }
    catch (const glib2::Utils::Exception &excp)
    {
        throw Method::Exception(excp.GetRawError());
    }
}


void Arguments::ValidateOutputType(GVariant *params) const
{
    try
    {
        _arguments_validate_arguments(output, params);
    }
    catch (const glib2::Utils::Exception &excp)
    {
        throw Method::Exception(std::string("Unexpected return type - ")
                                + excp.GetRawError());
    }
}


GVariant *Arguments::GetMethodParameters() const noexcept
{
    return call_params;
}


int Arguments::ReceiveFD() const
{
    if (PassFDmode::RECEIVE != pass_fd_mode)
    {
        throw Method::Exception("Method is not setup up for receiving file descriptors");
    }
    return fd_receive;
}


void Arguments::SetMethodReturn(GVariant *result) noexcept
{
    return_params = result;
}


void Arguments::SendFD(int &fd)
{
    if (PassFDmode::SEND != pass_fd_mode)
    {
        throw Method::Exception("Method is not setup up for sending file descriptors");
    }
    fd_send = fd;
}


const std::string Arguments::GetCallerBusName() const noexcept
{
    return sender;
}


const bool Arguments::empty() const
{
    return input.empty() && output.empty();
}


const std::string Arguments::GenerateIntrospection() const
{
    std::ostringstream ret;
    for (const auto &ia : input)
    {
        ret << " <arg type='" << ia.dbustype << "'"
            << " name='" << ia.name << "' "
            << " direction='in'/>" << std::endl;
    }
    for (const auto &oa : output)
    {
        ret << " <arg type='" << oa.dbustype << "'"
            << " name='" << oa.name << "' "
            << " direction='out'/>" << std::endl;
    }
    return ret.str();
}



///////////////////////////////////////////////////////////////////////////
//
//  class Method::CallbackArguments
//
///////////////////////////////////////////////////////////////////////////



void CallbackArguments::SetRequestInfo(AsyncProcess::Request::UPtr &req)
{
    ValidateInputType(req->params);
    call_params = req->params;
    sender = req->sender;

    if (PassFDmode::RECEIVE == pass_fd_mode)
    {
        glib2::Utils::CheckCapabilityFD(req->dbusconn);

        // It might be the file descriptor is available in the D-Bus invocation
        // object; attempt to retrieve it from there first
        GDBusMessage *dmsg = g_dbus_method_invocation_get_message(req->invocation);
        GUnixFDList *fdlist = g_dbus_message_get_unix_fd_list(dmsg);
        if (fdlist != nullptr)
        {
            GError *error = nullptr;
            fd_receive = g_unix_fd_list_get(fdlist, 0, &error);
            if (fd_receive < 0 || error)
            {
                throw Object::Exception(req->object,
                                        "Could not retrieve file descriptors from D-Bus call",
                                        error);
            }
        }
    }
}


void CallbackArguments::PrepareResponse(AsyncProcess::Request::UPtr &req)
{
    ValidateOutputType(return_params);

    if (PassFDmode::SEND == pass_fd_mode && -1 < fd_send)
    {
        glib2::Utils::CheckCapabilityFD(req->dbusconn);

        GUnixFDList *fdlist = g_unix_fd_list_new();
        if (!fdlist)
        {
            throw Object::Exception(req->object,
                                    "Failed allocating file descriptor return list");
        }
        GError *error = nullptr;
        if (g_unix_fd_list_append(fdlist, fd_send, &error) < 0)
        {
            throw Object::Exception(req->object,
                                    "Failed preparing file descriptor return list",
                                    error);
        }
        close(fd_send);
        g_dbus_method_invocation_return_value_with_unix_fd_list(req->invocation,
                                                                return_params,
                                                                fdlist);
        glib2::Utils::unref_fdlist(fdlist);
    }
    else
    {
        g_dbus_method_invocation_return_value(req->invocation, return_params);
    }
}


GVariant *CallbackArguments::GetReturnArgs() const noexcept
{
    return return_params;
}


const std::string CallbackArguments::GenerateIntrospection() const
{
    return Arguments::GenerateIntrospection();
}



///////////////////////////////////////////////////////////////////////////
//
//  class Method::Callback
//
///////////////////////////////////////////////////////////////////////////



Arguments::Ptr Callback::GetArgsList() const
{
    return method_args;
}


const std::string Callback::GenerateIntrospection() const
{
    std::ostringstream ret;
    if (method_args->empty())
    {
        ret << "<method name='" << method_name << "'/>" << std::endl;
    }
    else
    {
        ret << "<method name='" << method_name << "'>" << std::endl;
        ret << method_args->GenerateIntrospection();
        ret << "</method>" << std::endl;
    }
    return ret.str();
}


const std::string Callback::GetMethodName() const
{
    return method_name;
}


void Callback::Execute(AsyncProcess::Request::UPtr &req)
{
    method_args->SetRequestInfo(req);
    callback_fn(static_cast<Arguments::Ptr>(method_args));

#ifdef GDBUSPP_INTERNAL_DEBUG
    GVariant *result = method_args->GetReturnArgs();
    GDBUSPP_LOG("Callback::Execute (return) - "
                << req << " - Result: "
                << (result ? g_variant_print(result, true) : "(n/a)"));
#endif

    method_args->PrepareResponse(req);
}


Callback::Callback(const std::string &method_name_, CallbackFnc callback)
    : method_name(method_name_),
      callback_fn(callback)
{
    method_args = CallbackArguments::Create();
}



///////////////////////////////////////////////////////////////////////////
//
//  class Method::Collection
//
///////////////////////////////////////////////////////////////////////////



Arguments::Ptr Collection::AddMethod(const std::string &method_name,
                                     Method::CallbackFnc method_callback)
{
    Callback::Ptr meth = Callback::Create(method_name,
                                          method_callback);
    methods.push_back(meth);
    return meth->GetArgsList();
}


const std::string Collection::GenerateIntrospection() const
{
    std::ostringstream xml;
    for (const auto &meth : methods)
    {
        xml << meth->GenerateIntrospection();
    }
    return xml.str();
}


const bool Collection::Exists(const std::string &method_name) const
{
    const auto it = std::find_if(methods.begin(),
                                 methods.end(),
                                 [method_name](const Method::Callback::Ptr &ep)
                                 {
                                     return method_name == ep->GetMethodName();
                                 });
    return (methods.end() != it);
}


void Collection::Execute(AsyncProcess::Request::UPtr &req)
{
    auto it = std::find_if(methods.begin(),
                           methods.end(),
                           [method_name = req->method](Method::Callback::Ptr &meth)
                           {
                               return method_name == meth->GetMethodName();
                           });
    if (methods.end() == it)
    {
        throw Method::Exception("Method '" + req->method + "' does not exist");
    }
    it->get()->Execute(req);
}

} // namespace Method
} // namespace Object
} // namespace DBus
