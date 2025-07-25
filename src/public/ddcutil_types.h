/** @file ddcutil_types.h
 *  @brief File ddcutil_types.h contains type declarations for the C API.
 *
 *  API function declarations are specified in a separate file, ddcutil_c_api.h.
 *  The reason for this split is that the type declarations are used throughout the
 *  **ddcutil** implementation, whereas the function declarations are only referenced
 *  within the code that implements the API.
 */

// Copyright (C) 2014-2025 Sanford Rockowitz <rockowitz@minsoft.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DDCUTIL_TYPES_H_
#define DDCUTIL_TYPES_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** @cond */
#include <stdbool.h>
#include <stdint.h>                // for uint8_t, unit16_t
/** @endcond */


//
// Status Code
//

/**
 * **ddcutil** Status Code
 *
 *  Most public **ddcutil** functions return a status code.
 *  These status codes have 3 sources:
 *  - Linux
 *  - ADL (AMD Display Library) (no longer used)
 *  - **ddcutil** itself
 *
 *  These multiple status code sources are consolidated by "modulating"
 *  the raw values into non-overlapping ranges.
 *  - Linux errno values are returned as negative numbers (e.g. -EIO)
 *  - ADL values are modulated by 2000 (i.e., 2000 subtracted from negative ADL status codes,
 *         or added to positive ADL status codes) (no longer used)
 *  - ddcutil errors are always in the -3000 range
 *
 *  In summary:
 *  - 0 always indicates a normal successful status
 *  - Positive values (possible with ADL) represent qualified success of some sort (no longer used)
 *  - Negative values indicate an error condition.
 */
typedef int DDCA_Status;


//
// Build Information
//

//! ddcutil version
//!
typedef struct {
   uint8_t    major;          ///< Major release number
   uint8_t    minor;          ///< Minor release number
   uint8_t    micro;          ///< Micro release number
} DDCA_Ddcutil_Version_Spec;

//! Build option flags, as returned by #ddca_build_options()
//! The enum values are defined as 1,2,4 etc so that they can be or'd.
typedef enum {
   DDCA_BUILT_WITH_NONE    = 0x00,
   /** @brief ddcutil was built with support for USB connected monitors */
   DDCA_BUILT_WITH_USB     = 0x02,
  /** @brief ddcutil was built with support for failure simulation */
   DDCA_BUILT_WITH_FAILSIM = 0x04
} DDCA_Build_Option_Flags;


//
// Error Reporting
//

#define DDCA_ERROR_DETAIL_MARKER "EDTL"
//! Detailed error report
typedef struct ddca_error_detail {
   char                       marker[4];         ///< Always "EDTL"
   DDCA_Status                status_code;       ///< Error code
   char *                     detail;            ///< Optional explanation string
   uint16_t                   cause_ct;          ///< Number of sub-errors
   struct ddca_error_detail * causes[];          ///< Variable length array of contributing errors
} DDCA_Error_Detail;


//
// Initialization
//

//! Options passed to #ddca_init()
typedef enum {
   DDCA_INIT_OPTIONS_NONE                  = 0,
   DDCA_INIT_OPTIONS_DISABLE_CONFIG_FILE   = 1,  ///< Do not process configuration file
   DDCA_INIT_OPTIONS_CLIENT_OPENED_SYSLOG  = 2,  ///< Client has already opened syslog
   DDCA_INIT_OPTIONS_ENABLE_INIT_MSGS      = 4   ///< Emit msgs re how config file and passed options merged
} DDCA_Init_Options;


//
// Message Control
//

//! Output Level
//!
//! Values assigned to constants allow them to be or'd in bit flags.
//!
//! Values are ascending in order of verbosity
typedef enum {
   DDCA_OL_TERSE  =0x04,         /**< Brief   output  */
   DDCA_OL_NORMAL =0x08,         /**< Normal  output */
   DDCA_OL_VERBOSE=0x10,         /**< Verbose output */
   DDCA_OL_VV     =0x20          /**< Very verbose output */
} DDCA_Output_Level;


