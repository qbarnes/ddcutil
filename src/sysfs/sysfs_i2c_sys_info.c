/** @file sysfs_i2c_sys_info.c */

// Copyright (C) 2020-2025 Sanford Rockowitz <rockowitz@minsoft.com>
// SPDX-License-Identifier: GPL-2.0-or-later
 
#include "config.h"

/** \cond */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <glib-2.0/glib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
/** \endcond */


#include "util/data_structures.h"
#include "util/debug_util.h"
#include "util/drm_common.h"
#include "util/edid.h"
#include "util/file_util.h"
#include "util/glib_string_util.h"
#include "util/i2c_util.h"
#include "util/report_util.h"
#include "util/string_util.h"
#include "util/subprocess_util.h"
#include "util/sysfs_filter_functions.h"
#include "util/sysfs_i2c_util.h"
#include "util/sysfs_util.h"
#include "util/utilrpt.h"

#include "public/ddcutil_types.h"

#include "base/core.h"
#include "base/i2c_bus_base.h"
#include "base/rtti.h"

#include "sysfs_base.h"

#include "sysfs_i2c_sys_info.h"

// Trace class for this file
static DDCA_Trace_Group TRACE_GROUP = DDCA_TRC_SYSFS;

//
// *** I2C_Sys_Info ***
//
// Detailed exploratory scan of sysfs
// Called from query_sysenv_sysfs.c
//

void free_i2c_sys_info(I2C_Sys_Info * info) {
   if (info) {
      free(info->pci_device_path);
      free(info->drm_connector_path);
      free(info->connector);
      free(info->linked_ddc_filename);
      free(info->device_name);
      free(info->drm_dp_aux_name);
      free(info->drm_dp_aux_dev);
      free(info->i2c_dev_name);
      free(info->i2c_dev_dev);
      free(info->driver);
      free(info->ddc_path);
      free(info->ddc_name);
      free(info->ddc_i2c_dev_name);
      free(info->ddc_i2c_dev_dev);
      free(info);
   }
}


//  same whether displayport, non-displayport video, non-video
//    /sys/bus/i2c/devices/i2c-N
//    /sys/devices/pci0000:00/0000:00:02.0/0000:01:00.0/drm/card0/card0-DP-1/i2c-N

// static
void
read_i2cN_device_node(
      const char *   device_path,
      I2C_Sys_Info * info,
      int            depth)
{
   assert(device_path);
   assert(info);
   bool debug = false;
   DBGTRC_STARTING(debug, TRACE_GROUP, "device_path=%s", device_path);
   int d0 = (depth < 0 && IS_DBGTRC(debug, TRACE_GROUP)) ? 2 : depth;
   assert(device_path);

   char * i2c_N = g_path_get_basename(device_path);
   RPT_ATTR_TEXT( d0, &info->device_name,    device_path, "name");
   RPT_ATTR_TEXT( d0, &info->i2c_dev_dev,    device_path, "i2c-dev", i2c_N, "dev");
   RPT_ATTR_TEXT( d0, &info->i2c_dev_name,   device_path, "i2c-dev", i2c_N, "name");
   free(i2c_N);

   DBGTRC_DONE(debug, TRACE_GROUP, "");
}

#ifdef IN_PROGRESS
static void
read_drm_card_connector_node_common(
      const char *   dirname,
      const char *   connector;      // e.g. card0-DP-1
      void *         accumulator,
      int            depth)
{
   bool debug = false;
   DBGMSF(debug, "connector_path=%s", connector_path);
   int d0 = depth;
   if (debug && d0 < 0)
      d0 = 2;
   I2C_Sys_Info * info = accumulator;
   char connector_path[PATH_MAX];
   g_snprintf(connector_path, PATH_MAX, "%s/%s", dirname, connector);

   char * drm_dp_aux_dir;
   RPT_ATTR_SINGLE_SUBDIR(d0, &drm_dp_aux_dir, str_starts_with, "drm_dp_aux", connector_path);
   if (drm_dp_aux_dir) {
      RPT_ATTR_TEXT(d0, &info->drm_dp_aux_name, connector_path, drm_dp_aux_dir, "name");
      RPT_ATTR_TEXT(d0, &info->drm_dp_aux_dev,  connector_path, drm_dp_aux_dir, "dev");
   }

   char * ddc_path_fn;
   RPT_ATTR_REALPATH(d0, &ddc_path_fn, connector_path, "ddc");
   if (ddc_path_fn) {
      info->ddc_path = ddc_path_fn;
      info->linked_ddc_filename = g_path_get_basename(ddc_path_fn);
      info->connector = g_path_get_basename(connector_path);  // == coonector
      RPT_ATTR_TEXT(d0, &info->ddc_name,         ddc_path_fn, "name");
      RPT_ATTR_TEXT(d0, &info->ddc_i2c_dev_name, ddc_path_fn, "i2c-dev", info->linked_ddc_filename, "name");
      RPT_ATTR_TEXT(d0, &info->ddc_i2c_dev_dev,  ddc_path_fn, "i2c-dev", info->linked_ddc_filename, "dev");
   }


   RPT_ATTR_EDID(d1, NULL, dirname, connector, "edid");
   RPT_ATTR_TEXT(d1, NULL, dirname, connector, "enabled");
   RPT_ATTR_TEXT(d1, NULL, dirname, connector, "status");
}
#endif


