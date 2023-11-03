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

#include <vector>
#include <string>
#include <sstream>
#include <type_traits>

#include <gio/gunixfdlist.h>

#include "../exceptions.hpp"


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

// Declare template as prototype only so it cannot be used directly
template <typename T>
inline const char *GetDBusType();

template <>
inline const char *GetDBusType<uint32_t>()
{
    return "u";
}

template <>
inline const char *GetDBusType<int32_t>()
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
inline typename std::enable_if<sizeof(T) == 4 && std::is_signed<T>::value, const char *>::type
GetDBusDataType()
{
    return "i";
}
#endif

template <>
inline const char *GetDBusType<uint16_t>()
{
    return "q";
}

template <>
inline const char *GetDBusType<int16_t>()
{
    return "n";
}

template <>
inline const char *GetDBusType<uint64_t>()
{
    return "t";
}

template <>
inline const char *GetDBusType<int64_t>()
{
    return "x";
}

template <>
inline const char *GetDBusType<double>()
{
    return "d";
}

template <>
inline const char *GetDBusType<bool>()
{
    return "b";
}

template <>
inline const char *GetDBusType<std::string>()
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
 *  Wraps the content in a GVariantBuilder object into a tuple
 *  and returns the complete tuple as GVariant object.
 *
 *  This function will call @g_variant_builder_end() on the provided
 *  GVariantBuilder object and unrefs it.
 *
 *  @param bld the builder to wrap
 * @return the result of the builder wrapped into a tuple
 */
GVariant *TupleWrap(GVariantBuilder *bld);


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
                const char *override_type = nullptr)
{
    if (override_type)
    {
        g_variant_builder_add(builder, override_type, value);
    }
    else
    {
        g_variant_builder_add(builder, DataType::GetDBusType<T>(), value);
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
                const char *override_type)
{
    if (override_type)
    {
        g_variant_builder_add(builder, override_type, value.c_str());
    }
    else
    {
        g_variant_builder_add(builder, DataType::GetDBusType<std::string>(), value.c_str());
    }
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
        type = std::string("a") + std::string(DataType::GetDBusType<T>());
    }

    GVariantBuilder *bld = g_variant_builder_new(G_VARIANT_TYPE(type.c_str()));
    for (const auto &e : input)
    {
        Add(bld, e, override_type);
    }

    return bld;
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
inline T Get(GVariant *v);


template <>
inline double Get<double>(GVariant *v)
{
    return g_variant_get_double(v);
}

template <>
inline uint32_t Get<uint32_t>(GVariant *v)
{
    return g_variant_get_uint32(v);
}

template <>
inline int32_t Get<int32_t>(GVariant *v)
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
Get(GVariant *v)
{
    return g_variant_get_int32(v);
}
#endif

template <>
inline uint16_t Get<uint16_t>(GVariant *v)
{
    return g_variant_get_uint16(v);
}

template <>
inline int16_t Get<int16_t>(GVariant *v)
{
    return g_variant_get_int16(v);
}

template <>
inline uint64_t Get<uint64_t>(GVariant *v)
{
    return g_variant_get_uint64(v);
}

template <>
inline int64_t Get<int64_t>(GVariant *v)
{
    return g_variant_get_int64(v);
}

template <>
inline bool Get<bool>(GVariant *v)
{
    return g_variant_get_boolean(v);
}

template <>
inline std::string Get<std::string>(GVariant *v)
{
    gsize size = 0;
    return std::string(g_variant_get_string(v, &size));
}


/*
 * These methods extracts values from a GVariant object containing
 * values in a tuple.  This is used as an alternative to g_variant_get()
 * which may cause explosions in some situations.  This method seems to
 * work more reliable and with with less surprises.
 */

// Declare template as prototype only so it cannot be used directly
template <typename T>
inline T Extract(GVariant *v, int elm);

template <>
inline double Extract<double>(GVariant *v, int elm)
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    double ret = Get<double>(bv);
    g_variant_unref(bv);
    return ret;
}


template <>
inline uint64_t Extract<uint64_t>(GVariant *v, int elm)
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    uint64_t ret = Get<uint64_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline int64_t Extract<int64_t>(GVariant *v, int elm)
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    int64_t ret = Get<int64_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline uint32_t Extract<uint32_t>(GVariant *v, int elm)
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    uint32_t ret = Get<uint32_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline int32_t Extract<int32_t>(GVariant *v, int elm)
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    int32_t ret = Get<int32_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline uint16_t Extract<uint16_t>(GVariant *v, int elm)
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    uint16_t ret = Get<uint16_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline int16_t Extract<int16_t>(GVariant *v, int elm)
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    int16_t ret = Get<int16_t>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline bool Extract<bool>(GVariant *v, int elm)
{
    GVariant *bv = g_variant_get_child_value(v, elm);
    bool ret = Get<bool>(bv);
    g_variant_unref(bv);
    return ret;
}

template <>
inline std::string Extract<std::string>(GVariant *v, int elm)
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
inline std::vector<T> ExtractVector(GVariant *params, const char *override_type = nullptr, bool wrapped = true)
{
    std::stringstream type;
    type << (wrapped ? "(" : "")
         << "a"
         << (override_type ? std::string(override_type) : std::string(DataType::GetDBusType<T>()))
         << (wrapped ? ")" : "");

    GVariantIter *list = nullptr;
    g_variant_get(params, type.str().c_str(), &list);

    std::vector<T> ret = {};
    GVariant *rec = nullptr;
    while ((rec = g_variant_iter_next_value(list)))
    {
        ret.push_back(Value::Get<T>(rec));
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
inline GVariant *Create(const char *dbustype, const T &value)
{
    return g_variant_new(dbustype, value);
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
inline GVariant *Create(const char *dbustype, const std::string &value)
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
inline GVariant *Create(const T &value)
{
    return g_variant_new(DataType::GetDBusType<T>(), value);
}

/**
 *  Variant of the @Create function above, handling C++ std::string
 *  and converting it into a C type string (std::string::c_str())
 *
 * @param value        std::string of the GVariant object shall carry
 * @return GVariant*
 */
template <>
inline GVariant *Create(const std::string &value)
{
    return g_variant_new(DataType::GetDBusType<std::string>(), value.c_str());
}

/**
 *  Wraps a single C++ value into a D-Bus tuple in a GVariant object
 *
 *  If T is std::string, the D-Bus data type in the GVariant object
 *  will then be '(s,)'.
 *
 * @tparam T           C++ data type
 * @param value        C++ variable content to wrap in
 * @return GVariant*
 */
template <typename T>
inline GVariant *CreateTupleWrapped(const T &value)
{
    GVariantBuilder *bld = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
    g_variant_builder_add_value(bld, Create(value));
    GVariant *ret = g_variant_builder_end(bld);
    g_variant_builder_unref(bld);
    return ret;
}


/**
 *  Converts a std::vector<T> to a D-Bus compliant
 *  array of the corresponding D-Bus data type
 *
 * @param input  std::vector<T> to convert
 *
 * @return Returns a GVariant object containing the complete array
 */
template <typename T>
inline GVariant *CreateVector(const std::vector<T> &input)
{
    GVariantBuilder *bld = Builder::FromVector(input);
    GVariant *ret = g_variant_builder_end(bld);
    g_variant_builder_unref(bld);

    return ret;
}

} // namespace Value

} // namespace glib2