//
// Tracing
//

//! ddcutil message severity maps to syslog() severity.
//!
//! Gaps in values allow for further refinement
typedef enum {
   DDCA_SYSLOG_NOT_SET  =  -1,
   DDCA_SYSLOG_NEVER    =   0,
   DDCA_SYSLOG_ERROR    =   3,
   DDCA_SYSLOG_WARNING  =   6,
   DDCA_SYSLOG_NOTICE   =   9,
   DDCA_SYSLOG_INFO     =  12,
   DDCA_SYSLOG_VERBOSE  =  15,
   DDCA_SYSLOG_DEBUG    =  18,
} DDCA_Syslog_Level;


//
// Performance statistics
//

//! Used as values to specify a single statistics type, and as
//! bitflags to select statistics types.
typedef enum {
   DDCA_STATS_NONE     = 0x00,    ///< no statistics
   DDCA_STATS_TRIES    = 0x01,    ///< retry statistics
   DDCA_STATS_ERRORS   = 0x02,    ///< error statistics
   DDCA_STATS_CALLS    = 0x04,    ///< system calls
   DDCA_STATS_ELAPSED  = 0x08,    ///< total elapsed time
   DDCA_STATS_API      = 0x10,    ///< API specific stats
   DDCA_STATS_ALL      = 0xFF     ///< indicates all statistics types
} DDCA_Stats_Type;


//
// Miscellaneous types
//

typedef double DDCA_Sleep_Multiplier;

//
// Output capture
//

//! Capture option flags, used by #ddca_start_capture()
//!
//! The enum values are defined as 1,2,4 etc so that they can be or'd.
//!
//!  @since 0.9.0
typedef enum {
   DDCA_CAPTURE_NOOPTS     = 0,     ///< @brief no options specified
   DDCA_CAPTURE_STDERR     = 1      ///< @brief capture **stderr** as well as **stdout**
} DDCA_Capture_Option_Flags;


//
//  Display Specification
//

/** @defgroup api_display_spec API Display Specification */

/** Opaque display identifier
 *
 * A **DDCA_Display_Identifier** holds the criteria for selecting a monitor,
 * typically as specified by the user.
 *
 * It can take several forms:
 * - the display number assigned by **dccutil**
 * - an I2C bus number
 * - an ADL (adapter index, display index) pair (deprecated)
 * - a  USB (bus number, device number) pair or USB device number
 * - an EDID
 * - manufacturer, model, and serial number strings
 *
 * @ingroup api_display_spec
 * */
typedef void * DDCA_Display_Identifier;

/** Opaque display reference.
 *
 * A #DDCA_Display_Ref describes a monitor.  It contains 3 kinds of information:
 * - Assigned ddcutil display number
 * - The operating system path to the monitor, which is an I2C bus number or
 *   a USB device number.
 * - Accumulated information about the monitor, such as the EDID or capabilities string.
 *
 * @remark
 * When libddcutil is started, it detects all connected monitors and creates
 * a persistent #DDCA_DisplayRef for each.
 * @remark
 * A #DDCA_Display_Ref can be obtained in 2 ways:
 * - From the DDCA_Display_List returned by #ddca_get_display_info_list2()
 * - Searching based on #DDCA_Display_Identifier using #ddca_get_display_ref()
 *
 * @ingroup api_display_spec
 */
typedef void * DDCA_Display_Ref;


/** Opaque display handle
 *
 * A **DDCA_Display_Handle** represents an "open" display on which actions can be
 * performed. This is required for communicating with a display. It is obtained by
 * calling #ddca_open_display2().
 *
 * For I2C and USB connected displays, an operating system open is performed by
 * # ddca_open_display2().  #DDCA_Display_Handle then contains the file handle
 * returned by the operating system.
 *
 * @ingroup api_display_spec
 */
typedef void * DDCA_Display_Handle;

///@}


//
// VCP Feature Information
//

/** MCCS Version in binary form */
typedef struct {
   uint8_t    major;           /**< major version number */
   uint8_t    minor;           /*** minor version number */
} DDCA_MCCS_Version_Spec;

