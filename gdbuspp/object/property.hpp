//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Antonio Quartulli <antonio@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//  Copyright (C)  Arne Schwabe <arne@openvpn.net>
//

/**
 * @file object/property.hpp
 *
 * @brief Declare the infrastructure needed to manage D-Bus object
 *        exposed properties and keep them under control of a
 *        DBus::Object::Base object
 */

#pragma once

#include <functional>
#include <map>
#include <memory>

#include "../glib2/utils.hpp"
#include "exceptions.hpp"


namespace DBus {
namespace Object {

class Base; // Forward declaration - found in object/base.hpp


namespace Property {

class Interface; // Forward declaration, declared later in this file

/**
 *  Property::Update is used when a property value has been changed
 *  through the org.freedesktop.DBus.Properties.Set() D-Bus method call.
 *
 *  This collects information needed to issue the
 *  org.freedesktop.DBus.Properties.PropertiesChanged() D-Bus signal
 *  when the change has been performed.
 *
 *  This object is created and prepared by the Property::SetValue()
 *  method.
 */
class Update
{
  public:
    using Ptr = std::shared_ptr<Update>;


    /**
     *  Create a new property update reponse
     *
     * @param prop          Reference to the property being updated
     * @return Update::Ptr  Returns a std::shared_ptr<> to the new object
     */
    [[nodiscard]] static Update::Ptr Create(const Property::Interface &prop)
    {
        Ptr ret;
        ret.reset(new Update(prop));
        return ret;
    }


    /**
     *  Add a the new value for the property.  For simple (single)
     *  property types, this is only called once.  For complex types
     *  (via PropertyBySpec objects), this can be called more times.
     *
     * @tparam T    C++ data type of the new value
     * @param val   Reference to the new value
     */
    template <typename T>
    void AddValue(const T &val)
    {
        updated_vals.push_back(glib2::Value::Create<T>(val));
    }


    /**
     *  This is similar to the @AddValue() method above, but this
     *  is used when processing properties storing array/vector values.
     *
     * @tparam T     C++ data type of the array/vector (std::vector<T>)
     * @param vals   The complete std::vector<> object with all new values
     */
    template <typename T>
    void AddValue(const std::vector<T> &vals)
    {
        GVariantBuilder *val_array = glib2::Builder::Create("a*");
        for (const auto &v : vals)
        {
            glib2::Builder::Add(val_array, v);
        }
        updated_vals.push_back(glib2::Builder::Finish(val_array));
    }


    /**
     *  This is used by the @_int_dbusobject_callback_set_property()
     *  glib2 callback function.  This is a generic internal callback
     *  which can handle all types of properties via the Property::Update
     *  objects.
     *
     * @return GVariant*   Pointer to a GVariant object containing the data
     *         needed to send the org.freedesktop.Properties.PropertyChanged
     *         signal.
     */
    GVariant *Finalize();



  private:
    Update(const Property::Interface &prop)
        : property(prop)
    {
    }

    const Property::Interface &property;
    std::vector<GVariant *> updated_vals = {};
};



/**
 *  The Property::Interface class is intended to be inherited by an
 *  implementation for D-Bus object properties.  It enforces the
 *  implementation to provide the needed functions expected by the
 *  Object::Base::PropertyType class.
 */
class Interface
{
  public:
    using Ptr = std::shared_ptr<Interface>;

    Interface() = default;
    virtual ~Interface() noexcept = default;

    /**
     *  Generates the XML introspection fragment for this property
     *
     * @return const std::string containing the XML data
     */
    virtual const std::string GenerateIntrospection() const = 0;

    /**
     *  Retrieve the D-Bus data type this property is declared as
     *
     * @return const char*  Containing the D-Bus specific data type reference.
     */
    virtual const char *GetDBusType() const noexcept = 0;

    /**
     *  Retrive the value of the property, as a GVariant object.
     *  This method is normally accessed via the internal
     *  @_int_dbusobject_callback_get_property callback function, which
     *  is called by glib2 interfaces when a D-Bus property is being read
     *  via the org.freedesktop.Properties.Get() method.
     *
     * @return GVariant* object containing the property value.
     */
    virtual GVariant *GetValue() const = 0;

    /**
     *  Changes the property value.  This is normally called by the
     *  internal @_int_dbusobject_callback_set_property callback function,
     *  which is called by the glib2 interfaces when a D-Bus property value
     *  is being set via the org.freedesktop.DBus.Properties.Set() method.
     *
     * @param value_arg     GVariant object containing the new value
     *                      for the property
     *
     * @return Property::Update object containing the information needed
     *         to emit the org.freedesktop.Properties.PropertyChanged signal.
     *         The signal itself is being emitted by the
     *         glib2::Callbacks::_int_dbusobject_callback_set_property
     *         callback method.
     */
    virtual Update::Ptr SetValue(GVariant *value_arg) = 0;

    /**
     *  Retrieve the D-Bus property name of this property.
     *
     * @return const std::string&
     */
    virtual const std::string &GetName() const noexcept = 0;


    /**
     *  Retrieve the object interface this property is bound to
     *
     * @return const std::string&
     */
    virtual const std::string &GetInterface() const noexcept = 0;


    /**
     *  Create a new @Properties::Update object for this property object
     *
     * @return Property::Update::Ptr
     */
    Update::Ptr PrepareUpdate() const
    {
        return Update::Create(*this);
    }
};



/**
 *  Property By Specification: Defines a property using a more
 *  complex data type than a single data type.
 *
 *  This will pass all the set and get property calls to specialized
 *  callback functions for this property.
 */
class BySpec : public Interface
{
  public:
    using GetPropertyCallback = std::function<GVariant *(const BySpec &)>;
    using SetPropertyCallback = std::function<Update::Ptr(const BySpec &, GVariant *)>;

