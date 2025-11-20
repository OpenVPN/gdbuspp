//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file glib2/callbacks.hpp
 *
 * @brief  Declaration of C based callback functions used
 *         by the glib2 gdbus APIs.
 */

#pragma once

#include <glib.h>
#include <gio/gio.h>


namespace glib2 {
namespace Callbacks {

/**
 *  C wrapper function for the g_dbus_own_name_on_connect() callback
 *  function.  This is called once the D-Bus library managed
 *  to retrieve the requested bus name.
 *
 * @param conn      D-Bus connection pointer calling this function
 * @param name      char * containing the bus name aqcuired
 * @param this_ptr  void * to the current object.  This *must* point
 *                  at a valid DBus::Service::Ptr object which handles this
 *                  bus name.
 */
void _int_callback_name_acquired(GDBusConnection *conn,
                                 const char *name,
                                 void *this_ptr);


/**
 *  C wrapper function for the GDBus g_bus_own_name_on_connection()
 *  callback function.  This is called if the well-known D-Bus name
 *  is lost or revoked by the D-Bus daemon/broker.
 *
 * @param conn      D-Bus connection pointer calling this function
 * @param name      char * containing the bus name lost
 * @param this_ptr  void * to the current object.  This *must* point
 *                  at a valid DBus::Service::Ptr object which handles this
 *                  bus name.
 */
void _int_callback_name_lost(GDBusConnection *conn, const gchar *name, void *this_ptr);


/**
 *  C wrapper function used by the AsyncProcess::Pool implementation, which
 *  wraps the glib2 process threading feature into a C++ wrapping.
 *
 *  This is called from within glib2 and prepared via the
 *  AsyncProcess::Pool::PushCallback() method wrapping the
 *  glib2 g_thread_pool_push() function.
 *
 * @param data       Raw pointer to the AsyncProcess::Request object which
 *                   contains the data requested to be processed.  This
 *                   contains information about the D-Bus object which the
 *                   operation will be performed in.
 * @param pool_data  (Not used)
 */
void _int_pool_processpool_cb(void *data, void *pool_data);


/**
 *  C wrapper function called whenever a D-Bus object method is being
 *  called.  This call will contain a pointer to a DBus::Object::CallbackLink
 *  object which has the needed information to which DBus::Object::Base this
 *  call will be performed in.
 *
 *  Once all the information has been gathered and inspected, a
 *  AsyncPool::Request object is created and sent to AsyncProcess::Pool queue
 *  to be processed asynchronously.
 *
 * @param conn      GDBusConnection* where the call occurred
 * @param sender    char * containing a unique bus name of the caller
 * @param obj_path  char * containing the D-Bus object path to operate on
 * @param intf_name char * containing the D-Bus interface inside the object
 * @param meth_name char * containing the D-Bus method name inside the interface
 * @param params    GVariant * containing all the arguments to the method call
 * @param invoc     GDBusMethodINvocation * object  where the method respons
 *                  need to be returned
 * @param this_ptr  Raw pointer pointing at the CallbackLink object which
 *                  contains information about the related DBus::Object::Base
 *                  object this D-Bus method call belongs to
 */
void _int_dbusobject_callback_method_call(GDBusConnection *conn,
                                          const gchar *sender,
                                          const gchar *obj_path,
                                          const gchar *intf_name,
                                          const gchar *meth_name,
                                          GVariant *params,
                                          GDBusMethodInvocation *invoc,
                                          void *this_ptr);


/**
 *  C wrapper function called whenever a D-Bus object property value is being
 *  retrieved.  This call will contain a pointer to a DBus::Object::CallbackLink
 *  object which has the needed information to the DBus::Object::Base containing
 *  the requested property.
 *
 * @param conn           GDBusConnection* where the call occurred
 * @param sender         char * containing a unique bus name of the caller
 * @param obj_path       char * containing the D-Bus object path to operate on
 * @param intf_name      char * containing the D-Bus interface inside the object
 * @param property_name  char * containing the object property name
 * @param error          GError ** return pointer where errors will be provided
 * @param this_ptr       Raw pointer pointing at the CallbackLink object which
 *                       contains information about the related DBus::Object::Base
 *                       object the D-Bus object property belongs to
 *
 * @return GVariant*     Returns a GVariant * object containing the the property
 *                       value for the caller
 */
GVariant *_int_dbusobject_callback_get_property(GDBusConnection *conn,
                                                const gchar *sender,
                                                const gchar *obj_path,
                                                const gchar *intf_name,
                                                const gchar *property_name,
                                                GError **error,
                                                void *this_ptr);


/**
 *  C wrapper function called whenever a D-Bus object property value is being
 *  changed by a D-Bus client proxy.  This call will contain a pointer to a
 *  DBus::Object::CallbackLink object which has the needed information to the
 *  DBus::Object::Base containing the requested property.
 *
 *  This callback handler will also do the D-Bus signal maintenance, sending
 *  the org.freedesktop.DBus.Properties.PropertiesChanged signal with information
 *  about the new value for the property.
 *
 * @param conn           GDBusConnection* where the call occurred
 * @param sender         char * containing a unique bus name of the caller
 * @param obj_path       char * containing the D-Bus object path to operate on
 * @param intf_name      char * containing the D-Bus interface inside the object
 * @param property_name  char * containing the object property name
 * @param value          GVariant * containing the new value for the property
 * @param error          GError ** return pointer where errors will be provided
 * @param this_ptr       Raw pointer pointing at the CallbackLink object which
 *                       contains information about the related DBus::Object::Base
 *                       object the D-Bus object property belongs to
 *
 * @return gboolean Returns true if the change was successful, otherwise false.
 */
gboolean _int_dbusobject_callback_set_property(GDBusConnection *conn,
                                               const gchar *sender,
                                               const gchar *obj_path,
                                               const gchar *intf_name,
                                               const gchar *property_name,
                                               GVariant *value,
                                               GError **error,
                                               void *this_ptr);


/**
 *  C wrapper function called whenever the g_dbus_connection_unregister_object()
 *  function is called.  The purpose of this callback is to clean up the memory
 *  resources used by the object hosted in a D-Bus service.
 *
 *  The g_dbus_connection_unregister_object() function typically called via
 *  the DBus::Object::Manager::RemoveObject() method.  And this C wrapper
 *  function will again call the internal
 *  DBus::Object::Manager::_destructObjectCallback() method to release the
 *  C++ objects from memory.
 *
 * @param this_ptr   Raw pointer pointing at the CallbackLink object which
 *                   contains information about the related DBus::Object::Base
 *                   object the D-Bus object property belongs to
 */
void _int_dbusobject_callback_destruct(void *this_ptr);



/**
 *  C wrapper functions called by the glib2 stack when a D-Bus signal event
 *  happens.  This will only be called if a connection has subscribed to
 *  certain D-Bus signals.
 *
 * @param conn           GDBusConnection* where the signal event occurred
 * @param sender         char * containing a unique bus name of the caller
 * @param obj_path       char * containing the D-Bus object path to operate on
 * @param intf_name      char * containing the D-Bus interface inside the object
 * @param sign_name      char * containing the signal name
 * @param params         GVariant object containing the signal data
 * @param this_ptr       Raw pointer to the Signals::SingleSubscription object
 *                       prepared to handle this signal event
 */
void _int_dbus_connection_signal_handler(GDBusConnection *conn,
                                         const gchar *sender,
                                         const gchar *obj_path,
                                         const gchar *intf_name,
                                         const gchar *sign_name,
                                         GVariant *params,
                                         void *this_ptr);



/**
 *  C wrapper function called by signal handler configured via the
 *  g_unix_signal_add() calls in DBus::MainLoop::Run().  The purpose of this
 *  call is to stop the currently running GMainLoop
 *
 * @param loop   GMainLoop * pointer where to operate on
 * @return int
 */
int _int_mainloop_stop_handler(void *loop) noexcept;

} // namespace Callbacks
} // namespace glib2