extern const DDCA_MCCS_Version_Spec DDCA_VSPEC_V10;        ///< MCCS version 1.0
extern const DDCA_MCCS_Version_Spec DDCA_VSPEC_V20;        ///< MCCS version 2.0
extern const DDCA_MCCS_Version_Spec DDCA_VSPEC_V21;        ///< MCCS version 2.1
extern const DDCA_MCCS_Version_Spec DDCA_VSPEC_V30;        ///< MCCS version 3.0
extern const DDCA_MCCS_Version_Spec DDCA_VSPEC_V22;        ///< MCCS version 2.2
extern const DDCA_MCCS_Version_Spec DDCA_VSPEC_ANY;        ///< used as query specifier
extern const DDCA_MCCS_Version_Spec DDCA_VSPEC_UNKNOWN;    ///< value for monitor has been queried unsuccessfully
extern const DDCA_MCCS_Version_Spec DDCA_VSPEC_UNQUERIED;  ///< indicates version not queried


/** MCCS VCP Feature Id */
typedef uint8_t DDCA_Vcp_Feature_Code;


/** Bitfield specifying a collection of VCP feature codes
 *
 *  @remark
 *  This struct might be more appropriately named DDCA_Feature_Set, but
 *  that results in confusing function names such as ddca_feature_set_set()
 */
typedef struct {
   uint8_t bytes[32];
} DDCA_Feature_List;

extern const DDCA_Feature_List DDCA_EMPTY_FEATURE_LIST;

/** Identifiers for publicly useful VCP feature subsets
 *
 * @remark
 * These subset identifiers represent a subset of the much
 * larger collection of subset ids used internally.
 */
typedef enum {
   DDCA_SUBSET_UNSET = 0,      ///< No subset selected
   DDCA_SUBSET_KNOWN,          ///< All features defined in a MCCS spec
   DDCA_SUBSET_COLOR,          ///< Color related features
   DDCA_SUBSET_PROFILE,        ///< Features saved and restored by loadvcp/setvcp
   DDCA_SUBSET_MFG,            ///< Feature codes reserved for manufacturer use (0x0e..0xff)
   DDCA_SUBSET_CAPABILITIES,   ///< Feature codes specified in capabilities string
   DDCA_SUBSET_SCAN,            ///< All feature codes other than known write-only or table
   DDCA_SUBSET_CUSTOM
} DDCA_Feature_Subset_Id;


//
// Display Information
//

/** Indicates how MCCS communication is performed */
typedef enum {
   DDCA_IO_I2C,     /**< Use DDC to communicate with a /dev/i2c-n device */
   DDCA_IO_USB      /**< Use USB reports for a USB connected monitor */
} DDCA_IO_Mode;


/** Describes a display's physical access mode and the location identifiers for that mode  */
typedef struct {
   DDCA_IO_Mode io_mode;        ///< physical access mode
   union {
      int        i2c_busno;     ///< I2C bus number
      int        hiddev_devno;  ///* USB hiddev device  number
   } path;
} DDCA_IO_Path;


// Maximum length of strings extracted from EDID, plus 1 for trailing NULL
#define DDCA_EDID_MFG_ID_FIELD_SIZE 4
#define DDCA_EDID_MODEL_NAME_FIELD_SIZE 14
#define DDCA_EDID_SN_ASCII_FIELD_SIZE 14


#define DDCA_DISPLAY_INFO_MARKER "DDIN"
/** Describes one monitor detected by ddcutil.
 *
 *  This struct is copied to the caller and can simply be freed.
 */
