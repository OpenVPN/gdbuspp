//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Antonio Quartulli <antonio@openvpn.net>
//  Copyright (C)  Arne Schwabe <arne@openvpn.net>
//  Copyright (C)  Charlie Vigue <charlie.vigue@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file object/base.hpp
 *
 * @brief Declaration of DBus::Object::Base.  This class contains
 *        everything needed to represent a C++ object as a complete object
 *        for a specific path and interface in a D-Bus service
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../authz-request.hpp"
#include "../glib2/utils.hpp"
#include "../signals/group.hpp"
#include "exceptions.hpp"
#include "method.hpp"
#include "property.hpp"


namespace DBus {
namespace Object {

class Base : public std::enable_shared_from_this<Base>
{
  public:
    using Ptr = std::shared_ptr<Base>;

    /**
     *  Creates a new object based on the Object::Base class
     *
     * @tparam C    Class name of the object to create.
     *              This must inherit from the Object::Base
     * @tparam T    Template argument for the arguments passed to the
     *              class constructor
     * @param args  All the argument list used with the constructor
     *
     * @return Base::Ptr  Returns a std::shared_ptr to the newly created object
     */
    template <typename C, typename... T>
    static Base::Ptr Create(T &&...args)
    {
        return Ptr(new C(std::forward<T>(args)...));
    }

    virtual ~Base() noexcept = default;


    /**
     * Get the D-Bus object path of this object
     *
     * @return const std::string&
     */
    const std::string &GetPath() const noexcept;

    /**
     * Get the D-Bus interface name used by this object
     *
     * @return const std::string&
     */
    const std::string &GetInterface() const noexcept;


    /**
     *  Add a D-Bus property to this object, with a link to a C++ variable.
     *  When this property is read or updated by a D-Bus caller, the
     *  bounded C++ variable will be accessed.
     *
     *  If the property is set up allowing read-write operations, the C++
     *  variable is also updated via D-Bus calls to the
     *  org.freedesktop.DBus.Properties.Set method.
     *
     * @tparam T         Type of the C++ variable storing the property value
     * @param propname   std::string with the D-Bus property name
     * @param variable   Reference to the C++ variable this property
     *                   bound to.
     * @param readwrite  If false, this is exposed as a read-only property,
     *                   otherwise can be updated by a D-Bus caller
     * @param override_dbus_type  (Optional).  The C++ data type is normally
     *                   deducted automatically and converted to a related
     *                   D-Bus data type.  In cases where this deduction is
     *                   not correct, this can be overridden by setting this
     *                   argument to the expected D-Bus data type.
     */
    template <typename T>
    void AddProperty(const std::string &propname,
                     T &variable,
                     bool readwrite,
                     const std::string &override_dbus_type = "")
    {
        Property::Interface::Ptr prop;
        prop = std::make_shared<PropertyType<T>>(interface,
                                                 propname,
                                                 readwrite,
                                                 variable,
                                                 override_dbus_type);
        properties->AddBinding(prop);
    }


    /**
     *  Add a D-Bus property to this object, with a link to a C++ std::vector
     *  variable.  When this property is read or updated by a D-Bus caller,
     *  the bounded C++ std::vector will be accessed.
     *
     *  If the property is set up allowing read-write operations, the C++
     *  std::vector variable is also updated via D-Bus calls to the
     *  org.freedesktop.DBus.Properties.Set method.
     *
     * @tparam T         Type of the C++ variable inside the std::vector
     *                   storing the values
     * @param propname   std::string with the D-Bus property name
     * @param variable   Reference to the C++ std::vector<> based variable
     *                   this property bound to.
     * @param readwrite  If false, this is exposed as a read-only property,
     *                   otherwise can be updated by a D-Bus caller
     * @param override_dbus_type  (Optional).  The C++ data type is normally
     *                   deducted automatically and converted to a related
     *                   D-Bus data type.  In cases where this deduction is
     *                   not correct, this can be overridden by setting this
     *                   argument to the expected D-Bus data type.  Only
     *                   the D-Bus data type of the main variable type must
     *                   be provided, not the complete array declaration.
     */
    template <typename T>
    void AddProperty(const std::string &propname,
                     std::vector<T> &variable,
                     bool readwrite,
                     const std::string &override_dbus_type = "")
    {
        Property::Interface::Ptr prop;
        prop = std::make_shared<PropertyType<std::vector<T>>>(interface,
                                                              propname,
                                                              readwrite,
                                                              variable);
        properties->AddBinding(prop);
    }


