/** @file sysfs_base.h */

// Copyright (C) 2020-2025 Sanford Rockowitz <rockowitz@minsoft.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SYSFS_BASE_H_
#define SYSFS_BASE_H_

#include <glib-2.0/glib.h>
#include <stdbool.h>
#include <stdint.h>

#include "base/displays.h"
#include "base/i2c_bus_base.h"

extern bool force_sysfs_unreliable;
extern bool force_sysfs_reliable;
extern bool enable_write_detect_to_status;

// predicate functions
// typedef Dir_Filter_Func
bool        is_drm_connector(const char * dirname, const char * simple_fn);
bool        fn_equal(const char * filename, const char * val);
bool        fn_starts_with(const char * filename, const char * val);
bool        is_n_nnnn(const char * dirname, const char * simple_fn);

GPtrArray * get_sys_video_devices();
void        dbgrpt_sysfs_basic_connector_attributes(int depth);
char *      get_sys_drm_connector_name_by_connector_id(int connector_id);
char *      get_sys_drm_connector_name_by_busno(int busno);
bool        all_sys_drm_connectors_have_connector_id_direct();

char *      get_driver_for_adapter(char * adapter_path, int depth);
// char *      find_adapter(char * path, int depth); // MOVED
char *      find_adapter_and_get_driver(char * path, int depth);
char *      get_driver_for_busno(int busno);


void possibly_write_detect_to_status(const char * driver, const char * connector);
void possibly_write_detect_to_status_by_connector_name(const char * connector);
void possibly_write_detect_to_status_by_businfo(I2C_Bus_Info * businfo);
void possibly_write_detect_to_status_by_dref(Display_Ref * dref);
void possibly_write_detect_to_status_by_connector_path(const char * path);


typedef struct {
   int    i2c_busno;
   int    base_busno;
   int    connector_id;
   char * name;
} Connector_Bus_Numbers;

void        dbgrpt_connector_bus_numbers(Connector_Bus_Numbers * cbn, int depth);
void        free_connector_bus_numbers(Connector_Bus_Numbers * cbn);
void        get_connector_bus_numbers(
               const char *            dirname,    // <device>/drm/cardN
               const char *            fn,         // card0-HDMI-1 etc
               Connector_Bus_Numbers * cbn);

typedef struct {
   GPtrArray *  all_connectors;
   GPtrArray *  connectors_having_edid;
} Sysfs_Connector_Names;

Sysfs_Connector_Names
            get_sysfs_drm_connector_names();
bool        sysfs_connector_names_equal(Sysfs_Connector_Names cn1, Sysfs_Connector_Names cn2);
void        free_sysfs_connector_names_contents(Sysfs_Connector_Names names_struct);
void        dbgrpt_sysfs_connector_names(Sysfs_Connector_Names connector_names, int depth);
Sysfs_Connector_Names
            copy_sysfs_connector_names_struct(Sysfs_Connector_Names original);
char *      find_sysfs_drm_connector_name_by_edid(GPtrArray* connector_names, Byte * edid);

bool        is_sysfs_reliable_for_driver(const char * driver);
bool        is_sysfs_reliable_for_busno(int busno);
bool        is_sysfs_reliable();


// moved from sysfs_i2c_util.h:

char *
sysfs_find_adapter(char * path);

char *
get_i2c_sysfs_driver_by_busno(
      int busno);

char *
get_i2c_sysfs_driver_by_device_name(
      char * device_name);

char *
get_i2c_sysfs_driver_by_fd(
      int fd);

uint32_t
get_i2c_device_sysfs_class(
      int busno);

char *
get_i2c_device_sysfs_name(
      int busno);

bool
sysfs_is_ignorable_i2c_device(
      int busno);


void init_i2c_sysfs_base();


#ifdef FOR_FUTURE_USE
typedef struct {
   char * connector;
   int    busno;
   Display_Ref * dref;    // currently
}   Connector_Busno_Dref;
extern GPtrArray * cbd_table;
typedef GPtrArray Connector_Busno_Dref_Table;

Connector_Busno_Dref_Table * create_connector_busnfo_dref_table();
Connector_Busno_Dref * new_cbd0(int busno);
Connector_Busno_Dref * new_cbd(const char * connector, int busno);
Connector_Busno_Dref * get_cbd_by_connector(const char * connector);
Connector_Busno_Dref * get_cbd_by_busno(int busno);
// if dref != NULL, replaces, if NULL, just erases
void                   set_cbd_connector(Connector_Busno_Dref * cbd, Display_Ref * dref);
void dbgrpt_cbd_table(Connector_Busno_Dref_Table * cbd_table, int depth);
#endif

#endif /* SYSFS_BASE_H_ */