typedef struct {
   char                   marker[4];        ///< always "DDIN"
   int                    dispno;           ///< ddcutil assigned display number
   DDCA_IO_Path           path;             ///< physical access path to display
   int                    usb_bus;          ///< USB bus number, if USB connection
   int                    usb_device;       ///< USB device number, if USB connection
   char                   mfg_id[    DDCA_EDID_MFG_ID_FIELD_SIZE    ]; ///< 3 character mfg id from EDID
   char                   model_name[DDCA_EDID_MODEL_NAME_FIELD_SIZE]; ///< model name from EDID, 13 char max
   char                   sn[        DDCA_EDID_SN_ASCII_FIELD_SIZE  ]; ///< "serial number" from EDID, 13 char max
   uint16_t               product_code;     ///< model product number
   uint8_t                edid_bytes[128];  ///< first 128 bytes of EDID
   DDCA_MCCS_Version_Spec vcp_version;      ///< VCP version as pair of numbers
   DDCA_Display_Ref       dref;             ///< opaque display reference
} DDCA_Display_Info;


#define DDCA_DRM_CONNECTOR_FIELD_SIZE 32

typedef enum {
   DDCA_DRM_CONNECTOR_NOT_FOUND,
   DDCA_DRM_CONNECTOR_FOUND_BY_BUSNO,
   DDCA_DRM_CONNECTOR_FOUND_BY_EDID
} DDCA_Drm_Connector_Found_By;


/** Describes one monitor detected by ddcutil.
 *
 *  This is an extended version of DDCA_Display_Info with additional information
 *  and space allocated to allow for future extensions without breaking
 *  the ABI.
 *
 *  This struct is copied to the caller and can simply be freed.
 */
typedef struct {
   char                   marker[4];        ///< always "DDIN"
   int                    dispno;           ///< ddcutil assigned display number
   DDCA_IO_Path           path;             ///< physical access path to display
   int                    usb_bus;          ///< USB bus number, if USB connection
   int                    usb_device;       ///< USB device number, if USB connection
   char                   mfg_id[    DDCA_EDID_MFG_ID_FIELD_SIZE    ]; ///< 3 character mfg id from EDID
   char                   model_name[DDCA_EDID_MODEL_NAME_FIELD_SIZE]; ///< model name from EDID, 13 char max
   char                   sn[        DDCA_EDID_SN_ASCII_FIELD_SIZE  ]; ///< "serial number" from EDID, 13 char max
   uint16_t               product_code;     ///< model product number
   uint8_t                edid_bytes[128];  ///< first 128 bytes of EDID
   DDCA_MCCS_Version_Spec vcp_version;      ///< VCP version as pair of numbers
   DDCA_Display_Ref       dref;             ///< opaque display reference
   char                   drm_card_connector[DDCA_DRM_CONNECTOR_FIELD_SIZE];
   DDCA_Drm_Connector_Found_By drm_card_connector_found_by;
   int16_t                drm_connector_id;   ///< -1 if undefined in sysfs
   void *                 unused[8];
} DDCA_Display_Info2;


/** Collection of #DDCA_Display_Info */
typedef struct {
   int                ct;       ///< number of records
   DDCA_Display_Info  info[];   ///< array whose size is determined by ct
} DDCA_Display_Info_List;


/** @name Version Feature Flags
 *
 * #DDCA_Version_Feature_Flags is a byte of flags describing attributes of a
 * VCP feature that can vary by MCCS version.
 *
 * @remark
 * Exactly 1 of #DDCA_RO, #DDCA_WO, #DDCA_RW is set.
 * @remark
 * Flags #DDCA_STD_CONT, #DDCA_COMPLEX_CONT, #DDCA_SIMPLE_NC, #DDCA_COMPLEX_NC,
 * #DDCA_WO_NC, #DDCA_NORMAL_TABLE, #DDCA_WO_TABLE refine  the C/NC/TABLE categorization
 * of the VESA MCCS specification.  Exactly 1 of these bits is set.
 */
///@{

/** Flags specifying VCP feature attributes, which can be VCP version dependent. */
typedef uint16_t DDCA_Version_Feature_Flags;

// Bits in DDCA_Version_Feature_Flags:

// Exactly 1 of DDCA_RO, DDCA_WO, DDCA_RW is set
#define DDCA_RO           0x0400               /**< Read only feature */
#define DDCA_WO           0x0200               /**< Write only feature */
#define DDCA_RW           0x0100               /**< Feature is both readable and writable */
#define DDCA_READABLE     (DDCA_RO | DDCA_RW)  /**< Feature is either RW or RO */
#define DDCA_WRITABLE     (DDCA_WO | DDCA_RW)  /**< Feature is either RW or WO */

