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


void CheckCapabilityFD(const GDBusConnection *dbuscon);


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
const std::string Extract(GVariant *value) noexcept;


// Declare template as prototype only so it cannot be used directly
template <typename T>
inline const char *DBus() noexcept;

template <>
inline const char *DBus<uint32_t>() noexcept
{
    return "u";
}

template <>
inline const char *DBus<int32_t>() noexcept
{
    return "i";
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
    return "i";
}
#endif

template <>
inline const char *DBus<uint16_t>() noexcept
{
    return "q";
}

template <>
inline const char *DBus<int16_t>() noexcept
{
    return "n";
}

template <>
inline const char *DBus<uint64_t>() noexcept
{
    return "t";
}

template <>
inline const char *DBus<int64_t>() noexcept
{
    return "x";
}

template <>
inline const char *DBus<std::byte>() noexcept
{
    // The D-Bus spec declares 'y' (BYTE) as "unsigned 8-bit integer",
    // but in C/C++ uint8_t has an ambiguity and is typedefed to
    // unsigned char
    return "y";
}

template <>
inline const char *DBus<double>() noexcept
{
    return "d";
}

template <>
inline const char *DBus<bool>() noexcept
{
    return "b";
}

template <>
inline const char *DBus<DBus::Object::Path>() noexcept
{
    return "o";
}

template <>
inline const char *DBus<std::string>() noexcept
{
    return "s";
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
 *  Converts a std::vector<T> to a D-Bus compliant
 *  array  builder of the D-Bus corresponding data type
 *
 * @param input  std::vector<T> to convert
 * @param override_type (optional) If set, it will use the given type instead
 *                      of deducting the D-Bus type for T
 *
 * @return Returns a GVariantBuilder object containing the complete array
 */
template <typename T>
inline GVariantBuilder *FromVector(const std::vector<T> input,
                                   const char *override_type = nullptr)
{
    std::string type = {};
    if (override_type)
    {
        type = std::string("a") + std::string(override_type);
    }
    else
    {
        type = std::string("a") + std::string(DataType::DBus<T>());
    }

    GVariantBuilder *bld = g_variant_builder_new(G_VARIANT_TYPE(type.c_str()));
    for (const auto &e : input)
    {
        Add(bld, e, override_type);
    }

    return bld;
}


/**
 *  Just a wrapper around @g_variant_builder_end() with a memory cleanup.
 *  This function is primarily here to provide a consistent C++ API.
 *
 * @param builder   GVariantBuilder object to complete
 * @return GVariant* The finialized GVariantBuilder object as a GVariant object
 */
GVariant *Finish(GVariantBuilder *builder) noexcept;


/**
 *  Wraps the content in a GVariantBuilder object into a tuple
 *  and returns the complete tuple as GVariant object.
 *
 *  This function will call @g_variant_builder_end() on the provided
 *  GVariantBuilder object and unrefs it.
 *
 *  @param bld the builder to wrap
 * @return the result of the builder wrapped into a tuple
 */
GVariant *FinishWrapped(GVariantBuilder *bld) noexcept;


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
 * @param wrapped         Is the result wrapped as a tuple
 * @return std::vector<T>
 */
template <typename T>
inline std::vector<T> ExtractVector(GVariant *params,
                                    const char *override_type = nullptr,
                                    bool wrapped = true) noexcept
{
    std::stringstream type;
    type << (wrapped ? "(" : "")
         << "a"
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
 *  Variant of the @Create function above, which converts the
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
 *  Variant of the @Create function above, which converts the
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
 *  data type.  Otherwise, the same as the @Create() methods above.
 *
 * @tparam T          C++ data type of the value
 * @param value       The C++ value
 * @return GVariant*
 */
template <typename T>
inline GVariant *Create(const T value) noexcept
{
    return g_variant_new(DataType::DBus<T>(), value);
}

/**
 *  Variant of the @Create function above, handling C++ DBus::Object::Path
 *  and converting it into a C type string (std::string::c_str())
 *
 * @param value        DBus::Object::Path with the value to store in GVariant
 * @return GVariant*
 */
template <>
inline GVariant *Create(DBus::Object::Path value) noexcept
{
    return g_variant_new("o", value.c_str());
}

/**
 *  Variant of the @Create function above, handling C++ std::string
 *  and converting it into a C type string (std::string::c_str())
 *
 * @param value        std::string of the GVariant object shall carry
 * @return GVariant*
 */
template <>
inline GVariant *Create(std::string value) noexcept
{
    return g_variant_new("s", value.c_str());
}


/**
 *  Converts a std::vector<T> to a D-Bus compliant
 *  array of the corresponding D-Bus data type
 *
 * @param input          std::vector<T> to convert
 * @param override_type  (optional) char * string with the D-Bus type string
 *                       to use for the GVariant container instead of the
 *                       template derived type.
 *
 * @return Returns a GVariant object containing the complete array
 */
template <typename T>
inline GVariant *CreateVector(const std::vector<T> &input,
                              const char *override_type = nullptr) noexcept
{
    GVariantBuilder *bld = Builder::FromVector(input, override_type);
    GVariant *ret = g_variant_builder_end(bld);
    g_variant_builder_unref(bld);

    return ret;
}


/**
 *  Wraps a single C++ value into a D-Bus tuple in a GVariant object
 *
 *  If T is std::string, the D-Bus data type in the GVariant object
 *  will then be '(s,)'.
 *
 * @tparam T           C++ data type
 * @param value        C++ variable content to wrap in
 * @param override_type  (optional) char * string with the D-Bus type string
 *                       to use for the GVariant container instead of the
 *                       template derived type.
 * @return GVariant*
 */
template <typename T>
inline GVariant *CreateTupleWrapped(const T &value,
                                    const char *override_type = nullptr) noexcept
{
    GVariant *v = nullptr;
    if (!override_type)
    {
        v = Create(value);
    }
    else
    {
        v = CreateType(override_type, value);
    }
    GVariantBuilder *bld = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
    g_variant_builder_add_value(bld, v);
    GVariant *ret = g_variant_builder_end(bld);
    g_variant_builder_unref(bld);
    return ret;
}


template <typename T>
inline GVariant *CreateTupleWrapped(const std::vector<T> &input,
                                    const char *override_type = nullptr) noexcept
{
    GVariantBuilder *bld = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
    g_variant_builder_add_value(bld, CreateVector(input, override_type));
    GVariant *ret = g_variant_builder_end(bld);
    g_variant_builder_unref(bld);
    return ret;
}


namespace Dict {

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

} // namespace Dict

} // namespace Value

} // namespace glib2
