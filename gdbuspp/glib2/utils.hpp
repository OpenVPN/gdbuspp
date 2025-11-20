//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Arne Schwabe <arne@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//  Copyright (C)  Lev Stipakov <lev@openvpn.net>
//

/**
 * @file   glib2/utils.hpp
 *
 * @brief  Various utility functions bridging the gap between glib2 C
 *         interfaces and modern C++
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include <string>
#include <sstream>
#include <type_traits>

#include <gio/gunixfdlist.h>

#include "../exceptions.hpp"
#include "../object/path.hpp"


namespace glib2 {

//////////////////////////////////////////////////////////////////////////
////                                                                  ////
////   namespace glib2::Utils                                         ////
////                                                                  ////
//////////////////////////////////////////////////////////////////////////

/**
 *  Generic glib2 utilities
 */
namespace Utils {

class Exception : public DBus::Exception
{
  public:
    Exception(const std::string &err, GError *gliberr = nullptr);
    Exception(const std::string &callfunc, const std::string &err, GError *gliberr = nullptr);
};


void CheckCapabilityFD(GDBusConnection *dbuscon);


/**
 * Unreferences an fd list. This is a helper function since the normal
 * g_unref_object does not fit the signature and there seem to be no
 * function to properly unref fdlists. The other examples that I found use
 * g_object_unref on the list
 *
 * @param the fd list
 */
void unref_fdlist(GUnixFDList *fdlist);


/**
 *  Validate the data type provided in a GVariant object against
 *  a string containing the expected data type.
 *
 * @param func     C string containing the calling functions name,
 *                 used if an exception is thrown
 * @param params   GVariant* containing the parameters
 * @param format   C string containing the expected data type string
 * @param num      Number of child elements in the GVariant object.
 *                 If 0, element count will not be considered.
 *
 * @throws glib2::Utils::Exception
 */
void checkParams(const char *func,
                 GVariant *params,
                 const char *format,
                 unsigned int num = 0);

} // namespace Utils



//////////////////////////////////////////////////////////////////////////
////                                                                  ////
////   namespace glib2::DataType                                      ////
////                                                                  ////
//////////////////////////////////////////////////////////////////////////



/**
 *  Functions via template specialization to return the
 *  D-Bus data type for a C/C++ type
 */
namespace DataType {

// clang-format off
constexpr const char *DBUS_TYPE_ARRAY      = "a";
constexpr const char *DBUS_TYPE_BOOL       = "b";
constexpr const char *DBUS_TYPE_BYTE       = "y";
constexpr const char *DBUS_TYPE_DOUBLE     = "d";
constexpr const char *DBUS_TYPE_INT16      = "n";
constexpr const char *DBUS_TYPE_INT32      = "i";
constexpr const char *DBUS_TYPE_INT64      = "x";
constexpr const char *DBUS_TYPE_OBJECTPATH = "o";
constexpr const char *DBUS_TYPE_STRING     = "s";
constexpr const char *DBUS_TYPE_TUPLE      = "r";
constexpr const char *DBUS_TYPE_UINT16     = "q";
constexpr const char *DBUS_TYPE_UINT32     = "u";
constexpr const char *DBUS_TYPE_UINT64     = "t";
constexpr const char *DBUS_TYPE_VARIANT    = "v";
// clang-format on


/**
 *  Retrieve the D-Bus notation of the data type stored
 *  in a GVariant object.
 *
 *  This is just a simple wrapper around g_variant_get_type_string(),
 *  but providing the result as a std::string value instead of char *
 *
 * @param v   GVariant object to inspect
 * @return std::string of the D-Bus data type representation of the value
 */
std::string Extract(GVariant *value) noexcept;


// Declare template as prototype only so it cannot be used directly
template <typename T>
inline const char *DBus() noexcept;

template <>
inline const char *DBus<uint32_t>() noexcept
{
    return DBUS_TYPE_UINT32;
}

template <>
inline const char *DBus<int32_t>() noexcept
{
    return DBUS_TYPE_INT32;
}

/*
 * Since long and int are separate types and int32_t is most times defined as
 * int on ILP32 platform, do we do this template magic to match all signed 32 bit
 * types instead of just int32_t
 */
#if 0 // FIXME::This is now failing on x86_64 - added GetDBusType<int32_t>() as workaround
template <typename T>
inline typename std::enable_if<sizeof(T) == 4 && std::is_signed<T>::value, const char *>::type DBus()
{
    return DBUS_TYPE_INT32;
}
#endif

template <>
inline const char *DBus<uint16_t>() noexcept
{
    return DBUS_TYPE_UINT16;
}

template <>
inline const char *DBus<int16_t>() noexcept
{
    return DBUS_TYPE_INT16;
}

template <>
inline const char *DBus<uint64_t>() noexcept
{
    return DBUS_TYPE_UINT64;
}

template <>
inline const char *DBus<int64_t>() noexcept
{
    return DBUS_TYPE_INT64;
}

template <>
inline const char *DBus<unsigned long long>() noexcept
{
    return DBUS_TYPE_UINT64;
}

template <>
inline const char *DBus<long long>() noexcept
{
    return DBUS_TYPE_INT64;
}

template <>
inline const char *DBus<std::byte>() noexcept
{
    // The D-Bus spec declares 'y' (BYTE) as "unsigned 8-bit integer",
    // but in C/C++ uint8_t has an ambiguity and is typedefed to
    // unsigned char
    return DBUS_TYPE_BYTE;
}

template <>
inline const char *DBus<double>() noexcept
{
    return DBUS_TYPE_DOUBLE;
}

template <>
inline const char *DBus<bool>() noexcept
{
    return DBUS_TYPE_BOOL;
}

template <>
inline const char *DBus<DBus::Object::Path>() noexcept
{
    return DBUS_TYPE_OBJECTPATH;
}

template <>
inline const char *DBus<std::string>() noexcept
{
    return DBUS_TYPE_STRING;
}

template <>
inline const char *DBus<GVariant *>() noexcept
{
    return DBUS_TYPE_VARIANT;
}

} // namespace DataType