// Further refine the C/NC/TABLE categorization of the MCCS spec
// Exactly 1 of the following 8 bits is set
#define DDCA_STD_CONT     0x0080       /**< Normal continuous feature */
#define DDCA_COMPLEX_CONT 0x0040       /**< Continuous feature with special interpretation */
#define DDCA_SIMPLE_NC    0x0020       /**< Non-continuous feature, having a defined list of values in byte SL */
#define DDCA_EXTENDED_NC  0x8000       /**< Like DDCA_SIMPLE_NC, but also using byte SH */
#define DDCA_COMPLEX_NC   0x0010       /**< Non-continuous feature, having a complex interpretation using one or more of SL, SH, ML, MH */
#define DDCA_NC_CONT      0x0800       /**< NC feature combining reserved values with continuous range */
// For WO NC features.  There's no interpretation function or lookup table
// Used to mark that the feature is defined for a version
#define DDCA_WO_NC        0x0008       /**< Used internally for write-only non-continuous features */
#define DDCA_NORMAL_TABLE 0x0004       /**< Normal RW table type feature */
#define DDCA_WO_TABLE     0x0002       /**< Write only table feature */

#define DDCA_CONT         (DDCA_STD_CONT|DDCA_COMPLEX_CONT)            /**< Continuous feature, of any subtype */
#define DDCA_NC           (DDCA_EXTENDED_NC |DDCA_SIMPLE_NC|DDCA_COMPLEX_NC|DDCA_WO_NC|DDCA_NC_CONT)  /**< Non-continuous feature of any subtype */
#define DDCA_NON_TABLE    (DDCA_CONT | DDCA_NC)                        /**< Non-table feature of any type */

#define DDCA_TABLE        (DDCA_NORMAL_TABLE | DDCA_WO_TABLE)        /**< Table type feature, of any subtype */
// #define DDCA_KNOWN     (DDCA_CONT | DDCA_NC | DDCA_TABLE)           // *** unused ***

// Additional bits:
#define DDCA_DEPRECATED   0x0001     /**< Feature is deprecated in the specified VCP version */

///@}

typedef uint16_t DDCA_Global_Feature_Flags;

// Bits in DDCA_Global_Feature_Flags:
// Lifecycle:
#define DDCA_SYNTHETIC_VCP_FEATURE_TABLE_ENTRY  0x8000 /**< Used internally to indicate a temporary VCP_Feature_Table_Entry */
#define DDCA_PERSISTENT_METADATA 0x1000  /**< Part of internal feature tables, do not free */
// Describe Provenance:
#define DDCA_USER_DEFINED  0x4000        /**< User provided feature definition */
#define DDCA_SYNTHETIC     0x2000        /**< Generated feature definition  */

typedef uint16_t DDCA_Feature_Flags;    // union (DDCA_Version_Feature_Flags, DDCA_Global_Feature_Flags)


/** One entry in array listing defined simple NC values.
 *
 * An entry of {0x00,NULL} terminates the list.
 */
typedef
struct {
   uint8_t   value_code;
   char *    value_name;
} DDCA_Feature_Value_Entry;

// Makes reference to feature value table less implementation specific
// deprecated
// typedef DDCA_Feature_Value_Entry * DDCA_Feature_Value_Table;

#define DDCA_FEATURE_METADATA_MARKER  "FMET"
/** Describes a VCP feature code, tailored for a specific monitor.
 *  Feature metadata can vary by VCP version and user defined features */
typedef
struct {
   char                                  marker[4];      /**< always "FMET" */
   DDCA_Vcp_Feature_Code                 feature_code;   /**< VCP feature code */
   DDCA_MCCS_Version_Spec                vcp_version;    /**< MCCS version    */
   DDCA_Feature_Flags                    feature_flags;  /**< feature type description */
   DDCA_Feature_Value_Entry *            sl_values;      /**< valid when DDCA_SIMPLE_NC set */
   void *                                unused;         /** no longer used, was latest_sl_values */
   char *                                feature_name;   /**< feature name */
   char *                                feature_desc;   /**< feature description */
   // possibly add pointers to formatting functions
} DDCA_Feature_Metadata;