    /**
     *  This is similar method to @AddProperty() but instead of linking
     *  the D-Bus property to a C++ variable, C++ functors are used as
     *  callback methods for retrieving and setting the property value.
     *
     *  This is used when needing to operate on more complex D-Bus data
     *  types, like properties containing struct values of different types.
     *
     * @param name        std::string with the D-Bus property name
     * @param dbustype    std::string containing the D-Bus data type of this
     *                    property
     * @param get_cb      GetPropertyCallback() functor. Called each time
     *                    this property is queried for its value via the
     *                    D-Bus org.freedesktop.DBus.Properties.Get() method.
     * @param set_cb      SetPropertyCallback() functor. Called when this
     *                    property value is being changed via the D-Bus
     *                    org.freedesktop.DBus.Properties.Set() method.
     */
    void AddPropertyBySpec(const std::string name,
                           const std::string dbustype,
                           Property::BySpec::GetPropertyCallback get_cb,
                           Property::BySpec::SetPropertyCallback set_cb);

    /**
     *  This is the AddPropertyBySpec variant for read-only properties.
     *  The read-write flag is automatically false, as this method call
     *  does not set a "set property" callback; only a "get property".
     *
     *  This is used when needing to operate on more complex D-Bus data
     *  types, like properties containing struct values of different types.
     *
     * @param name        std::string with the D-Bus property name
     * @param dbustype    std::string containing the D-Bus data type of this
     *                    property
     * @param get_cb      GetPropertyCallback() functor. Called each time
     *                    this property is queried for its value via the
     *                    D-Bus org.freedesktop.DBus.Properties.Get() method.
     */
    void AddPropertyBySpec(const std::string name,
                           const std::string dbustype,
                           Property::BySpec::GetPropertyCallback get_cb);


    /**
     *  Checks if a specific D-Bus property is declared within this object
     *
     * @param propname  std::string with the property name to check
     * @returns True if the given property name is found, otherwise false.
     */
    bool PropertyExists(const std::string &propname) const;

    /**
     *  Retrieves the current value of the property in a GVariant
     *  object.  This object can be used as the return value in the
     *  glib2 get property callback method.
     *
     * @param propname    std::string containing the property name to lookup
     * @return GVariant*  Raw pointer to a GVariant object containing
     *                    the property value retrieved from assigned
     *                    the C++ variable.
     */

    GVariant *GetProperty(const std::string &propname) const;

    /**
     *  Extracts a new value from a GVariant object for a named property
     *  and changes the property value in the C++ variable bound to the
     *  property.
     *
     * @param property_name        Property name to update
     * @param value                GVariant pointer containing the new value
     * @return PropertyUpdate::Ptr PropertyUpdate object with all the updated properties
     *                             and their new values
     */
    Property::Update::Ptr SetProperty(const std::string &propname, GVariant *value);



    /**
     *  Add a new D-Bus method to this object.  The provided callback
     *  functor/lambda will be called each time a D-Bus proxy calls this
     *  method via the D-Bus.
     *
     *  This functor/callback will receive Method::Arguments::Ptr
     *  object with all the details related to the D-Bus method call.
     *
     *  To add input/output arguments for this D-Bus method, the
     *  Method::Arguments::Ptr object returned by this method call, the
     *  Method::Arguments::AddInput() and Method::Arguments::AddOutput()
     *  methods can be used.
     *
     * @param method_name      std::string with the name of the D-Bus method
     * @param method_callback  Method::CallbackFnc callback function/lambda to
     *                         execute on a D-Bus request
     *
     * @return Method::Arguments::Ptr which can be used to add input and
     *         output arguments this D-Bus method needs and will provide.
     */
    Method::Arguments::Ptr AddMethod(const std::string &method_name,
                                     Method::CallbackFnc method_callback);


    /**
     *  Register all signals prepared in a Signals::Group based object
     *
     *  Signals registered in this object will be added to the
     *  object introspection of this object.
     *
     * @param signal_group  Signal::Group::Ptr to the signal group
     */
    void RegisterSignals(const Signals::Group::Ptr signal_group);


    /**
     *  This provides the complete introspection XML document needed
     *  by the glib2 API to register a new D-Bus object on the bus.
     *
     * @return const std::string containing the XML document describing this
     *               object
     */
    const std::string GenerateIntrospection() const;