// Process <controller>/drm/cardN/cardN-<connector> for case that
// cardN-<connector> is a DisplayPort connector

// static
void
read_drm_dp_card_connector_node(
      const char *   connector_path,
      I2C_Sys_Info * info,
      int            depth)
{
   bool debug = false;
   DBGTRC_STARTING(debug, TRACE_GROUP, "connector_path=%s", connector_path);
   int d0 = (depth < 0 && IS_DBGTRC(debug, TRACE_GROUP)) ? 2 : depth;
   assert(connector_path);

   char * ddc_path_fn;
   RPT_ATTR_REALPATH(d0, &ddc_path_fn, connector_path, "ddc");
   if (ddc_path_fn) {
      info->ddc_path = ddc_path_fn;
      info->linked_ddc_filename = g_path_get_basename(ddc_path_fn);
      info->connector = g_path_get_basename(connector_path);
      RPT_ATTR_TEXT(d0, &info->ddc_name,         ddc_path_fn, "name");
      RPT_ATTR_TEXT(d0, &info->ddc_i2c_dev_name, ddc_path_fn, "i2c-dev", info->linked_ddc_filename, "name");
      RPT_ATTR_TEXT(d0, &info->ddc_i2c_dev_dev,  ddc_path_fn, "i2c-dev", info->linked_ddc_filename, "dev");
   }

   char * drm_dp_aux_dir;
   RPT_ATTR_SINGLE_SUBDIR(d0, &drm_dp_aux_dir, str_starts_with, "drm_dp_aux", connector_path);
   if (drm_dp_aux_dir) {
      RPT_ATTR_TEXT(d0, &info->drm_dp_aux_name, connector_path, drm_dp_aux_dir, "name");
      RPT_ATTR_TEXT(d0, &info->drm_dp_aux_dev,  connector_path, drm_dp_aux_dir, "dev");
      free(drm_dp_aux_dir);
   }

   possibly_write_detect_to_status_by_connector_path(connector_path);
   RPT_ATTR_EDID(d0, NULL, connector_path, "edid");
   RPT_ATTR_TEXT(d0, NULL, connector_path, "enabled");
   RPT_ATTR_TEXT(d0, NULL, connector_path, "status");

   DBGTRC_DONE(debug, TRACE_GROUP, "");
}


// Process a <controller>/drm/cardN/cardN-<connector> for case when
// cardN-<connector> is not a DisplayPort connector