//////////////////////////////////////////////////////////////////////////
////                                                                  ////
////   namespace glib2::Builder                                       ////
////                                                                  ////
//////////////////////////////////////////////////////////////////////////



namespace Builder {

/**
 *  Simple wrapper around g_variant_builder_new() to provide a more
 *  consistent C++ interface
 *
 * @param type               D-Bus data type this builder container expects
 * @return GVariantBuilder*
 */
GVariantBuilder *Create(const char *type);


/**
 *  Creates a new empty tuple GVariantBuilder object
 *
 *  Use Builder::Add() to add elements to this tuple
 *  object and close it using Builder::Finish()
 *
 * @return GVariantBuilder*
 */
GVariantBuilder *EmptyTuple();


/**
 *  Simple wrapper around g_variant_builder_open() to provide a more
 *  consistent C++ interface
 *
 * @param builder   GVariantBuilder object where to create the child nodes
 * @param type      D-Bus data type this builder container expects
 */
void OpenChild(GVariantBuilder *builder, const char *type);


/**
 *  Simple wrapper around g_variant_builder_close() to provide a more
 *  consistent C++ interface
 *
 * @param builder GVariantBuilder with the child node to close.  The
 *                OpenChild() must have been called first to prepare a child
 *                node
 */
void CloseChild(GVariantBuilder *builder);


/**
 *  Creates an empty variant object based on more complex types.
 *  The implementation of this method is very basic and simple, without
 *  any type checking, except of what @g_variant_builder_new() provides.
 *
 *  This function is useful in the property get callback method when
 *  an empty dictionary or array needs to be returned.
 *
 * @param type  String containing the D-Bus data type to return
 * @return Returns a new GVariant based object with the given data type but
 *         without any values.
 */
GVariant *CreateEmpty(const char *type);


/**
 *  Just a wrapper around @g_variant_builder_end() with a memory cleanup.
 *  This function is primarily here to provide a consistent C++ API.
 *
 * @param builder   GVariantBuilder object to complete
 * @return GVariant* The finialized GVariantBuilder object as a GVariant object
 */
GVariant *Finish(GVariantBuilder *builder) noexcept;


/**
 *   Template variant of GLib2's @g_variant_builder_add() which extract
 *   the D-Bus data type automatically via the data type passed.
 *
 *   @param builder  GVariantBuilder object where to add the value
 *   @param value    Templated value to add to the GVariantBuilder object
 *   @param override_type (optional) If set, it will use the given type instead
 *                        of deducting the D-Bus type for T
 */
template <typename T>
inline void Add(GVariantBuilder *builder,
                const T &value,
                const char *override_type = nullptr) noexcept
{
    if (override_type)
    {
        g_variant_builder_add(builder, override_type, value);
    }
    else
    {
        g_variant_builder_add(builder, DataType::DBus<T>(), value);
    }
}


/**
 *   Variant of @GVariantBuilderAdd() which tackles C++ DBus::Object::Path
 *
 *   @param builder  GVariantBuilder object where to add the value
 *   @param value    DBus::Object::Path to add the GVariantBuilder object
 *   @param override_type (optional) If set, it will use the given type instead
 *                        of using std::string as the expected type
 */
template <>
inline void Add(GVariantBuilder *builder,
                const DBus::Object::Path &path,
                const char *override_type) noexcept
{
    if (override_type)
    {
        g_variant_builder_add(builder, override_type, path.c_str());
    }
    else
    {
        g_variant_builder_add(builder,
                              DataType::DBus<DBus::Object::Path>(),
                              path.c_str());
    }
}

/**
 *   Variant of @GVariantBuilderAdd() which tackles C++ std::string
 *
 *   @param builder  GVariantBuilder object where to add the value
 *   @param value    std::string value to add to the GVariantBuilder object
 *   @param override_type (optional) If set, it will use the given type instead
 *                        of using std::string as the expected type
 */
template <>
inline void Add(GVariantBuilder *builder,
                const std::string &value,
                const char *override_type) noexcept
{
    if (override_type)
    {
        g_variant_builder_add(builder, override_type, value.c_str());
    }
    else
    {
        g_variant_builder_add(builder, DataType::DBus<std::string>(), value.c_str());
    }
}


/**
 *  Just a wrapper fpr adding an already prepared GVariant object
 *  to a builder object.  This function is primarily here to provide
 *  a consistent C++ API.
 *
 * @param builder   GVariantBuilder object where to add the value
 * @param value     GVariant object to add to the builder object
 */
inline void Add(GVariantBuilder *builder,
                GVariant *value) noexcept
{
    g_variant_builder_add_value(builder, value);
}


/**
 *  Add a "dictionary type of element" to a GVariantBuilder object
 *
 *  Normally glib2::Dict::Add() would be used, but that approach
 *  enforces the value to be `DBUS_TYPE_VARIANT` and the data type for
 *  key is always `DBUS_TYPE_STRING`.  For use cases where the data
 *  types need to be more specific it is needed to use GVariantBuilder
 *  instead.
 *
 *  To use this feature, a GVariantBuilder container needs to be
 *  created with the correct data types for both the key and value
 *  parts.
 *
 *  Example 1: A simple dictionary with string as the data type for
 *             both key and value
 *             @code
 *                GVariantBuilder *dictionary = glib2::Builder::Create("a{ss}");
 *                glib2::Builder::AddKeyValue<std::string, std::string>(dictionary, "my-key", "My Value");
 *             @endcode
 *
 *  Example 2: A simple dictionary with string as the data type for
 *             the key and int64_t for the value
 *             @code
 *                GVariantBuilder *dictionary = glib2::Builder::Create("a{sx}");
 *                glib2::Builder::AddKeyValue<std::string, int64_t>(dictionary, "my-key", 123456789);
 *             @endcode
 *
 *  Example 3: A simple dictionary with uint16_t as the data type for
 *             the key and bool for the value
 *             @code
 *                GVariantBuilder *dictionary = glib2::Builder::Create("a{qb}");
 *                glib2::Builder::AddKeyValue<uint16_t, bool>(dictionary, 1, true);
 *             @endcode
 *
 * @tparam TKEY    C++ data type for the key element
 * @tparam TVAL    C++ data type for the value element
 * @param builder  GVariantBuilder object to insert the key/value pairs into
 * @param key      Dictionary key element
 * @param value    Dictionary value element to be assigned to the key
 */
template <typename TKEY, typename TVAL>
inline void AddKeyValue(GVariantBuilder *builder,
                        const TKEY &key,
                        const TVAL &value)
{
    std::string dbus_type = std::string("{") + DataType::DBus<TKEY>() + DataType::DBus<TVAL>() + "}";
    g_variant_builder_add(builder, dbus_type.c_str(), key, value);
}


/**
 *  Variant of AddKeyValue() where the data type for the dictionary `value` is `std::string`
 *
 *  @copydetails glib2::Builder::AddKeyValue()
 */
template <typename TKEY, typename TVAL>
inline void AddKeyValue(GVariantBuilder *builder,
                        const TKEY &key,
                        const std::string &value)
{
    std::string dbus_type = std::string("{") + DataType::DBus<TKEY>() + DataType::DBus<TVAL>() + "}";
    g_variant_builder_add(builder, dbus_type.c_str(), key, value.c_str());
}

/**
 *  Variant of AddKeyValue() where the data type for the dictionary `key` is `std::string`
 *
 *  @copydetails glib2::Builder::AddKeyValue()
 */
template <typename TKEY, typename TVAL>
inline void AddKeyValue(GVariantBuilder *builder,
                        const std::string &key,
                        const TVAL &value)
{
    std::string dbus_type = std::string("{") + DataType::DBus<TKEY>() + DataType::DBus<TVAL>() + "}";
    g_variant_builder_add(builder, dbus_type.c_str(), key.c_str(), value);
}

/**
 *  Variant of AddKeyValue() where the data type for both the dictionary `key` and `value` are `std::string`
 *
 *  @copydetails glib2::Builder::AddKeyValue()
 */
template <typename TKEY, typename TVAL>
inline void AddKeyValue(GVariantBuilder *builder,
                        const std::string &key,
                        const std::string &value)
{
    std::string dbus_type = std::string("{") + DataType::DBus<TKEY>() + DataType::DBus<TVAL>() + "}";
    g_variant_builder_add(builder, dbus_type.c_str(), key.c_str(), value.c_str());
}



/**
 *  Add structured data to a GVariantBuilder object.
 *
 *  This is used to process a C/C++ struct based object into
 *  a GVariant object which can be added to a GVariantBuilder
 *  object.  This is achieved by using a callback (lambda) function
 *  which receives the C/C++ struct as an argument and prepares
 *  the GVariant object which it need to return.
 *
 *  A simple example for a single element struct:
 *
 *  @code
 *     struct value_container
 *     {
 *           uint32_t my_int;
 *     };
 *
 *     auto process_callback = [](void const *data_ptr) -> GVariant *
 *     {
 *         const auto *data = static_cast<const value_container *>(data_ptr);
 *         return glib2::Value::Create(data->my_int);
 *     }
 *
 *     value_container my_data;
 *     my_data.my_int = 12345;
 *
 *     auto *builder = glib2::Builder::Create("(u)");
 *     glib2::Builder::AddStructData(builder, my_data, process_callback);
 *     GVariant *result = glib2::Builder::Finish(builder);
 *  @endcode
 *
 * @tparam T            C/C++ struct data type
 * @param builder       GVariantBuilder object to add the data to
 * @param struct_data   Const reference to the data to process
 * @param extractor     Callback function used to process `struct_data`
 */
template <typename T>
inline void AddStructData(GVariantBuilder *builder,
                          const T &struct_data,
                          std::function<GVariant *(void const *data_ptr)> extractor)
{
    GVariant *data = extractor(static_cast<void const *>(&struct_data));
    Builder::Add(builder, data);
}


/**
 *  Add a vector/array of structured data elements into a GVariantBuilder object
 *
 *  This is the std::vector<> version of @AddStructData(), which will iterate
 *  through all the elements and add them to the GVariantBuilder object.
 *
 *  The D-Bus data type of the GVariantBuilder object must be typed as an
 *  array of a struct.
 *
 *  Extending the example code from @AddStructData(), an array version
 *  will be like this:
 *
 *  @code
 *     std::vector<value_container> my_value_array;
 *     // data need to be inserted; not demonstrated here
 *
 *     auto *builder = glib2::Builder::Create("a(u)");
 *     glib2::Builder::AddStructData(builder, my_value_array, process_callback);
 *     GVariant *array_result = glib2::Builder::Finish(builder);
 *  @endcode
 *
 *  See the Test::Program::PropertyTests::GetMoreComplexProperty() method in tests/simple-service.cpp
 *  for a complete example.
 *
 * @tparam T            C/C++ struct data type used in the std::vector
 * @param builder       GVariantBuilder object to add the data to
 * @param struct_data   Const reference to the data to process
 * @param extractor     Callback function used to process `struct_data`
 */
template <typename T>
inline void AddStructData(GVariantBuilder *main_builder,
                          const std::vector<T> &data_set,
                          std::function<GVariant *(void const *data_ptr)> extractor)
{
    if (data_set.empty())
    {
        return;
    }

    std::vector<GVariant *> elements;
    for (auto rec : data_set)
    {
        elements.push_back(extractor(static_cast<void const *>(&rec)));
    }

    std::string dbus_type = DataType::DBUS_TYPE_ARRAY
                            + std::string(g_variant_get_type_string(elements[0]));
    GVariantBuilder *array_builder = glib2::Builder::Create(dbus_type.c_str());
    for (const auto &data : elements)
    {
        glib2::Builder::Add(array_builder, data);
    }
    GVariant *array_entries = glib2::Builder::Finish(array_builder);
    glib2::Builder::Add(main_builder, array_entries);
}


/**
 *  Creates and populates a new GVariantBuilder object based
 *  on the content of a std::vector<T>.
 *
 * @param input         std::vector<T> to convert
 * @param override_type (optional) If set, it will use the given type instead
 *                      of deducting the D-Bus type for T
 *
 * @return Returns a GVariantBuilder object containing the complete array
 */
template <typename T>
inline GVariantBuilder *FromVector(const std::vector<T> &input,
                                   const char *override_type = nullptr)
{
    std::string type = DataType::DBUS_TYPE_ARRAY + std::string(override_type ? override_type : DataType::DBus<T>());
    GVariantBuilder *bld = g_variant_builder_new(G_VARIANT_TYPE(type.c_str()));
    for (const auto &e : input)
    {
        glib2::Builder::Add(bld, e, override_type);
    }

    return bld;
}


/**
 *  Adds the content of a std::vector<T> into an existing
 *  GVariantBuilder object
 *
 * @tparam T             C++ data type of the std::vector
 * @param builder        GVariantBuilder object to add the data to
 * @param vector_value   std::vector<T> containing the data to add
 * @param override_type  (optional) If set, it will use the given type instead
 *                       of deducting the D-Bus type for T
 */
template <typename T>
inline void Add(GVariantBuilder *builder,
                const std::vector<T> &vector_value,
                const char *override_type = nullptr) noexcept
{
    GVariantBuilder *bld = glib2::Builder::FromVector(vector_value, override_type);
    glib2::Builder::Add(builder, glib2::Builder::Finish(bld));
}

} // namespace Builder