    /**
     *  Before any of the method or get/set property callbacks happens,
     *  the Authorize() is called to be authorized for the requested
     *  operation.
     *
     *  If the implementation does not require any type of access control,
     *  this method can return true.
     *
     * @param AuthReq::Ptr  The Authz request object with information
     *                      about caller and object target
     *
     * @returns If the callback returns true, the requested operation is
     *          allowed, otherwise the operation will be rejected.
     */
    virtual const bool Authorize(const Authz::Request::Ptr request) = 0;


    /**
     *  Check if a specific D-Bus method is configured in this D-Bus object
     *
     *  @param meth_name   std::string with the method name to check for
     *
     *  @return bool True if the method exists, otherwise false
     */
    const bool MethodExists(const std::string &meth_name) const;


    /**
     *  Run a specific callback in the object.  This method is expected
     *  to be called from the glib2::Callbacks::_int_pool_processpool_cb()
     *  function.
     *
     *  The glib2 callback integration will first do an Authorize() call
     *  before passing the AsyncProcess::Request object to this method.
     *  The AsyncProcess::Request object contains all the needed information
     *  to perform the D-Bus method call and provide a response back to the
     *  D-Bus proxy caller.
     *
     *  If the method is expected to return a response back to the proxy
     *  caller, this is handled by the Method::Callback::Execute() method,
     *  which this method eventually will call.
     *
     * @param req         AsyncProcess::Request::UPtr with the method call
     *                    details
     */
    void MethodCall(AsyncProcess::Request::UPtr &req);

    /**
     *  Retrieve the object setting if the idle detector should consider
     *  this object when evaluating if a service is running idle or not.
     *
     * @return Returns true when the idle detector has been disabled and the
     *         service will be shut down regardless of this object is in use
     *         or not if no activity has been registered.
     *         If this returns false, the idle detector will not shut down
     *         the service as long as this object is available in memory.
     */
    const bool GetIdleDetectorDisabled() const;


    friend std::ostream &operator<<(std::ostream &os, const Base &object)
    {
        std::ostringstream r;
        r << "Object(path=" << object.object_path
          << ", interface=" << object.interface << ")";
        return os << r.str();
    }

    friend std::ostream &operator<<(std::ostream &os, const Base *object)
    {
        std::ostringstream r;
        r << "Object(path=" << object->object_path
          << ", interface=" << object->interface << ")";
        return os << r.str();
    }

  protected:
    /**
     *  Create a new DBus::Object which is being represented as an object
     *  via a DBus::Service provided via the D-Bus daemon
     *
     *  This implementation does not support multiple interfaces on the
     *  same object path.
     *
     *  This constructor is not intended to be used directly, but
     *  via the Object::Base::Create() static method
     *
     * @param path    std::string with the D-Bus path to this object
     * @param interf  std::string with the D-Bus interface used by this object
     */
    Base(const std::string &path, const std::string &interf);

    /**
     *  If the DBus::Service implementation has enabled the idle detection,
     *  it may shutdown if there has not been any activity within a certain
     *  amount of time.
     *
     *  If a D-Bus object is added to the service at runtime, that will
     *  result in the service disabling the idle detection while that object
     *  is active.  Once the D-Bus object is removed again, the idle detection
     *  will be activated again.
     *
     *  Via this method, it can instruct the idle detection logic to not
     *  consider this object at all.  If an object with the "skip idle
     *  detection" flag enabled, the service may shutdown even if this object
     *  is still in memory.
     *
     *  This flag is false by default
     *
     * @param enable  bool flag to indicate if this object should be considered
     *                if the D-Bus service is being idle and wants to shut down.
     */
    void DisableIdleDetector(const bool enable);


  private:
    //
    //  Internal property related classes
    //
    //  DBus::Object::PropertyTypeBase and DBus::Object::PropertyType
    // FIXME: Reconsider location of these sub-classes