// static
void
read_drm_nondp_card_connector_node(
      const char * dirname,                // e.g /sys/devices/pci.../card0
      const char * connector,              // e.g card0-DP-1
      void *       accumulator,
      int          depth)
{
   bool debug = false;
   DBGTRC_STARTING(debug, TRACE_GROUP, "dirname=%s, connector=%s", dirname, connector);
   int d1 = (depth < 0) ? -1 : depth + 1;
   if (debug && d1 < 0)
      d1 = 2;
   I2C_Sys_Info * info = accumulator;

   if (info->connector) {  // already handled by read_drm_dp_card_connector_node()
      DBGTRC_DONE(debug, TRACE_GROUP, "Connector already found, skipping");
      return;
   }

   bool is_dp = RPT_ATTR_SINGLE_SUBDIR(depth, NULL, str_starts_with, "drm_dp_aux", dirname, connector);
   if (is_dp) {
      DBGTRC_DONE(debug, TRACE_GROUP, "Is display port connector, skipping");
      return;
   }

   char i2cN[20];
   g_snprintf(i2cN, 20, "i2c-%d", info->busno);
   bool found_i2c = RPT_ATTR_SINGLE_SUBDIR(depth, NULL, streq, i2cN, dirname, connector, "ddc/i2c-dev");
   if (found_i2c) {
      info->connector = g_strdup(connector);
      possibly_write_detect_to_status_by_connector_name(connector);
      RPT_ATTR_TEXT(d1, NULL, dirname, connector, "ddc", "name");
      RPT_ATTR_TEXT(d1, NULL, dirname, connector, "ddc/i2c-dev", i2cN, "dev");
      RPT_ATTR_TEXT(d1, NULL, dirname, connector, "ddc/i2c-dev", i2cN, "name");
      RPT_ATTR_EDID(d1, NULL, dirname, connector, "edid");
      RPT_ATTR_TEXT(d1, NULL, dirname, connector, "enabled");
      RPT_ATTR_TEXT(d1, NULL, dirname, connector, "status");
   }
   DBGTRC_DONE(debug, TRACE_GROUP, "");
   return;
}


// Dir_Foreach_Func
// Process a <controller>/drm/cardN node

// static
void
one_drm_card(
      const char * dirname,     // e.g /sys/devices/pci
      const char * fn,          // card0, card1 ...
      void *       info,
      int          depth)
{
   bool debug = false;
   DBGTRC_STARTING(debug, TRACE_GROUP, "dirname=%s, fn=%s", dirname, fn);

   char buf[PATH_MAX];
   g_snprintf(buf, PATH_MAX, "%s/%s", dirname, fn);
   dir_ordered_foreach(
         buf,
         predicate_cardN_connector,
         gaux_ptr_scomp,
         read_drm_nondp_card_connector_node,
         info,
         depth);

   DBGTRC_DONE(debug, TRACE_GROUP, "");
}


static void
read_controller_driver(
      const char *   controller_path,
      I2C_Sys_Info * info,
      int            depth)
{
   char * driver_path = NULL;
   RPT_ATTR_REALPATH(depth, &driver_path, controller_path, "driver");
   if (driver_path) {
      info->driver = g_path_get_basename(driver_path);
      free(driver_path);
   }
}



// called only if not DisplayPort

// static
void
read_pci_display_controller_node(
      const char *   nodepath,
      int            busno,
      I2C_Sys_Info * info,
      int            depth)
{
   bool debug = false;
   DBGTRC_STARTING(debug, TRACE_GROUP, "busno=%d, nodepath=%s", busno, nodepath);
   int d0 = depth;                              // for this function
   if (debug && d0 < 0)
      d0 = 2;
   int depth1 = (depth < 0) ? -1 : depth + 1;   // for called functions

   char * class;
   RPT_ATTR_TEXT(d0, &class, nodepath, "class");
   if (class && str_starts_with(class, "0x03")) {
      // this is indeed a display controller node
      RPT_ATTR_TEXT(d0, NULL, nodepath, "boot_vga");
      RPT_ATTR_TEXT(d0, NULL, nodepath, "vendor");
      RPT_ATTR_TEXT(d0, NULL, nodepath, "device");

      // RPT_ATTR_TEXT(d0, NULL, nodepath, "fw_version");
#ifdef OLD
      char * driver_path = NULL;
      RPT_ATTR_REALPATH(d0, &driver_path, nodepath, "driver");
      if (driver_path && info->connector)   // why the info->connector test?
         info->driver = g_path_get_basename(driver_path);
      free(driver_path);
#endif
      read_controller_driver(nodepath, info, depth);

      // examine all drm/cardN subnodes
      char buf[PATH_MAX];
      g_snprintf(buf, PATH_MAX, "%s/%s", nodepath, "drm");
      DBGTRC_NOPREFIX(debug, TRACE_GROUP, "Calling dir_ordered_foreach, buf=%s, predicate predicate_cardN_connector()", buf);
      dir_ordered_foreach(buf, predicate_cardN_connector, i2c_compare, one_drm_card, info, depth1);
   }
   free(class);
   DBGTRC_DONE(debug, TRACE_GROUP, "");
}