//////////////////////////////////////////////////////////////////////////
////                                                                  ////
////   namespace glib2::Value                                         ////
////                                                                  ////
//////////////////////////////////////////////////////////////////////////



/**
 *  Functions handling values stored in a GVariant object
 */
namespace Value {
/*
 * These overloaded Get with multiple int and std::string
 * types are here to allow the vector template to be as generic as
 * possible while still calling the right D-Bus function
 *
 */

// Declare template as prototype only so it cannot be used directly
template <typename T>
inline T Get(GVariant *v) noexcept;


template <>
inline double Get<double>(GVariant *v) noexcept
{
    return g_variant_get_double(v);
}

template <>
inline uint32_t Get<uint32_t>(GVariant *v) noexcept
{
    return g_variant_get_uint32(v);
}

template <>
inline int32_t Get<int32_t>(GVariant *v) noexcept
{
    return g_variant_get_int32(v);
}

/*
 * Since long and int are separate types and int32_t is most times defined as
 * int on ILP32 platform, do we do this template magic to match all signed 32 bit
 * types instead of just int32_t
 */
#if 0 // FIXME::This is now failing on x86_64 - added GetVariantValue<int32_t>() as workaround
template <typename T>
inline typename std::enable_if<sizeof(T) == 4 && std::is_signed<T>::value, int32_t>::type
Get(GVariant *v) noexcept
{
    return g_variant_get_int32(v);
}
#endif

template <>
inline std::byte Get<std::byte>(GVariant *v) noexcept
{
    // The D-Bus spec declares 'y' as "unsigned 8-bit integer",
    // but in C/C++ uint8_t has an ambiguity and is typedefed to
    // unsigned char
    return static_cast<std::byte>(g_variant_get_byte(v));
}

template <>
inline uint16_t Get<uint16_t>(GVariant *v) noexcept
{
    return g_variant_get_uint16(v);
}

template <>
inline int16_t Get<int16_t>(GVariant *v) noexcept
{
    return g_variant_get_int16(v);
}

template <>
inline uint64_t Get<uint64_t>(GVariant *v) noexcept
{
    return g_variant_get_uint64(v);
}

template <>
inline int64_t Get<int64_t>(GVariant *v) noexcept
{
    return g_variant_get_int64(v);
}

template <>
inline bool Get<bool>(GVariant *v) noexcept
{
    return g_variant_get_boolean(v);
}

template <>
inline DBus::Object::Path Get<DBus::Object::Path>(GVariant *v) noexcept
{
    const char *val = g_variant_get_string(v, nullptr);
    return DBus::Object::Path((val ? val : ""));
}


template <>
inline std::string Get<std::string>(GVariant *v) noexcept
{
    const char *val = g_variant_get_string(v, nullptr);
    return std::string((val ? val : ""));
}


template <>
inline GVariant *Get<GVariant *>(GVariant *v) noexcept
{
    return g_variant_get_variant(v);
}


/*
 * These methods extracts values from a GVariant object containing
 * values in a tuple.  This is used as an alternative to g_variant_get()
 * which may cause explosions in some situations.  This method seems to
 * work more reliable and with with less surprises.
 */

// Declare template as prototype only so it cannot be used directly
template <typename T>
inline T Extract(GVariant *v, int elm) noexcept;

template <>
inline double Extract<double>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    double ret = Get<double>(bv);
    g_variant_unref(bv);
    return ret;
}