//
// Represent the Capabilities string returned by a monitor
//

#define DDCA_CAP_VCP_MARKER  "DCVP"
/** Represents one feature code in the vcp() section of the capabilities string. */
typedef
struct {
   char                                 marker[4];     /**< Always DDCA_CAP_VCP_MARKER */
   DDCA_Vcp_Feature_Code                feature_code;  /**< VCP feature code */
   int                                  value_ct;      /**< number of values declared */
   uint8_t *                            values;        /**< array of declared values */
} DDCA_Cap_Vcp;

#define DDCA_CAPABILITIES_MARKER   "DCAP"
/** Represents a monitor capabilities string */
typedef
struct {
   char                                 marker[4];       /**< always DDCA_CAPABILITIES_MARKER */
   char *                               unparsed_string; /**< unparsed capabilities string */
   DDCA_MCCS_Version_Spec               version_spec;    /**< parsed mccs_ver() field */
   int                                  cmd_ct;          /**< number of command codes */
   uint8_t *                            cmd_codes;       /**< array of command codes */
   int                                  vcp_code_ct;     /**< number of features in vcp() field */
   DDCA_Cap_Vcp *                       vcp_codes;       /**< array of pointers to structs describing each vcp code */
   int                                  msg_ct;
   char **                              messages;
} DDCA_Capabilities;


//
// Get and set VCP feature values
//

/** Indicates the physical data type.  At the DDC level, continuous (C) and
 *  non-continuous (NC) features are treated identically.  They share the same
 *  DDC commands (Get VCP Feature and VCP Feature Reply) and data structure.
 *  Table (T) features use DDC commands Table Write and Table Read, which take
 *  different data structures.
 */
typedef enum {
   DDCA_NON_TABLE_VCP_VALUE = 1,   /**< Continuous (C) or Non-Continuous (NC) value */
   DDCA_TABLE_VCP_VALUE     = 2,       /**< Table (T) value */
} DDCA_Vcp_Value_Type;


typedef struct {
   uint8_t    mh;
   uint8_t    ml;
   uint8_t    sh;
   uint8_t    sl;
} DDCA_Non_Table_Vcp_Value;


/** Represents a single table VCP value.   Consists of a count, and a pointer to the bytes */
typedef struct {
   uint16_t bytect;        /**< Number of bytes in value */
   uint8_t*  bytes;        /**< Bytes of the value */
} DDCA_Table_Vcp_Value;


/** Stores a VCP feature value of any type */
typedef struct {
   DDCA_Vcp_Feature_Code  opcode;         /**< VCP feature code */
   DDCA_Vcp_Value_Type    value_type;      // probably a different type would be better
   union {
      struct {
         uint8_t *  bytes;          /**< pointer to bytes of table value */
         uint16_t   bytect;         /**< number of bytes in table value */
      }         t;                  /**< table value */
      struct {
         uint8_t    mh;
         uint8_t    ml;
         uint8_t    sh;
         uint8_t    sl;
      }    c_nc;                /**< continuous non-continuous, i.e. non-table, value */
   }       val;
} DDCA_Any_Vcp_Value;

#define VALREC_CUR_VAL(valrec) ( valrec->val.c_nc.sh << 8 | valrec->val.c_nc.sl )
#define VALREC_MAX_VAL(valrec) ( valrec->val.c_nc.mh << 8 | valrec->val.c_nc.ml )


//
// For reporting display status changes to client
//