    /**
     *  Type generic internal interface for managing D-Bus properties.
     *  This keeps track of the property name and value, as well as
     *  read/write access for the property.
     *
     *  This class defines the main Base API used for the property access,
     *  generalised for the C++ data type via the template.  This class is
     *  used by PropertyType which is the main implementation of the
     *  property API.
     *
     *  @tparam T  C++ variable data type this property object is bound to
     */
    template <typename T>
    class PropertyTypeBase : public Property::Interface
    {
      public:
        /**
         *  See @PropertyType for details; arguments are passed from
         *  that class to this contructor.
         *
         * @tparam T          C++ variable data type of the value variable
         * @param name_arg    D-Bus object property name
         * @param readwr_arg  boolean read/write flag. Read-only if false
         * @param variable    Reference to the C++ variable holding the
         *                    property value
         */
        PropertyTypeBase(const std::string &interface_arg,
                         const std::string &name_arg,
                         const bool readwr_arg,
                         T &variable)
            : interface(interface_arg),
              name(name_arg),
              readwrite(readwr_arg),
              variable_ref(variable)
        {
        }


        /**
         *  Generates the <property/> XML node for this particular
         *  property.
         *
         * @return const std::string containing the XML fragment to be
         *         included in a larger introspection document.
         */
        const std::string GenerateIntrospection() const override
        {

            return "<property type='" + std::string(GetDBusType()) + "' "
                   + " name='" + name + "' access='"
                   + (readwrite ? "readwrite" : "read") + "' />";
        }


        /**
         *  Retrieve the D-Bus property name of this property object.
         *
         * @return const std::string&
         */
        const std::string &GetName() const noexcept override
        {
            return name;
        }


        /**
         *  Retrieve the D-Bus interface this property is bound to
         *
         * @return const std::string&
         */
        const std::string &GetInterface() const noexcept override
        {
            return interface;
        }


      protected:
        const std::string interface; //<< D-Bus interface this property belongs to
        const std::string name;      //<< D-Bus property name
        const bool readwrite;        //<< Read/write or read-only property
        T &variable_ref;             //<< Reference to the variable holding the value
    };


    /**
     *  Generic implementation for linking a D-Bus object property to a
     *  C++ object's variable member.  The callback methods for reading and
     *  writing property data can be used directly with this interface.
     *
     *  This is the implementation of a PropertyTypeBase class into
     *  the PropertyType class which is used by Object::Base
     *
     * @tparam T          C++ variable data type of the value variable
     */
    template <typename T>
    class PropertyType : public PropertyTypeBase<T>
    {
      public:
        /**
         *  Constructs a PropertyType object which links a D-Bus
         *  object's property and value to a C++ object's variable.
         *
         * @tparam T          C++ variable data type of the value variable
         * @param name_arg            D-Bus property name this is available
         * @param readwr_arg          Read/write flag, if true this can be
         *                            updated via the D-Bus
         *                            org.freedesktop.DBus.Properties.Set()
         *                            method.  Otherwise, this is a
         *                            read-only property
         * @param variable            Reference to the C++ variable to link
         *                            this D-Bus property with.
         * @param override_dbus_type  (Optional) If the C++ deducted data
         *                            type does not match well enough, the
         *                            needed D-Bus data type can be overridden.
         */
        PropertyType(const std::string &interface_arg,
                     const std::string &name_arg,
                     const bool readwr_arg,
                     T &variable,
                     const std::string &override_dbus_type = "")
            : PropertyTypeBase<T>(interface_arg, name_arg, readwr_arg, variable),
              override_dbus_type(override_dbus_type)
        {
        }

        virtual ~PropertyType() noexcept = default;


        /**
         *  Retrieve the D-Bus data type this property is declared with
         *
         * @return const char* containing the D-Bus data type as a string.
         */
        const char *GetDBusType() const noexcept override
        {
            return (override_dbus_type.empty()
                        ? glib2::DataType::DBus<T>()
                        : override_dbus_type.c_str());
        }


        /**
         *  Retrieves the current value of the property in a GVariant
         *  object.  This object can be used as the return value in the
         *  glib2 get property callback method.
         *
         * @return GVariant*  Raw pointer to a GVariant object containing
         *                    the property value retrieved from assigned
         *                    the C++ variable.
         */
        GVariant *GetValue() const noexcept override
        {
            return glib2::Value::CreateType(GetDBusType(), PropertyTypeBase<T>::variable_ref);
        }