template <>
inline uint64_t Extract<uint64_t>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    uint64_t ret = Get<uint64_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline int64_t Extract<int64_t>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    int64_t ret = Get<int64_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline uint32_t Extract<uint32_t>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    uint32_t ret = Get<uint32_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline int32_t Extract<int32_t>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    int32_t ret = Get<int32_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline uint16_t Extract<uint16_t>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    uint16_t ret = Get<uint16_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline int16_t Extract<int16_t>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    int16_t ret = Get<int16_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline bool Extract<bool>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    bool ret = Get<bool>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline DBus::Object::Path Extract<DBus::Object::Path>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    DBus::Object::Path ret{Get<DBus::Object::Path>(bv)};
    g_variant_unref(bv);
    return ret;
}

template <>
inline std::string Extract<std::string>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    std::string ret = Get<std::string>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline GVariant *Extract<GVariant *>(GVariant *v, int elm) noexcept
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    auto ret = Get<GVariant *>(bv);
    g_variant_unref(bv);
    return ret;
}


/**
 *  Parses a GVariant object containing a an array/list of a single
 *  D-Bus data type into a C++ std::vector of the same corresponding
 *  data type.
 *
 * @tparam T              Data type of the std::vector result type
 * @param params          GVariant pointer to the data to parse
 * @param override_type   (optional) Override the data type. Normally extracted
                          via the C++ data type (T), but some types may need
                          a different D-Bus data type
 * @return std::vector<T>
 */
