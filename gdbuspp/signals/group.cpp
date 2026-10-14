//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file signals/group.cpp
 *
 * @brief  Implementation of the DBus::Signals::Group
 */

#include <string>
#include <glib.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include "../glib2/utils.hpp"
#include "../object/path.hpp"
#include "exceptions.hpp"
#include "group.hpp"


namespace DBus {
namespace Signals {

std::string SignalArgSignature(const SignalArgList &list)
{
    std::string typestr = "(";
    for (const auto &e : list)
    {
        typestr += e.type;
    }
    typestr += ")";
    return typestr;
}


void Group::RegisterSignal(const std::string &signal_name, const SignalArgList &signal_type)
{
    try
    {
        (void)registered_signals.at(signal_name);
        throw Signals::Exception(fmt::format(FMT_COMPILE("Group name {:?} is already registered"),
                                             signal_name));
    }
    catch (const std::out_of_range &)
    {
        registered_signals[signal_name] = signal_type;
    };
};


std::string Group::GenerateIntrospection()
{
    std::string ret;

    for (const auto &[sig_name, sig_type] : registered_signals)
    {
        std::string type_elem;
        std::string introsp;

        for (const auto &spec : sig_type)
        {
            type_elem.append(spec.type);
            introsp.append(fmt::format(
                FMT_COMPILE("      <arg name=\"{}\" type=\"{}\"/>\n"),
                spec.name,
                spec.type));
        }

        type_cache[sig_name] = fmt::format(FMT_COMPILE("({})"), type_elem);
        ret.append(fmt::format(
            FMT_COMPILE("    <signal name=\"{}\">\n{}    </signal>\n"),
            sig_name,
            introsp));
    }
    return ret;
}


void Group::ModifyPath(const Object::Path &new_path) noexcept
{
    object_path = new_path;
}


void Group::AddTarget(const std::string &busname)
{
    signal_groups.at("__default__")->AddTarget(busname, object_path, object_interface);
};


void Group::GroupCreate(const std::string &groupname)
{
    try
    {
        (void)signal_groups.at(groupname);
        throw Signals::Exception(fmt::format(FMT_COMPILE("Group name {:?}"), groupname));
    }
    catch (const std::out_of_range &)
    {
        // If we receive this exception; this key does not exists
        signal_groups[groupname] = Signals::Emit::Create(connection);
    }
}


void Group::GroupRemove(const std::string &groupname)
{
    if ("__default__" == groupname)
    {
        throw Signals::Exception("Cannot use reserved group name (__default__)");
    }

    const auto it = signal_groups.find(groupname);
    if (signal_groups.end() == it)
    {
        throw Signals::Exception(fmt::format(FMT_COMPILE("Group name {:?} is not created"), groupname));
    }
    signal_groups.erase(it);
}


void Group::GroupAddTarget(const std::string &groupname,
                           const std::string &busname)
{
    get_group_emitter(groupname)->AddTarget(busname, object_path, object_interface);
}


void Group::GroupAddTargetList(const std::string &groupname,
                               const std::vector<std::string> &list)
{
    for (const std::string &busname : list)
    {
        GroupAddTarget(groupname, busname);
    }
}


void Group::GroupClearTargets(const std::string &groupname)
{
    get_group_emitter(groupname)->ClearTargets();
}


void Group::SendGVariant(const std::string &signal_name,
                         GVariant *param)
{
    GroupSendGVariant("__default__", signal_name, param);
}


void Group::GroupSendGVariant(const std::string &groupname,
                              const std::string &signal_name,
                              GVariant *signal_data)
{
    // If the type cache is empty, run the introspection generation
    // to build the cache
    if (type_cache.empty())
    {
        (void)GenerateIntrospection();
    }

    std::string exp_type{};
    try
    {
        // Retrieve the expected type from the type cache for the signal ...
        exp_type = type_cache.at(signal_name);
    }
    catch (const std::out_of_range &)
    {
        throw Signals::Exception(fmt::format(FMT_COMPILE("Not a registered signal: {}"), signal_name));
    }

    GVariant *param = glib2::Value::TupleWrap(signal_data);

    // ... and validate it with what we have recevied
    std::string param_type(g_variant_get_type_string(param));
    if (exp_type != param_type)
    {
        throw Signals::Exception(fmt::format(
            FMT_COMPILE("Invalid data type for {:?} - Expected {:?} but received {:?}"),
            signal_name,
            exp_type,
            param_type));
    }

    auto emitter = get_group_emitter(groupname, true);
    emitter->SendGVariant(signal_name, param);
    g_variant_unref(param);
}


Group::Group(DBus::Connection::Ptr conn,
             const Object::Path &object_path_,
             const std::string &object_interface_)
    : connection(conn),
      object_path(object_path_), object_interface(object_interface_)
{
    // Create a default target group
    signal_groups["__default__"] = Signals::Emit::Create(connection);
}



Signals::Emit::Ptr Group::get_group_emitter(const std::string &groupname,
                                            const bool internal) const
{
    if (!internal && ("__default__" == groupname))
    {
        throw Signals::Exception("Cannot use reserved group name (__default__)");
    }
    try
    {
        return signal_groups.at(groupname);
    }
    catch (const std::out_of_range &)
    {
        throw Signals::Exception(fmt::format(FMT_COMPILE("Group name {:?} is not created"), groupname));
    }
}



} // namespace Signals
} // namespace DBus