I2C_Sys_Info *
get_i2c_sys_info(
      int busno,
      int depth)
{
   bool debug = false;
   DBGTRC_STARTING(debug, TRACE_GROUP, "busno=%d. depth=%d", busno, depth);
   I2C_Sys_Info * result = NULL;
   int d1 = (depth < 0) ? -1 : depth+1;

   char i2c_N[20];
   g_snprintf(i2c_N, 20, "i2c-%d", busno);
                                               // Example:
   char   i2c_device_path[50];                 // /sys/bus/i2c/devices/i2c-13
   char * pci_i2c_device_path = NULL;          // /sys/devices/../card0/card0-DP-1/i2c-13
   char * pci_i2c_device_parent = NULL;        // /sys/devices/.../card0/card0-DP-1
// char * connector_path = NULL;               // .../card0/card0-DP-1
// char * drm_dp_aux_dir = NULL;               // .../card0/card0-DP-1/drm_dp_aux0
// char * ddc_path_fn = NULL;                  // .../card0/card0-DP-1/ddc
   g_snprintf(i2c_device_path, 50, "/sys/bus/i2c/devices/i2c-%d", busno);

   if (directory_exists(i2c_device_path)) {
      result = calloc(1, sizeof(I2C_Sys_Info));
      result->busno = busno;
      // real path is in /sys/devices tree
      RPT_ATTR_REALPATH(d1, &pci_i2c_device_path, i2c_device_path);
      result->pci_device_path = pci_i2c_device_path;
      DBGTRC_NOPREFIX(debug, TRACE_GROUP, "pci_i2c_device_path=%s", pci_i2c_device_path);
      read_i2cN_device_node(pci_i2c_device_path, result, d1);

      RPT_ATTR_REALPATH(d1, &pci_i2c_device_parent, pci_i2c_device_path, "..");
      DBGTRC_NOPREFIX(debug, TRACE_GROUP, "pci_i2c_device_parent=%s", pci_i2c_device_parent);

      bool has_drm_dp_aux_dir =
              RPT_ATTR_SINGLE_SUBDIR(d1, NULL, str_starts_with, "drm_dp_aux", pci_i2c_device_parent);
      // RPT_ATTR_SINGLE_SUBDIR(d1, &drm_dp_aux_dir, str_starts_with, "drm_dp_aux", pci_i2c_device_parent);
      // if (drm_dp_aux_dir) {
      if (has_drm_dp_aux_dir) {
         // pci_i2c_device_parent is a drm connector node
         result->is_amdgpu_display_port = true;
         read_drm_dp_card_connector_node(pci_i2c_device_parent, result, d1);

         char controller_path[PATH_MAX];
         g_snprintf(controller_path, PATH_MAX, "%s/../../..", pci_i2c_device_parent);
         read_controller_driver(controller_path, result, d1);

#ifdef OLD
         char * driver_path = NULL;
         // look in controller node:
         RPT_ATTR_REALPATH(d1, &driver_path, pci_i2c_device_parent, "../../..", "driver");
         result->driver = g_path_get_basename(driver_path);
         free(driver_path);
#endif

         // free(drm_dp_aux_dir);
      }
      else {
         // pci_i2c_device_parent is a display controller node
         read_pci_display_controller_node(pci_i2c_device_parent, busno, result, d1);


#ifdef OLD
         char * driver_path = NULL;
         RPT_ATTR_REALPATH(d1, &driver_path, pci_i2c_device_parent, "driver");
         result->driver = g_path_get_basename(driver_path);
         free(driver_path);
#endif
      }
      free(pci_i2c_device_parent);
   }

   // ASSERT_IFF(drm_dp_aux_dir, ddc_path_fn);
   DBGTRC_DONE(debug, TRACE_GROUP, "Returning: %p", result);
   return result;
}


/** Emit debug type report of a #I2C_Sys_Info struct
 *
 *  @param  info   pointer to struct with relevant /sys information
 *  @param  depth  logical indentation depth, if < 0 perform no indentation
 */
