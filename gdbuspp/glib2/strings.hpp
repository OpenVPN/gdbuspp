//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

#pragma once


/**
 *  Very simple macro to convert a C++ std::string to a C char * string
 *  which will be nullptr if the std::string is empty.
 *
 *  This is used in some of the glib2 function calls, where empty
 *  strings is treated differently than nullptr values.
 */
#define str2gchar(x) (x.empty() ? nullptr : (gchar *)x.c_str())
