//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file signals/signal.hpp
 *
 * @brief  Declaration of the DBus::Signals::Signal interface
 *         This is used to create a generic signal class which to easily
 *         generate the data needed to send the signals.  This is typically
 *         used via Signals::Group::CreateSignal<>()
 */

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <glib.h>

#include "emit.hpp"
#include "group.hpp"


namespace DBus::Signals {

/**
 *  Base class for setting up a D-Bus Signal object.  Before a signal
 *  can be sent, the implementation must first call the SetArgument()
 *  method with the prepared SignalArgList declaring the signal.
 *
 *  See tests/signal-group.cpp and the DebugSignal class for an
 *  implementation example
 */
class Signal
{
  public:
    template <typename C, typename... T>
    static std::shared_ptr<C> Create(Emit::Ptr emitter, T &&...args)
    {
        return std::shared_ptr<C>(new C(emitter, std::forward<T>(args)...));
    }

    virtual ~Signal() noexcept = default;

    /**
     *  Retrieve the assigned signal name
     *
     * @return const std::string&
     */
    const std::string &GetName() const;

    /**
     *  Retrieve the assigned SignalArgList describing the signal.
     *
     *  This is typically used by Signal::Group during the registration
     *  of the signal, so generate the introspection data and set up the
     *  verification of the signal data when sending a signal
     *
     * @return const SignalArgList&
     */
    const SignalArgList &GetArguments() const;


  protected:
    Signal(Emit::Ptr em, const std::string &sig_name);

    /**
     *  Setup the signal arguments, describing the signal.
     *  This method may only be used internally and may only be used once.
     *
     * @param sigargs  SignalArgList with the signal description to use
     */
    void SetArguments(const SignalArgList &sigargs);

    /**
     *  Retrieve the D-Bus data type of the signal.  This is generated
     *  based on the SignalArgList set via the SetArguments() method.
     *
     * @return const char*
     */
    const char *GetDBusType() const;

    /**
     *  Send (emit) the signal
     *
     *  The data passed to this method must have the same D-Bus data type
     *  as generated from the SignalArgList, otherwise it will throw an
     *  exception.
     *
     * @param params   GVariant value container with all the signal data as
     *                 a wrapped tuple.
     * @return true if sending was successful, otherwise false.
     * @throws Signals::Exception on programming errors (incorrect data type
     *         or missing SetArguments() call).
     */
    const bool EmitSignal(GVariant *params) const;


  private:
    /**
     *  Signal::Emit object which will perform the sending of the signal.
     *  This object will also have the complete target list where to send
     *  the signal
     */
    Emit::Ptr emitter = nullptr;

    ///! Signal name used when emitting the signal
    const std::string signal_name;

    ///! Signal "content description", describing the data this signal will emit
    SignalArgList signal_arguments{};

    /**
     *  A generated D-Bus data type of the signal data, generated from the
     *  data in signal_arguments.
     */
    std::string dbustype{};
};
} // namespace DBus::Signals
