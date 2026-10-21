//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file glib2/callbacks.cpp
 *
 * @brief  Implementation of C based callback functions used
 *         by the glib2 gdbus APIs
 */

#include "../features/fmt-compat.hpp"
#include <fmt/format.h>
#include <glib.h>
#include <gio/gio.h>

#include "../async-process.hpp"
#include "../features/debug-log.hpp"
#include "../glib2/utils.hpp"
#include "../object/base.hpp"
#include "../object/callbacklink.hpp"
#include "../object/manager.hpp"
#include "../signals/event.hpp"
#include "../signals/exceptions.hpp"
#include "../signals/single-subscription.hpp"
#include "../service.hpp"


using namespace DBus;

namespace glib2 {
namespace Callbacks {


void _int_callback_name_acquired(GDBusConnection *conn,
                                 const char *name,
                                 void *this_ptr)
{
    DBus::Service *service = static_cast<DBus::Service *>(this_ptr);
    service->BusNameAcquired(std::string(name));
    GDBUSPP_LOG("Service registered: {}", name);
    service->RunIdleDetector(true);
}


void _int_callback_name_lost(GDBusConnection *conn, const gchar *name, void *this_ptr)
{
    DBus::Service *service = static_cast<DBus::Service *>(this_ptr);
    service->BusNameLost(name);
    service->RunIdleDetector(false);
}


void _int_pool_processpool_cb(void *req_ptr, void *pool_data)
{
    auto req = AsyncProcess::Request::UPtr(static_cast<AsyncProcess::Request *>(req_ptr));
    if (req)
    {
        try
        {
            if (Object::Operation::NONE == req->request_type)
            {
                throw DBus::Exception("AsyncProcess::Request",
                                      "Invalid Request Type");
            }

            if (Object::Operation::PROPERTY_GET == req->request_type)
            {
                throw DBus::Exception("AsyncProcess::Request",
                                      "Not yet implemented");
            }

            if (Object::Operation::PROPERTY_SET == req->request_type)
            {
                throw DBus::Exception("AsyncProcess::Request",
                                      "Not yet implemented");
            }

            //  Authorize this request before starting to process the request
            auto azreq = Authz::Request::Create(req);
            bool authzres = req->object->Authorize(azreq);
            GDBUSPP_LOG("ProcessPool (Authorizaion): {} Result: {}",
                        req,
                        (authzres ? "Allow" : "Deny"));
            if (!authzres)
            {
                // Authz failed; stop the request and return an error
                std::string msg = req->object->AuthorizationRejected(azreq);
                throw Authz::Exception(azreq, msg);
            }

            // Authz granted; call the method
            // The Object::MethodCall() finds the proper callback method
            // and calls the appropriate Callback::Execute() method which
            // will handle setting the D-Bus response.
            if (Object::Operation::METHOD_CALL == req->request_type)
            {
                GDBUSPP_LOG("ProcessPool - Request: {}", req);
                req->object->MethodCall(req);
            }
        }
        catch (const DBus::Object::Method::Exception &excp)
        {
            // If a method callback throws Method::Exception, it's an error
            // to be returned to the caller - not an error in the application
            // itself - so don't trigger additional error logging.
            GError *dbuserr = g_dbus_error_new_for_dbus_error(excp.DBusErrorDomain(),
                                                              excp.GetRawError());
            dbuserr->domain = g_quark_from_string(excp.DBusErrorDomain());
            g_dbus_method_invocation_return_gerror(req->invocation, dbuserr);
            g_error_free(dbuserr);
        }
        catch (const Authz::Exception &excp)
        {
            GError *dbuserr = g_dbus_error_new_for_dbus_error(excp.DBusErrorDomain(),
                                                              excp.GetRawError());
            dbuserr->domain = g_quark_from_string(excp.DBusErrorDomain());
            g_dbus_method_invocation_return_gerror(req->invocation, dbuserr);
            g_error_free(dbuserr);
        }
        catch (const DBus::Exception &excp)
        {
            // TODO:  Extend this to include ReqType::GETPROP once that
            //        path is used here.  ReqType::SETPROP will probably
            //        not benefit much from going the async-route.
            GDBUSPP_LOG("ProcessPool - Process Pool Method Call FAILED: {}", excp.what());
            if (Object::Operation::METHOD_CALL == req->request_type)
            {
                GError *dbuserr = g_dbus_error_new_for_dbus_error(req->error_domain.c_str(),
                                                                  excp.GetRawError());
                dbuserr->domain = g_quark_from_string(req->error_domain.c_str());
                g_dbus_method_invocation_return_gerror(req->invocation, dbuserr);
                g_error_free(dbuserr);

                fmt::print(stderr,
                           FMT_COMPILE("** ERROR ** Async call failed: {}\n"
                                       "            error domain: {}\n"),
                           excp.what(),
                           excp.DBusErrorDomain());
            }
            else
            {
                // This should only happen with ReqType::SETPROP and
                // ReqType::NONE; both very unlikely to end up here
                fmt::print(stderr,
                           FMT_COMPILE("** ERROR **  Async call failed: {}\n"),
                           excp.what());
            }
        }
        req.reset();
    }
}


void _int_dbusobject_callback_method_call(GDBusConnection *conn,
                                          const gchar *sender,
                                          const gchar *obj_path,
                                          const gchar *intf_name,
                                          const gchar *meth_name,
                                          GVariant *params,
                                          GDBusMethodInvocation *invoc,
                                          void *this_ptr)
{
    auto cbl = static_cast<Object::CallbackLink *>(this_ptr);
    if (!cbl)
    {
        fmt::print(stderr,
                   FMT_COMPILE("{} MethodCall(sender={}, path={}, interface{}, method={}) callback link is invalid\n"),
                   __func__,
                   sender,
                   obj_path,
                   intf_name,
                   meth_name);
        g_dbus_method_invocation_return_error(invoc,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_INVALID_ARGS,
                                              "MethodCall failed; invalid callback link");
        return;
    }

    try
    {
        AsyncProcess::Request::UPtr req = cbl->NewObjectOperation(conn, sender, obj_path, intf_name);
        GDBUSPP_LOG("Method Callback (Queuing): {}", req);
        req->MethodCall(meth_name, params, invoc);
        cbl->QueueOperation(req);

        // Update the activity for the idle detector
        Object::Manager::Ptr om = cbl->manager.lock();
        if (om)
        {
            om->IdleActivityUpdate();
        }
    }
    catch (const DBus::Exception &excp)
    {
        GDBUSPP_LOG("Method Callback (Queuing FAILED): {}", excp.what());
        excp.SetDBusError(invoc);
    }
}


GVariant *_int_dbusobject_callback_get_property(GDBusConnection *conn,
                                                const gchar *sender,
                                                const gchar *obj_path,
                                                const gchar *intf_name,
                                                const gchar *property_name,
                                                GError **error,
                                                void *this_ptr)
{
    // TODO:  Go a better async route via AsyncProcess::Request::MethodCall()
    //        The glib2 gdbus callback interface expects this method to return instantly
    //
    try
    {
        auto cbl = static_cast<Object::CallbackLink *>(this_ptr);
        if (!cbl)
        {
            std::string errm = fmt::format(
                FMT_COMPILE("{} GetProperty(sender={}, path={}, interface={}, property={}) callback link is invalid"),
                __func__,
                sender,
                obj_path,
                intf_name,
                property_name);
            fmt::print(stderr, FMT_COMPILE("{}\n"), errm);
            throw Object::Exception(errm);
        }

        // Update the activity for the idle detector
        Object::Manager::Ptr om = cbl->manager.lock();
        if (om)
        {
            om->IdleActivityUpdate();
        }

        // Authorize this request before we do anything
        auto req = AsyncProcess::Request::Create(conn, cbl->object, sender, obj_path, intf_name);
        req->GetProperty(property_name);
        auto azreq = Authz::Request::Create(req);

        bool authzres = cbl->object->Authorize(azreq);
        GDBUSPP_LOG("Get Property Callback (Authorizaion): {} Result: {}",
                    req,
                    (authzres ? "Allow" : "Deny"));
        if (!authzres)
        {
            // Authz failed; stop the request and return an error
            std::string msg = req->object->AuthorizationRejected(azreq);
            throw Authz::Exception(azreq, msg);
        }

        // Check if the requested property is accessible via the PropertyCollection,
        // if not return an error
        if (!cbl->object->PropertyExists(property_name))
        {
            GDBUSPP_LOG("Get Property Callback FAIL: {}", cbl->object);
            throw Object::Property::Exception(cbl->object, "property_name", "Property not found");
        }

        // Retrieve the property value and return it to the caller
        GVariant *value = cbl->object->GetProperty(property_name);
        if (nullptr == value)
        {
            GDBUSPP_LOG("GetProperty('{}') returned nullptr: {}", property_name, cbl->object);
            throw Object::Property::Exception(cbl->object,
                                              property_name,
                                              "NULL/nullptr value is not allowed");
        }
        GDBUSPP_LOG("Get Property Callback (Return): {} - Value: {}",
                    req,
                    glib2::Utils::DumpToString(value));
        return value;
    }
    catch (const Object::Property::Exception &excp)
    {
        GDBUSPP_LOG("Get Property Callback FAIL -- Property: '{}' -- ERROR: {}", property_name, excp.what());
        excp.SetDBusErrorProperty(error);
        return nullptr;
    }
    catch (const Authz::Exception &excp)
    {
        GDBUSPP_LOG("Get Property Authorization FAIL -- Property: {} -- ERROR: {}", property_name, excp.what());
        excp.SetDBusError(error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED);
        return nullptr;
    }
    catch (const DBus::Exception &excp)
    {
        GDBUSPP_LOG("Get Property Callback FAIL (DBus::Exception) -- Property: '{}' -- ERROR: {}", property_name, excp.what());
        excp.SetDBusError(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED);
        return nullptr;
    }
}


gboolean _int_dbusobject_callback_set_property(GDBusConnection *conn,
                                               const gchar *sender,
                                               const gchar *obj_path,
                                               const gchar *intf_name,
                                               const gchar *property_name,
                                               GVariant *value,
                                               GError **error,
                                               void *this_ptr)
{
    try
    {
        auto cbl = static_cast<Object::CallbackLink *>(this_ptr);
        if (!cbl)
        {
            std::string errm = fmt::format(
                FMT_COMPILE("{} SetProperty(sender={}, path={}, interface={}, property{}) callback link is invalid"),
                __func__,
                sender,
                obj_path,
                intf_name,
                property_name);

            fmt::print(stderr, FMT_COMPILE("{}\n"), errm);
            throw Object::Exception(errm);
        }

        // Update the activity for the idle detector
        Object::Manager::Ptr om = cbl->manager.lock();
        if (om)
        {
            om->IdleActivityUpdate();
        }

        // Authorize this request before we do anything
        auto req = AsyncProcess::Request::Create(conn, cbl->object, sender, obj_path, intf_name);
        req->SetProperty(property_name, value);
        auto azreq = Authz::Request::Create(req);
        bool authzres = cbl->object->Authorize(azreq);
        GDBUSPP_LOG("Set Property Callback (Authorizaion): {} Result: {}", req, (authzres ? "Allow" : "Deny"));
        if (!authzres)
        {
            // Authz failed; stop the request and return an error
            std::string msg = req->object->AuthorizationRejected(azreq);
            throw Authz::Exception(azreq, msg);
        }

        // If the requested property is accessible via the PropertyCollection,
        // handle that internally

        Object::Property::Update::Ptr updated_vals = nullptr;
        if (cbl->object->PropertyExists(property_name))
        {
            updated_vals = cbl->object->SetProperty(property_name, value);
            GDBUSPP_LOG("Set Property Callback (Return): {} - New value: {}",
                        req,
                        glib2::Utils::DumpToString(value));
        }
        else
        {
            GDBUSPP_LOG("Set Property Callback FAIL: {}", cbl->object);
            throw Object::Property::Exception(cbl->object,
                                              property_name,
                                              "Property not found");
        }

        // If updated_vals != NULL, we have a valid response which contains
        // information about what has changed.  This is further
        // used to issue a standard D-Bus signal that an object property
        // have been modified; which is the signal being emitted below.
        if (updated_vals)
        {
            GError *local_err = nullptr;
            g_dbus_connection_emit_signal(conn,
                                          nullptr,
                                          obj_path,
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          updated_vals->Finalize(),
                                          &local_err);
            if (local_err)
            {
                GDBUSPP_LOG("Set Property Callback FAIL: {}", cbl->object);
                throw Object::Property::Exception(cbl->object,
                                                  property_name,
                                                  "Failed signalign new property value",
                                                  local_err);
            }
            return true;
        }

        throw Object::Property::Exception(cbl->object,
                                          property_name,
                                          "Failed signaling new property value");
    }
    catch (const Object::Exception &err)
    {
        GDBUSPP_LOG("Set Property Callback FAIL -- Property: {} -- ERROR: {}",
                    property_name,
                    err.what());
        err.SetDBusError(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED);
        return false;
    }
    catch (const Authz::Exception &err)
    {
        GDBUSPP_LOG("Set Property Authorization FAIL -- Property: {} -- ERROR: ",
                    property_name,
                    err.what());
        err.SetDBusError(error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED);
        return false;
    }
    catch (const DBus::Exception &err)
    {
        GDBUSPP_LOG("Set Property Callback FAIL (DBus::Exception) -- Property: {} -- ERROR: {}",
                    property_name,
                    err.what());
        err.SetDBusError(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED);
        return false;
    }
}


void _int_dbusobject_callback_destruct(void *this_ptr)
{
    auto cbl = static_cast<Object::CallbackLink *>(this_ptr);
    if (!cbl)
    {
        fmt::print(stderr,
                   FMT_COMPILE("{} Object destruction - callback link is invalid\n"),
                   __func__);
        return;
    }

    Object::Manager::Ptr om = cbl->manager.lock();
    if (om)
    {
        GDBUSPP_LOG("D-Bus Object Desctruct: {}", cbl->object);
        om->IdleActivityUpdate();
        om->_destructObjectCallback(cbl->object->GetPath());
        // NOTE: cbl is no longer valid after this call
    }
    else
    {
        fmt::print(stderr,
                   FMT_COMPILE("** ERROR **  {}: Could not get access to Object::Manager"),
                   __func__);
    }
}



void _int_dbus_connection_signal_handler(GDBusConnection *conn,
                                         const gchar *sender,
                                         const gchar *obj_path,
                                         const gchar *intf_name,
                                         const gchar *sign_name,
                                         GVariant *params,
                                         gpointer this_ptr)
{
    auto sigsub = static_cast<Signals::SingleSubscription *>(this_ptr);
    if (!sigsub)
    {
        std::string errm = fmt::format(
            FMT_COMPILE("{} SingleSubscription(sender={}, path={}, interface={}, singal_name={}) "
                        "object pointer is invalid"),
            __func__,
            sender,
            obj_path,
            intf_name,
            sign_name);
        fmt::print(stderr, FMT_COMPILE("{}\n"), errm);

        throw Signals::Exception(errm);
    }
    // TODO:  Consider idle detection timestamp update on signal handling
    //        This will require access to the object manager, which need to
    //        come via "this_ptr".  And it is probably more suitable for
    //        DBus::Signals::Group based signal handling

    // If the subscription is from a specific target, double check the
    // sender of the signal
    const char *busname = sigsub->target->GetBusName();
    if (busname && strcmp(sender, busname) != 0)
    {
        GDBUSPP_LOG("SIGNAL MISMATCH: {}\n\tsender={}", sigsub->target, sender);
        // This subscription does not expect this sender
        return;
    }

    auto event = Signals::Event::Create(sender, obj_path, intf_name, sign_name, params);
    GDBUSPP_LOG("Signal Callback: {}", event);
    sigsub->callback(event);
}



int _int_mainloop_stop_handler(void *loop) noexcept
{
    g_main_loop_quit((GMainLoop *)loop);
    g_thread_pool_stop_unused_threads();
    return G_SOURCE_CONTINUE;
}

} // namespace Callbacks
} // namespace glib2