template <typename T>
inline std::vector<T> ExtractVector(GVariant *params,
                                    const char *override_type = nullptr) noexcept
{
    std::stringstream type;
    const bool wrapped = g_variant_type_is_tuple(g_variant_get_type(params));
    type << (wrapped ? "(" : "")
         << DataType::DBUS_TYPE_ARRAY
         << (override_type ? std::string(override_type) : std::string(DataType::DBus<T>()))
         << (wrapped ? ")" : "");

    GVariantIter *list = nullptr;
    g_variant_get(params, type.str().c_str(), &list);

    std::vector<T> ret;
    ret.reserve(g_variant_iter_n_children(list));

    GVariant *rec = nullptr;
    while ((rec = g_variant_iter_next_value(list)))
    {
        ret.emplace_back(Value::Get<T>(rec));
        g_variant_unref(rec);
    }
    g_variant_unref(params);
    g_variant_iter_free(list);
    return ret;
}


/**
 *  Helper function to simplify iterating a GVariant array container.
 *
 *  The \p array argument need to be an array or an array inside a tuple.
 *  For each element in the the array, the function passed as the \p parser
 *  argument will be called, which provides access to a single array element.
 *
 * @param array    GVariant container object carrying the array to iterate
 * @param parser   Function to be called for each array element
 * @throws glib2::Utils::Exception if the data type of the array data is
 *         incorrect, otherwise the exceptions produced by the parser function
 */