    /**
     *  Creates a new PropertyBySpec object to be used by a
     *  Object::Base object.
     *
     * @param name        std::string with the D-Bus property name
     * @param readwr      boolean read/write flag. Read-only if false
     * @param dbustype    std::string containing the D-Bus data type of this
     *                    property
     * @param get_cb      GetPropertyCallback() functor. Called each time
     *                    this property is queried for its value
     * @param set_cb      SetPropertyCallback() functor. Called when this
     *                    property value is being changed via the D-Bus
     *                    org.freedesktop.DBus.Properties.Set() method.
     *
     * @return PropertyBySpec::Ptr
     */
    static BySpec::Ptr Create(const std::string &interface,
                              const std::string &name,
                              const bool readwr,
                              const std::string &dbustype,
                              GetPropertyCallback get_cb,
                              SetPropertyCallback set_cb)
    {
        BySpec::Ptr ret;
        ret.reset(new BySpec(interface,
                             name,
                             readwr,
                             dbustype,
                             get_cb,
                             set_cb));
        return ret;
    }

    virtual ~BySpec() noexcept = default;

    /**
     *  Generates the introspection XML fragment representing this
     *  property
     *
     * @return const std::string to be consumed by the D-Bus object
     *         introspection data generator.
     */
    const std::string GenerateIntrospection() const override;

    /**
     *  Returns a D-Bus data type this property is using
     *
     * @return const char* based string containing the D-Bus data type.
     */
    const char *GetDBusType() const noexcept override;

    /**
     *  Retrieve the value of this property.  This essentially
     *  calls the @GetPropCallback function this object was configured
     *  to use
     *
     * @return GVariant*  object containing the prepared value of this
     *         property
     */
    GVariant *GetValue() const override;

    /**
     *  Assign a new value from an incoming D-Bus request.  This
     *  essentially calls the @SetPropCallback function this object
     *  was configured to use
     *
     * @param value_arg     A pointer to the GVariant object contatining
     *                      the new value.
     *
     * @return Update::Ptr containing information for the
     *         org.freedesktop.DBus.Properties.PropertyChange signal
     *         the glibpp implementation will send automatically.
     */
    Update::Ptr SetValue(GVariant *value_arg) override;

    /**
     *  Retrieve the D-Bus property name of this property object.
     *
     * @return const std::string&
     */
    const std::string &GetName() const noexcept override;

    /**
     *  Retrieve the object interface this property is bound to
     *
     * @return const std::string&
     */
    virtual const std::string &GetInterface() const noexcept override;


  protected:
    /**
     * @see @PropertyBySpec::Create()
     *
     */
    BySpec(const std::string &interface_arg,
           const std::string &name_arg,
           const bool readwr_arg,
           const std::string &dbustype_arg,
           GetPropertyCallback get_cb,
           SetPropertyCallback set_cb);


  private:
    const std::string interface;            //<< D-Bus interface this property belongs to
    const std::string name;                 //<< D-Bus property name
    const bool readwrite;                   //<< Read/write or read-only property
    const std::string dbustype;             //<< D-Bus data type of this property
    const GetPropertyCallback get_callback; //<< Callback providing this property value
    const SetPropertyCallback set_callback; //<< Callback setting a new value to this property
};


/**
 *   This is the main class used by DBus::Object to store and manage all
 *   properties in a D-Bus object.  The data types supported by this
 *   implementation are fairly basic types.  For struct based types,
 *   another interface is needed.
 */
class Collection
{
  public:
    using Ptr = std::shared_ptr<Collection>;

    static Collection::Ptr Create()
    {
        return Collection::Ptr(new Collection);
    }

    /**
     *  Adds a new property to be managed
     *
     * @param prop  Property::Ptr to the property object to manage
     */
    void AddBinding(Property::Interface::Ptr prop);

    /**
     *  Checks if a specific named property exists or not
     *
     * @param name   std::string containing the property to look for
     * @returns Returns true if property is found, otherwise false.
     */
    bool Exists(const std::string &name) const noexcept;

    /**
     *  Generates the appropriate <property/> XML introspection
     *  elements based on all the properties being managed by this
     *  collection object
     *
     * @return const std::string  String containing just the <property/>
     *         fragments declaring all known properties.
     */
    const std::string GenerateIntrospection() const noexcept;

    /**
     *  Retrieve a GVariant representation containing the value of
     *  a specific property.  Used to return the property
     *  value directly in the the glib2 get property callback method.
     *
     * @param property_name  std::string containing the property name
     *                       to retrieve.
     *
     * @return GVariant*  Returns a pointer to a populated GVariant object
     *         containing the value of the property
     */
    GVariant *GetValue(const std::string &property_name) const;

    /**
     *  Extracts a new value from a GVariant object for a named property
     *  and changes the property value in the C++ variable bound to the
     *  property.
     *
     * @param property_name      Property name to update
     * @param value              GVariant pointer containing the new value
     *
     * @return GVariantBuilder*  Returns a pre-filled GVariantBuilder object
     *         which need to be used when sending the D-Bus PropertiesChanged
     *         signal.
     */
    Property::Update::Ptr SetValue(const std::string &property_name,
                                   GVariant *value);

  private:
    std::map<std::string, Property::Interface::Ptr> properties;

    Collection();
};


} // namespace Property
} // namespace Object
} // namespace DBus
