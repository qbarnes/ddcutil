/** @file app_experimental.c */

// Copyright (C) 2021-2025 Sanford Rockowitz <rockowitz@minsoft.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <assert.h>
#include <stdio.h>
#include <strings.h>

#include "util/report_util.h"
#include "util/string_util.h"
#include "util/timestamp.h"

#include "base/parms.h"
#include "i2c/i2c_bus_core.h"
#include "i2c/i2c_edid.h"
#include "i2c/i2c_strategy_dispatcher.h"

#include "ddc/ddc_display_ref_reports.h"
#include "ddc/ddc_displays.h"
#include "ddc/ddc_packet_io.h"
#include "ddc/ddc_dw_common.h"
#include "ddc/ddc_dw_main.h"

#include "app_experimental.h"


#define REPORT_FLAG_OPTION(_flagno, _action) \
rpt_vstring(depth+1, "Utility option --f"#_flagno" %s %s",   \
     (parsed_cmd->flags2 & CMD_FLAG2_F##_flagno ) ? "enabled: " : "disabled:", _action)

void
report_experimental_options(Parsed_Cmd * parsed_cmd, int depth)
{
   bool saved_prefix_report_output = rpt_set_ornamentation_enabled(false);

#ifdef UNUSED
   char buf0[80];
   g_snprintf(buf0, 80, "Use non-default watch mode (default = %s)", ddc_watch_mode_name(ddc_watch_mode));
#endif
   char buf5[80];
   g_snprintf(buf5, 80, "Use non-default value for EDID read uses I2C layer (default=%s)", SBOOL(DEFAULT_EDID_READ_USES_I2C_LAYER));

   rpt_label(depth, "Experimental Options:");
   REPORT_FLAG_OPTION(1,  "Suppress SE_POST_READ");
   REPORT_FLAG_OPTION(2,  "Experimental sysfs analysis");    // was Filter phantom displays
   REPORT_FLAG_OPTION(3,  "DDC Null Message never indicates invalid feature");
   REPORT_FLAG_OPTION(4,  "Read strategy tests");
   REPORT_FLAG_OPTION(5,  buf5);
   REPORT_FLAG_OPTION(6,  "Use DRM connector states");
   REPORT_FLAG_OPTION(7,  "Disable phantom display detection");
   REPORT_FLAG_OPTION(8,  "Redirect report output to syslog");
   // REPORT_FLAG_OPTION(9,  buf0);
   REPORT_FLAG_OPTION(9,  "Message to syslog only");
   REPORT_FLAG_OPTION(10, "Extended sleep for DDC Null Msg");
   REPORT_FLAG_OPTION(11, "Explore monitor state tests");
   REPORT_FLAG_OPTION(12, "Disable DRM services");
   REPORT_FLAG_OPTION(13, "Use all_displays_drm_using_drm_api()");
   REPORT_FLAG_OPTION(14, "Debug flock");
#ifdef GET_EDID_USING_SYSFS
   REPORT_FLAG_OPTION(15, "Verify sysfs EDID reads");
#else
   REPORT_FLAG_OPTION(15, "Unused");
#endif
   // REPORT_FLAG_OPTION(16, "Simple report /sys/class/drm");
   REPORT_FLAG_OPTION(16, "Decorate report output");
   REPORT_FLAG_OPTION(17, "Do not use sysfs connector_id");
   REPORT_FLAG_OPTION(18, "Always report UDEV events");
   REPORT_FLAG_OPTION(19, "Stabilize added buses with edid");
   REPORT_FLAG_OPTION(20, "DO NOT use x37 detection state hash");
   REPORT_FLAG_OPTION(21, "Force sysfs unreliable");
   REPORT_FLAG_OPTION(22, "Force sysfs reliable");
   REPORT_FLAG_OPTION(23, "Set global primitive_sysfs");
   REPORT_FLAG_OPTION(24, "Write detect to status if nvidia driver");

   rpt_vstring(depth+1, "Utility option --i1:          Extra millisec to wait after apparent display disconnect (default = %d)", DEFAULT_INITIAL_STABILIZATION_MILLISEC);
   rpt_vstring(depth+1, "Utility option --i2:          NULL Response Hack Millis");
   rpt_vstring(depth+1, "Utility option --i3:          flock_poll_millisec (default = %d)", DEFAULT_FLOCK_POLL_MILLISEC);
   rpt_vstring(depth+1, "Utility option --i4:          flock_max_wait_millisec (default = %d", DEFAULT_FLOCK_MAX_WAIT_MILLISEC);
   rpt_vstring(depth+1, "Utility option --i5:          Max retries for setvcp verification failure");
   rpt_vstring(depth+1, "Utility option --i6:          Unused");
   rpt_vstring(depth+1, "Utility option --i7           Stabilization poll millisec (default=%d)", DEFAULT_STABILIZATION_POLL_MILLISEC);
   rpt_vstring(depth+1, "Utility option --i8:          Display watch udev loop millisec (default = %d)", DEFAULT_UDEV_WATCH_LOOP_MILLISEC);
   rpt_vstring(depth+1, "Utility option --i9:          Display watch non-udev polling loop millisec (default=%d)", DEFAULT_POLL_WATCH_LOOP_MILLISEC);
   rpt_vstring(depth+1, "Utility option --i10:         Display watch xevent polling loop millisec (default=%d)", DEFAULT_XEVENT_WATCH_LOOP_MILLISEC);
   rpt_vstring(depth+1, "Utility option --i11:         Unused");
   rpt_vstring(depth+1, "Utility option --i12:         Unused");
   rpt_vstring(depth+1, "Utility option --i13:         Unused");
   rpt_vstring(depth+1, "Utility option --i14:         Unused");
   rpt_vstring(depth+1, "Utility option --i15:         Unused");
   rpt_vstring(depth+1, "Utility option --i16:         Unused");

   rpt_vstring(depth+1, "Utility option --s1:          Unused");
   rpt_vstring(depth+1, "Utility option --s2:          Unused");
   rpt_vstring(depth+1, "Utility option --s3:          Unused");
   rpt_vstring(depth+1, "Utility option --s4:          Unused");

   rpt_vstring(depth+1, "Utility option --fl1:         Unused");
   rpt_vstring(depth+1, "Utility option --fl2:         Unused");

   rpt_nl();

   rpt_set_ornamentation_enabled(saved_prefix_report_output);
}

#undef REPORT_FLAG_OPTION


//
// Test display detection variants
//

typedef enum {
   _DYNAMIC = 0,
   _128     = 128,
   _256     = 256
} Edid_Read_Size_Option;

static char * read_size_name(int n) {
   char * result = NULL;
   switch (n) {
   case   0: result = "dynamic";  break;
   case 128: result = "128";      break;
   case 256: result = "256";      break;
   default:  result = "INVALID";  break;
   }
   return result;
}


/** Tests for display detection variants.
 *
 *  Controlled by utility option --f4
 */
void test_display_detection_variants() {

   typedef enum {
      _FALSE,
      _TRUE,
      _DNA
   } Bytewise_Option;

   typedef struct {
      I2C_IO_Strategy_Id     i2c_io_strategy_id;
      bool                   edid_uses_i2c_layer;
      Bytewise_Option        edid_read_bytewise;    // applies when edid_uses_i2c_layer == FALSE
      Bytewise_Option        i2c_read_bytewise;     // applies when edid_uses_i2c_layer == TRUE
      bool                   write_before_read;
      Edid_Read_Size_Option  edid_read_size;
   } Choice_Entry;

   typedef struct {
      int      valid_display_ct;
      uint64_t elapsed_nanos;
   } Choice_Results;

   char * choice_name[] = {"false", "true", "DNA"};

   // char * read_size_name[] = {"dynamic", "128", "256"};

   Choice_Entry choices[] =
   //                          use I2c edid        i2c          write     EDID Read
   // i2c_io_strategy          layer   bytewise    bytewise     b4 read   Size
   // ================         ======  ========     =======     =======   ========
   {

     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _FALSE,      _DNA,      _FALSE,   _128},
     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _FALSE,      _DNA,      _FALSE,   _256},
     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _FALSE,      _DNA,      _FALSE,   _DYNAMIC},

     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _FALSE,      _DNA,      _TRUE,    _128},
     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _FALSE,      _DNA,      _TRUE,    _256},
     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _FALSE,      _DNA,      _TRUE,    _DYNAMIC},

     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _TRUE,       _DNA,      _FALSE,   _128},
     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _TRUE,       _DNA,      _FALSE,   _256},
     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _TRUE,       _DNA,      _FALSE,   _DYNAMIC},

     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _TRUE,       _DNA,      _TRUE,    _128},
     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _TRUE,       _DNA,      _TRUE,    _256},
     {I2C_IO_STRATEGY_IOCTL,   _DNA,    _TRUE,       _DNA,      _TRUE,    _DYNAMIC},

   };
   int choice_ct = ARRAY_SIZE(choices);

   Choice_Results results[ARRAY_SIZE(choices)];

   int d = 1;
   for (int ndx=0; ndx<choice_ct; ndx++) {
      // sleep_millis(1000);
      Choice_Entry   cur        = choices[ndx];
      Choice_Results* cur_result = &results[ndx];

      rpt_nl();
      rpt_vstring(0, "===========> IO STRATEGY %d:", ndx+1);
       char * s = (cur.i2c_io_strategy_id == I2C_IO_STRATEGY_IOCTL) ? "IOCTL" : "FILEIO";
       rpt_vstring(d, "i2c_io_strategy:          %s", s);

       rpt_vstring(d, "EDID read uses I2C layer: %s", (cur.edid_uses_i2c_layer) ? "I2C Layer" : "Directly"); // SBOOL(cur.edid_uses_i2c_layer));

    // rpt_vstring(d, "i2c_read_bytewise:        %s", choice_name[cur.i2c_read_bytewise]);
       rpt_vstring(d, "EDID read bytewise:       %s", choice_name[cur.edid_read_bytewise]);
       rpt_vstring(d, "write before read:        %s", SBOOL(cur.write_before_read));
       rpt_vstring(d, "EDID read size:           %s", read_size_name(cur.edid_read_size));

       DDC_Read_Bytewise        = false;       //      cur.i2c_read_bytewise;
       EDID_Read_Bytewise       = cur.edid_read_bytewise;
       EDID_Read_Size           = cur.edid_read_size;
       assert(EDID_Read_Size == 128 || EDID_Read_Size == 256 || EDID_Read_Size == 0);

       // discard existing detected monitors
       ddc_discard_detected_displays();
       uint64_t start_time = cur_realtime_nanosec();
       ddc_ensure_displays_detected();
       int valid_ct = ddc_get_display_count(/*include_invalid_displays*/ false);
       uint64_t end_time = cur_realtime_nanosec();
       cur_result->elapsed_nanos = end_time-start_time;
       rpt_vstring(d, "Valid displays:           %d", valid_ct);
       cur_result->valid_display_ct = valid_ct;
       rpt_vstring(d, "Elapsed time:           %s seconds", formatted_time_t(end_time - start_time));
       rpt_nl();
       // will include any USB or ADL displays, but that's ok
       ddc_report_displays(/*include_invalid_displays=*/ true, 0);
   }

   rpt_label(  d, "SUMMARY");
   rpt_nl();
   // will be wrong for our purposes if same monitor appears on 2 i2c buses
   // int total_displays = get_sysfs_drm_edid_count();

   // ddc_discard_detected_displays();
   // ddc_ensure_displays_detected();  // to perform normal detection
   // int total_displays = get_display_count(/*include_invalid_displays*/ true);
   // rpt_vstring(d, "Total Displays (per /sys/class/drm): %d", total_displays);
   rpt_nl();

   rpt_vstring(d, "   I2C IO    EDID        EDID Read   Write    EDID Read Valid    Seconds");
   rpt_vstring(d, "   Strategy  Method      Bytewise    b4 Read  Size      Displays         ");
   rpt_vstring(d, "   =======   ========    =========   =======  ========= ======== =======");
   for (int ndx = 0; ndx < choice_ct; ndx++) {
      Choice_Entry cur = choices[ndx];
      Choice_Results* cur_result = &results[ndx];

      rpt_vstring(d, "%2d %-7s   %-9s   %-7s     %-5s    %-7s %3d      %s",
            ndx+1,
            (cur.i2c_io_strategy_id == I2C_IO_STRATEGY_IOCTL) ? "IOCTL" : "FILEIO",
            (cur.edid_uses_i2c_layer) ? "I2C Layer" : "Directly",
        //    choice_name[cur.i2c_read_bytewise],
            choice_name[cur.edid_read_bytewise],
            SBOOL(cur.write_before_read),
            read_size_name(cur.edid_read_size),
            cur_result->valid_display_ct,
            formatted_time_t(cur_result->elapsed_nanos));
   }
   rpt_nl();
#ifdef DO_NOT_DISTRIBUTE
   rpt_label(d, "Failures");
   rpt_nl();
   for (int ndx = 0; ndx < choice_ct; ndx++) {
      Choice_Entry cur = choices[ndx];
      Choice_Results* cur_result = &results[ndx];

      if (cur_result->valid_display_ct < 3)
      rpt_vstring(d, "%2d %-7s   %-9s   %-7s     %-5s    %-7s %3d      %s",
            ndx+1,
            (cur.i2c_io_strategy_id == I2C_IO_STRATEGY_FILEIO) ? "FILEIO" : "IOCTL",
            (cur.edid_uses_i2c_layer) ? "I2C Layer" : "Directly",
        //    choice_name[cur.i2c_read_bytewise],
            choice_name[cur.edid_read_bytewise],
            SBOOL(cur.write_before_read),
            read_size_name(cur.edid_read_size),
            cur_result->valid_display_ct,
            formatted_time_t(cur_result->elapsed_nanos));
   }
#endif
}