        /**
         *  Extracts the value from a GVariant object and updates the C++
         *  variable with the new value.  This method is to be used from the
         *  glib2's set property callback.
         *
         *  This will also prepare a PropertyUpdate object containing the
         *  information needed in the
         *  org.freedesktop.DBus.Properties.PropertiesChanged signal which
         *  must be sent when properties changes.
         *
         * @param value_arg          Raw pointer to the GVariant object
         *                           containing the new value.
         * @return PropertyUpdate::Ptr object containing the information needed
         *         when sending the PropertiesChanged signal.
         */
        Property::Update::Ptr SetValue(GVariant *value_arg) override
        {
            PropertyTypeBase<T>::variable_ref = glib2::Value::Get<T>(value_arg);
            Property::Update::Ptr resp = Property::Interface::PrepareUpdate();
            resp->AddValue(PropertyTypeBase<T>::variable_ref);
            return resp;
        }

      private:
        const std::string override_dbus_type;
    };


    /**
     *  This does the same job as the Object::PropertyType class.  The
     *  main difference is this variant can handle std::vector<> based
     *  types.
     *
     *  @tparam T  C++ data type of the type stored inside the std::vector<>
     */
    template <typename T>
    class PropertyType<std::vector<T>> : public PropertyTypeBase<std::vector<T>>
    {
      public:
        /**
         *  A PropertyType based implementation targeting simple data types via
         *  an array (std::vector<> based).
         *
         * @tparam T  C++ data type of the type stored inside the std::vector<>
         * @param name_arg        Name of the property itself
         * @param readwrite       Boolean enabling read/write capabilities
         * @param vector_var      Reference to a std::vector<> container
         *                        variable holding the data.
         */
        PropertyType<std::vector<T>>(const std::string &interface_arg,
                                     const std::string &name_arg,
                                     const bool readwrite,
                                     std::vector<T> &vector_var)
            : PropertyTypeBase<std::vector<T>>(interface_arg, name_arg, readwrite, vector_var),
              dbus_array_type("a" + std::string(glib2::DataType::DBus<T>()))
        {
        }

        virtual ~PropertyType() noexcept = default;

        /**
         *  Retrieve the D-Bus data type of this property
         *
         * @return  Returns a C char based string containing the GVariant
         *          compatible data type in use.
         */
        const char *GetDBusType() const noexcept override
        {
            return dbus_array_type.c_str();
        }


        /**
         *  Returns a GVariant object with this object's vector contents
         *
         * @return GVariant object populated with this objects vector contents
         */
        GVariant *GetValue() const noexcept override
        {
            return glib2::Value::CreateVector(this->variable_ref);
        }


        /**
         *  Parses a GVariant array and populates this object's std::vector with
         *  the provided data
         *
         * @tparam T  C++ data type of the type stored inside the std::vector<>
         * @param value   GVariant object containing the array to decode into
         *                a C++ based std::vector/array.
         *
         * @return  Returns Property::Update object containing the information
         *          needed by the glib2::Callbacks::_int_dbusobject_callback_set_property
         *          method to process this property update properly.
         */
        Property::Update::Ptr SetValue(GVariant *value) override
        {
            // The following ExtractVector() call will unref value. This
            // seems to also be unref'd inside glib2's library; so we add
            // an extra reference count to not completely free it in our
            // own internal processing.
            g_variant_ref_sink(value);
            std::vector<T> newvalue = glib2::Value::ExtractVector<T>(value, glib2::DataType::DBus<T>(), false);
            this->variable_ref = newvalue;

            auto upd = Property::Interface::PrepareUpdate();
            upd->AddValue(newvalue);
            return upd;
        }


      private:
        const std::string dbus_array_type; ///< D-Bus data type of the array
    };
    // End of DBus::Object::PropertyType/PropertyTypeBase related classes

    //
    // DBus::Object::Base private variables
    //

    /// D-Bus object path this object is accessible from
    const std::string object_path;

    /// D-Bus object interface scope of this C++ is responsible for
    const std::string interface;

    /// Disable the idle detection checks, see DisableIdleDetector() for details
    bool disable_idle_detection = false;

    /**
     *  D-Bus properties stored within this object.
     *  This is populated via the Object::Base::AddProperty(),
     *  Object::Base::AddPropertyBySpec() methods.
     */
    Property::Collection::Ptr properties{};

    /**
     *  D-Bus methods provided from this object
     *  This is populated via the Object::Base::AddMethod() method.
     */
    Method::Collection::Ptr methods{};

    /**
     *  D-Bus signals this object may emit.  This is setup via
     *  the Object::Base::RegisterSignals() method.
     */
    Signals::Group::Ptr signals{};
};


} // namespace Object
} // namespace DBus