void IterateArray(GVariant *array, std::function<void(GVariant *data)> parser);


/**
 *  Creates a new, empty GVariant object of
 *  the "variant" D-Bus type
 *
 * @return GVariant*
 */
GVariant *NullVariant();


/**
 *  Templatized wrapper for g_variant_new() which returns a
 *  GVariant object with the provided D-Bus data type and value
 *
 *  This function ensure the g_variant_new() is called using the
 *  proper C/C++ data type, via the template (T).
 *
 * @tparam T
 * @param dbustype    D-Bus data type to use
 * @param value       The value the new GVariant object shall carry
 * @return GVariant*
 */
template <typename T>
inline GVariant *CreateType(const char *dbustype, const T &value) noexcept
{
    return g_variant_new(dbustype, value);
}

/**
 *  Variant of the Create() function above, which converts the
 *  C++ DBus::Object::Path type to a C type string, via the
 *  std::string::c_str() method
 *
 * @param dbustype     D-Bus data type to use
 * @param value        DBus::Object::Path the GVariant object shall carry
 * @return GVariant*
 */
template <>
inline GVariant *CreateType(const char *dbustype, const DBus::Object::Path &value) noexcept
{
    return g_variant_new(dbustype, value.c_str());
}


/**
 *  Variant of the Create() function above, which converts the
 *  C++ std::string type to a C type string, via the
 * std::string::c_str() method

 * @param dbustype     D-Bus data type to use
 * @param value        std::string of the GVariant object shall carry
 * @return GVariant*
 */