// used in i2c_bus_base.c, use eliminated 9/26/2024
void dbgrpt_i2c_sys_info(I2C_Sys_Info * info, int depth) {
   bool debug = false;
   DBGTRC_STARTING(debug, DDCA_TRC_NONE, "info=%p, depth=%d", info, depth);
   int d1 = (depth < 0) ? 0 : depth + 1;
   int d2 = (depth < 0) ? 0 : depth + 2;
   if (depth < 0)
      depth = 0;

   if (info) {
      rpt_vstring(depth, "Extended information for /sys/bus/i2c/devices/i2c-%d...", info->busno);
      char * busno_pad = (info->busno < 10) ? " " : "";
      rpt_vstring(d1, "PCI device path:     %s", info->pci_device_path);
      rpt_vstring(d1, "name:                %s", info->device_name);
      rpt_vstring(d1, "i2c-dev/i2c-%d/dev: %s %s",
                      info->busno, busno_pad, info->i2c_dev_dev);
      rpt_vstring(d1, "i2c-dev/i2c-%d/name:%s %s",
                      info->busno, busno_pad, info->i2c_dev_name);
      rpt_vstring(d1, "Connector:           %s", info->connector);
      rpt_vstring(d1, "Driver:              %s", info->driver);

      if (info->is_amdgpu_display_port) {
         rpt_vstring(d1, "DisplayPort only attributes:");
         rpt_vstring(d2, "ddc path:                %s", info->ddc_path);
      // rpt_vstring(d2, "Linked ddc filename:     %s", dp_info->linked_ddc_filename);
         rpt_vstring(d2, "ddc name:                %s", info->ddc_name);
         rpt_vstring(d2, "ddc i2c-dev/%s/dev:  %s %s",
                         info->linked_ddc_filename, busno_pad, info->ddc_i2c_dev_dev);
         rpt_vstring(d2, "ddc i2c-dev/%s/name: %s %s",
                         info->linked_ddc_filename, busno_pad, info->ddc_i2c_dev_name);
         rpt_vstring(d2, "DP Aux channel dev:      %s", info->drm_dp_aux_dev);
         rpt_vstring(d2, "DP Aux channel name:     %s", info->drm_dp_aux_name);
      }
      // else {
      //    rpt_vstring(d1, "Not a DisplayPort connection");
      // }
   }

   DBGTRC_DONE(debug, DDCA_TRC_NONE, "");
}


static void report_one_bus_i2c(
      const char * dirname,     //
      const char * fn,          // i2c-1, i2c-2, etc., possibly 1-0037, 1-0023, 1-0050 etc
      void *       data,
      int          depth)
{
   bool debug = false;
   DBGMSF(debug, "dirname=%s, fn=%s", dirname, fn);
   rpt_nl();
   int busno = i2c_name_to_busno(fn);  //   catches non-i2cN names
   if (busno < 0) {
      rpt_vstring(depth, "Ignoring %s/%s", dirname, fn);
   }
   else {
      rpt_vstring(depth, "Examining /sys/bus/i2c/devices/i2c-%d...", busno);
      int d1 = depth+1;
      // reports as it collects, no need to call report_i2c_sys_info()
      I2C_Sys_Info * info = get_i2c_sys_info(busno, d1);
      // report_i2c_sys_info(info, depth+1);
      free_i2c_sys_info(info);
   }
}

// used in query_sysenv_sysfs.c
void dbgrpt_sys_bus_i2c(int depth) {
   bool debug = FALSE;
   DBGTRC_STARTING(debug, DDCA_TRC_NONE, "");

   rpt_label(depth, "Examining /sys/bus/i2c/devices:");
   dir_ordered_foreach("/sys/bus/i2c/devices", NULL, i2c_compare, report_one_bus_i2c, NULL, depth);

   DBGTRC_DONE(debug, DDCA_TRC_NONE, "");
}


void init_i2c_sysfs_i2c_sys_info() {
   // I2C_Sys_Info
   RTTI_ADD_FUNC(read_i2cN_device_node);
   RTTI_ADD_FUNC(read_drm_dp_card_connector_node);
   RTTI_ADD_FUNC(read_drm_nondp_card_connector_node);
   RTTI_ADD_FUNC(one_drm_card);
   RTTI_ADD_FUNC(read_pci_display_controller_node);
   RTTI_ADD_FUNC(get_i2c_sys_info);
   RTTI_ADD_FUNC(dbgrpt_i2c_sys_info);
   RTTI_ADD_FUNC(dbgrpt_sys_bus_i2c);
}

// *** End of I2C_Sys_Info
