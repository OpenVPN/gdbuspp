//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C) 2026-  Karol Adrianek <karol.adrianek@openvpn.com>
//  Copyright (C) 2026-  OpenVPN Inc <sales@openvpn.net>
//

#pragma once

#include <memory>
#include <glib.h>

/**
 *  @file data.hpp
 *
 *  Functionality related to making the GVariant type
 *  to behave more C++ like
 */

namespace DBus::Data {

/**
 *  GVariant object wrapped in a std::unique_ptr which will
 *  automatically be freed (g_variant_unref()) when no longer
 *  needed.
 */
using UPtr = std::unique_ptr<GVariant, decltype(&g_variant_unref)>;

/**
 *  Takes ownership of the D-Bus method call parameters, ensuring they
 *  are released on all code paths - including when the call fails before
 *  the parameters are passed on to glib2.  A floating reference is
 *  consumed; a non-floating reference remains owned by the caller.
 *
 * @param params  GVariant * with the method call parameters, may be nullptr
 * @return std::unique_ptr holding a strong reference to the parameters
 */
UPtr MakeUPtr(GVariant *params);

} // namespace DBus::Data