template <>
inline GVariant *CreateType(const char *dbustype, const std::string &value) noexcept
{
    return g_variant_new(dbustype, value.c_str());
}

/**
 *  Templatized and simpler g_variant_new() wrapper which automatically
 *  converts the C++ data type of the value into the proper D-Bus
 *  data type.  Otherwise, the same as the Create() methods above.
 *
 * @tparam T          C++ data type of the value
 * @param value       The C++ value
 * @return GVariant*
 */
template <typename T>
inline GVariant *Create(const T &value) noexcept
{
    return g_variant_new(DataType::DBus<T>(), value);
}

/**
 *  Variant of the Create() function above, handling C++ DBus::Object::Path
 *  and converting it into a C type string (std::string::c_str())
 *
 * @param value        DBus::Object::Path with the value to store in GVariant
 * @return GVariant*
 */
template <>
inline GVariant *Create(const DBus::Object::Path &value) noexcept
{
    return g_variant_new("o", value.c_str());
}

/**
 *  Variant of the Create() function above, handling C++ std::string
 *  and converting it into a C type string (std::string::c_str())
 *
 * @param value        std::string of the GVariant object shall carry
 * @return GVariant*
 */
template <>
inline GVariant *Create(const std::string &value) noexcept
{
    return g_variant_new("s", value.c_str());
}


/**
 *  Variant of the Create() function which works on std::vector
 *
 * @tparam T           C++ data type of value
 * @param value        The C++ value
 * @return GVariant*
 */
template <typename T>
inline GVariant *Create(const std::vector<T> &value) noexcept
{
    std::string type = DataType::DBUS_TYPE_ARRAY + std::string(DataType::DBus<T>());
    GVariantBuilder *bld = Builder::Create(type.c_str());
    for (const auto &elem : value)
    {
        {
            Builder::Add(bld, elem);
        }
    }
    return Builder::Finish(bld);
}


/**
 *  Wraps the data in a GVariant object inside a D-Bus tuple
 *  unless it is already wrapped.
 *
 * @param data        GVariant* data to wrap
 * @return GVariant*
 */
GVariant *TupleWrap(GVariant *data);

} // namespace Value


