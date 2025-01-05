/** @file i2c_bus_base.h
 *
 */

// Copyright (C) 2014-2024 Sanford Rockowitz <rockowitz@minsoft.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef I2C_BUS_BASE_H_
#define I2C_BUS_BASE_H_

#include <stdbool.h>
#include <glib-2.0/glib.h>

#include "util/data_structures.h"
#include "util/edid.h"
#include "util/error_info.h"

#include "base/display_lock.h"

extern bool primitive_sysfs;

// Retrieve and inspect bus information

// Keep in sync with i2c_bus_flags_table
#define I2C_BUS_EXISTS                   0x0001

// #define I2C_BUS_VALID_NAME_CHECKED    0x----
// #define I2C_BUS_HAS_VALID_NAME        0x----
// #define I2C_BUS_DRM_CONNECTOR_CHECKED 0x----

#define I2C_BUS_LVDS_OR_EDP              0x0002
#define I2C_BUS_APPARENT_LAPTOP          0x0004
// #define I2C_BUS_EDP                   0x----    ///< bus associated with eDP display
// #define I2C_BUS_LVDS                  0x----    ///< bus associated with LVDS display
// #define I2C_BUS_LAPTOP                (I2C_BUS_EDP|I2C_BUS_LVDS) ///< bus associated with laptop display
#define I2C_BUS_LAPTOP                   (I2C_BUS_LVDS_OR_EDP | I2C_BUS_APPARENT_LAPTOP)
#define I2C_BUS_DISPLAYLINK              0x0008
#ifdef OLD
#define I2C_BUS_SYSFS_UNRELIABLE         0x----
#endif
#define I2C_BUS_SYSFS_KNOWN_RELIABLE     0x0010
#define I2C_BUS_INITIAL_CHECK_DONE       0x0020
#define I2C_BUS_DDC_DISABLED             0x0040

// Flags that can change when monitor connected/disconnected
// #define I2C_BUS_ADDR_0X50             0x----    ///< detected I2C bus address 0x50, may or may not have valid EDID
#define I2C_BUS_SYSFS_EDID               0x0100    ///< EDID was read from /sys
#define I2C_BUS_X50_EDID                 0x0200    ///< EDID was read using I2C
#define I2C_BUS_HAS_EDID                 (I2C_BUS_SYSFS_EDID | I2C_BUS_X50_EDID)
#define I2C_BUS_ADDR_X37                 0x0400    ///< detected I2C bus address 0x37
#define I2C_BUS_ADDR_X30                 0x0800    ///< detected write-only addr to specify EDID block number
#define I2C_BUS_ACCESSIBLE               0x1000    ///< user could change permissions

#define I2C_BUS_DDC_CHECKS_IGNORABLE     0x2000

// affected by display connection/disconnection?
#define I2C_BUS_PROBED                   0x8000    ///< has bus been checked?


typedef enum {
   DRM_CONNECTOR_NOT_CHECKED    = 0,    // ??? needed?
   DRM_CONNECTOR_NOT_FOUND      = 1,
   DRM_CONNECTOR_FOUND_BY_BUSNO = 2,
   DRM_CONNECTOR_FOUND_BY_EDID  = 3
} Drm_Connector_Found_By;

const char * drm_connector_found_by_name(Drm_Connector_Found_By found_by);

#define I2C_BUS_INFO_MARKER "BINF"
/** Information about one I2C bus */
typedef
struct {
   char             marker[4];          ///< always "BINF"
   int              busno;              ///< I2C device number, i.e. N for /dev/i2c-N
   unsigned long    functionality;      ///< i2c bus functionality flags
   Parsed_Edid *    edid;               ///< parsed EDID, if slave address x50 active
#ifdef ALT_LOCK_REC
   Display_Lock_Record * lock_record;   ///<
#endif
   uint32_t         flags;              ///< I2C_BUS_* flags
   char *           driver;             ///< driver name
   int              open_errno;         ///< errno if open fails (!I2C_BUS_ACCESSIBLE)
   char *           drm_connector_name; ///< from /sys
   Drm_Connector_Found_By
                    drm_connector_found_by;
   int              drm_connector_id;
   bool             last_checked_dpms_asleep;
} I2C_Bus_Info;

char *           i2c_interpret_bus_flags(uint16_t flags);
char *           i2c_interpret_bus_flags_t(uint16_t flags);

// Accessors
char *           i2c_get_drm_connector_name(I2C_Bus_Info * bus_info);
char *           i2c_get_drm_connector_attribute(const I2C_Bus_Info * businfo, const char * attribute);
#define I2C_GET_DRM_DPMS(_businfo)    i2c_get_drm_connector_attribute(_businfo, "dpms")
#define I2C_GET_DRM_STATUS(_businfo)  i2c_get_drm_connector_attribute(_businfo, "status")
#define I2C_GET_DRM_ENABLED(_businfo) i2c_get_drm_connector_attribute(_businfo, "enabled")

// Lifecycle
I2C_Bus_Info *   i2c_new_bus_info(int busno);
void             i2c_free_bus_info(I2C_Bus_Info * businfo);
void             i2c_reset_bus_info(I2C_Bus_Info * bus_info);

// Generalized Bus_Info retrieval
I2C_Bus_Info *   i2c_find_bus_info_in_gptrarray_by_busno(GPtrArray * buses, int busno);
int              i2c_find_bus_info_index_in_gptrarray_by_busno(GPtrArray * buses, int busno);

// Reports
void             i2c_dbgrpt_bus_info(I2C_Bus_Info * businfo, bool include_sysinfo, int depth);

// Detected Buses
extern GPtrArray * all_i2c_buses;

I2C_Bus_Info *   i2c_add_bus(int busno);
// void             i2c_add_bus_info(I2C_Bus_Info * businfo);
I2C_Bus_Info    * i2c_get_bus_info(int busno, bool* new_info);
void             i2c_remove_bus_by_businfo(I2C_Bus_Info * businfo);
void             i2c_discard_buses0(GPtrArray* buses);
void             i2c_discard_buses();
void             i2c_remove_bus_by_busno(int busno);

I2C_Bus_Info *   i2c_get_bus_info_by_index(guint busndx);
I2C_Bus_Info *   i2c_find_bus_info_by_busno(int busno);
int              i2c_dbgrpt_buses(bool report_all, bool include_sysfs_info, int depth);  // Reports all detected i2c buses
void             i2c_dbgrpt_buses_summary(int depth);

// Basic I2C bus operations
bool             i2c_device_exists(int busno); // Simple bus detection, no side effects
int              i2c_device_count();           // simple /dev/i2c-n count, no side effects
Error_Info *     i2c_check_device_access(char * dev_name);

// Record display X37 status for reconnection
typedef enum {
   X37_Not_Recorded = 0,
   X37_Not_Detected = 1,
   X37_Detected = 2
} X37_Detection_State;

extern bool use_x37_detection_table;

const char *         x37_detection_state_name(X37_Detection_State state);
void                 i2c_record_x37_detected(int busno, Byte * edidbytes, X37_Detection_State deteted);
X37_Detection_State  i2c_query_x37_detected(int busno, Byte * edidbytes);

#define DW_SLEEP_MILLIS(_millis, _msg) \
   do { \
      dw_sleep_millis(__func__, __LINE__, __FILE__, _millis, _msg); \
   } while(0)
void dw_sleep_millis(const char * func, int line, const char * file, uint millis, const char * msg);

// Initialization and termination
void init_i2c_bus_base();
void terminate_i2c_bus_base();

#endif /* I2C_BUS_BASE_H_ */
