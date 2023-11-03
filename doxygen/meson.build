#  GDBus++ - glib2 GDBus C++ wrapper
#
#  SPDX-License-Identifier: AGPL-3.0-only
#
#  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
#  Copyright (C)  David Sommerseth <davids@openvpn.net>

cfgdata = configuration_data()
cfgdata.set('doxygen_builddir', meson.current_build_dir())
cfgdata.set('top_srcdir', meson.project_source_root())

doxygen_conf = configure_file(
  input : 'gdbuspp.doxy.in',
  output : 'gdbuspp.doxy',
  configuration : cfgdata,
)

run_target('doxygen', command : ['doxygen', doxygen_conf])