namespace Dict {

/**
 *  Create a new GVariant based dictionary.
 *
 *  This method is a convenience wrapper around g_variant_dict_new(),
 *  to provide a more unified glib2::Dict API.
 *
 *  @note This approach will always result in the D-Bus data type `a{sv}`.
 *  If a specialized data type is required for the key or value, the
 *  glib2::Builder::Create() and glib2::Builder::AddKeyValue() functions
 *  need to be used; only this approach allows specifying the data types.
 *
 * @return GVariantDict*
 */
inline GVariantDict *Create() noexcept
{
    return g_variant_dict_new(nullptr);
}


/**
 *  Add a new GVariant object to the GVariant based dictionary.
 *
 * @param dict   GVariantDict object holding the dictionary
 * @param key    std::string with the dictionary key
 * @param value  GVariant object to store in the dictionary
 */
inline void Add(GVariantDict *dict, const std::string &key, GVariant *value) noexcept
{
    g_variant_dict_insert_value(dict, key.c_str(), value);
}


/**
 *  Adds a new value to the GVariant based dictionary.
 *
 *  This method is a convenience wrapper around g_variant_dict_insert_value(),
 *  to provide a more unified glib2::Dict API.  But it will also take care of
 *  converting the C++ data type and create the appropriate GVariant object
 *  holding the data, with the correct D-Bus data type.
 *
 * @tparam T     C++ data type of the value
 * @param dict   GVariantDict object holding the dictionary
 * @param key    std:::string with the dictionary key
 * @param value  The C++ value to store in the dictionary
 */
template <typename T>
inline void Add(GVariantDict *dict, const std::string &key, const T &value) noexcept
{
    g_variant_dict_insert_value(dict, key.c_str(), glib2::Value::Create<T>(value));
}


/**
 *  Adds a new vector/array value to the GVariant based dictionary.
 *
 *  This method is a convenience wrapper around g_variant_dict_insert_value(),
 *  to provide a more unified glib2::Dict API.  But it will also take care of
 *  converting the C++ data type and create the appropriate GVariant object
 *  holding the vector data, with the correct D-Bus data type.
 *
 * @tparam T     C++ data type of the vector values
 * @param dict   GVariantDict object holding the dictionary
 * @param key    std:::string with the dictionary key
 * @param value  The C++ std::vector<T> holding the vector to store in the dictionary
 */
template <typename T>
inline void Add(GVariantDict *dict, const std::string &key, const std::vector<T> &value) noexcept
{
    g_variant_dict_insert_value(dict, key.c_str(), glib2::Value::Create<T>(value));
}


/**
 *  This wraps up the GVariantDict object into a GVariant object which
 *  is used when passing the dictionary through various D-Bus APIs.
 *
 *  This method is a convenience wrapper around g_variant_dict_end(),
 *  to provide a more unified glib2::Dict API.
 *
 * @param dict        GVariantDict object holding the dictionary
 * @return GVariant*  object containing the finalised dictionary object.
 */
inline GVariant *Finish(GVariantDict *dict) noexcept
{
    GVariant *ret = g_variant_dict_end(dict);
    g_variant_dict_unref(dict);
    return ret;
}


/**
 *  Extract a value from a GVariant based dictionary container object.
 *  Such objects typically have D-Bus data type like 'a{sv}', which works
 *  like the ordinay key/value based dictionary containers.
 *
 *  This takes a key to look up from a GVariant object and returns the
 *  value as the data type it's declared as.  There must be a strict match
 *  between the D-Bus and C++ data type.
 *
 * @tparam T     C++ data type of the value to retrieve
 * @param dict   GVariant object containing the dictionary
 * @param key    std::string with the dictionary key to look up
 * @return T     The value retrieved from the dictionary
 * @throw glib2::Utils::Exception if the value could not be retrieved
 */
template <typename T>
T Lookup(GVariant *dict, const std::string &key)
{
    GVariant *v = g_variant_lookup_value(dict,
                                         key.c_str(),
                                         G_VARIANT_TYPE(DataType::DBus<T>()));
    if (!v)
    {
        throw glib2::Utils::Exception("Dict::Lookup",
                                      std::string("Could not retrieve the ")
                                          + "value for key '" + key + "'");
    }
    T ret = Value::Get<T>(v);
    g_variant_unref(v);
    return ret;
}


/**
 *  This is the vector variant of Dict::Lookup(), it the return value
 *  will be std::vector<T> instead of just T.
 *
 *  Extract a value from a GVariant based dictionary container object.
 *  Such objects typically have D-Bus data type like 'a{sv}', where the 'v'
 *  value is a GVarint based array, list or tuple.
 *
 *  This takes a key to look up from a GVariant object and returns the
 *  value as the data type it's declared as.  There must be a strict match
 *  between the D-Bus and C++ data type.
 *
 * @tparam T
 * @param dict      GVariant object holding the dictionary
 * @param key       std::string with the dictionary key to look up
 * @return std::vector<T>
 * @throw glib2::Utils::Exception if the value could not be retrieved
 */
template <typename T>
std::vector<T> LookupVector(GVariant *dict, const std::string &key)
{
    std::string dbus_type = DataType::DBUS_TYPE_ARRAY
                            + std::string(DataType::DBus<T>());
    GVariant *v = g_variant_lookup_value(dict,
                                         key.c_str(),
                                         G_VARIANT_TYPE(dbus_type.c_str()));
    if (!v)
    {
        throw glib2::Utils::Exception("Dict::Lookup",
                                      std::string("Could not retrieve the ")
                                          + "value for key '" + key + "'");
    }

    return Value::ExtractVector<T>(v, DataType::DBus<T>());
}


/**
 *  Helper function which will iterate through all the dictionary elements
 *  in a GVariant object.  The data type of the GVariant object MUST be
 *  `a{sv}`, otherwise an exception will be thrown.
 *
 *  For each element iteration it will call an arbitrary extractor function
 *  provided by the user of this function.  This exatractor function takes
 *  two arguments: `const std::string &key` and `GVariant *value`, which
 *  represents `{sv}` part of the dictionary container.  This function can
 *  not return any data.
 *
 * @param dict       GVariant object containing the dictionary
 * @param extractor  Function to be called for each element.  Function declaration
 *                   MUST be: `void(const std::string &key, GVariant *value)`
 * @throws glib2::Utils::Exception when `*dict` is not of the `a{sv}` data type.
 *         The excpetions from the extractor function will be implicitly forwarded.
 */
void IterateDictionary(GVariant *dict, std::function<void(const std::string &key, GVariant *value)> extractor);

} // namespace Dict

} // namespace glib2