//! Type of event being reported
//!
//!  @since 2.1.0
//!
//! As of ddcutil 2.2.0, events of type DDCA_EVENT_DPMS_AWAKE and
//! DDCA_EVENT_DPMS_ASLEEP are no longer issued.
typedef enum {
   DDCA_EVENT_DPMS_AWAKE,
   DDCA_EVENT_DPMS_ASLEEP,
   DDCA_EVENT_DISPLAY_CONNECTED,
   DDCA_EVENT_DISPLAY_DISCONNECTED,
   DDCA_EVENT_DDC_ENABLED,
   DDCA_EVENT_UNUSED2,
} DDCA_Display_Event_Type;


//! Specifies groups of Display_Status_Event_Type to watch for
//!
//! The enum values are defined as 1,2,4 etc so that they can be or'd.
//!
//!  @since 2.1.0
typedef enum {
   DDCA_EVENT_CLASS_NONE               = 0,
   DDCA_EVENT_CLASS_DPMS               = 1,   // unimplemented as of 2.2.0
   DDCA_EVENT_CLASS_DISPLAY_CONNECTION = 2,
   DDCA_EVENT_CLASS_UNUSED1            = 4,
} DDCA_Display_Event_Class;

#define DDCA_EVENT_CLASS_ALL ((DDCA_Display_Event_Class)(DDCA_EVENT_CLASS_DPMS | DDCA_EVENT_CLASS_DISPLAY_CONNECTION))


/** Event record passed by a display status callback function.
 *
 *  @remark
 *  Strictly speaking, connector names can have a maximum length of 32 characters.
 *  In practice, no connector names longer than 16 characters have ever been
 *  seen.  Therefore, it should be safe for connector_name to allow for a
 *  maximum of 31 printable characters and a termination byte.
 *
 *  @remark
 *  The DDCA_Display_Status_Event is defined with unused fields at the end to allow
 *  for future extension without breaking the ABI.
 *
 *  @since 2.1.0
 *
 *  @remark
 *  Field flags with bit DDCA_DISPLAY_EVENT_DDC_WORKING added in 2.2.0
 */

#define DDCA_DISPLAY_EVENT_DDC_WORKING 0x08

typedef struct {
   uint64_t                timestamp_nanos;
   DDCA_Display_Event_Type event_type;
   DDCA_IO_Path            io_path;
   char                    connector_name[32];
   DDCA_Display_Ref        dref;
   uint8_t                 flags;
   void *                  unused[1];
} DDCA_Display_Status_Event;


/** Signature of a function to be invoked by the shared library notifying the
 *  client that a change in connected displays has been detected.
 *
 *  The client program should call #ddca_redetect_displays() and then
 *  #ddca_get_display_refs() to get the currently valid display references.
 *
 *  @remark
 *  The DDCA_Display_Status_Event is passed on the stack, not allocated on
 *  the heap. Callback invocation is extremely infrequent, the struct size is
 *  not large, and passing the event on the stack relieves clients of
 *  responsibility for memory management.
 *
 *  @since 2.1.0
 */
typedef
void (*DDCA_Display_Status_Callback_Func)(DDCA_Display_Status_Event event);



/** Parameter record for tuning watching display connection changes.
 *
 *  @remark
 *  This struct is defined with unused fields at the end to allow for future
 *  extension without breaking the ABI.
 *
 *  @since 2.2.0
 */
//! @remark see init_display_watch_options() for consideration of what to add
typedef struct {
   // Polling loop intervals for each of the watch modes
   uint16_t   xevent_watch_interval_millisec;   /**< For watch mode XEVENT */
   uint16_t   poll_watch_interval_millisec;     /**< For watch mode POLL */

   // Once an event is received that possibly indicates a display change, libddcutil
   // repeatedly checks /sys/class/drm until the reported displays stabilize
   uint16_t  initial_stabilization_millisec; /**< Delay before first_initialization check */
   uint16_t  stabilization_poll_millisec;    /**< Polling interval between stabilization checks */

   // Interval at which to check that DDC has become enabled if it is not
   // immediately enabled when the EDID is detected.
   uint16_t   watch_retry_interval_millisec;

   void *  unused[4];
} DDCA_DW_Settings;

#ifdef __cplusplus
}
#endif

#endif /* DDCUTIL_TYPES_H_ */
