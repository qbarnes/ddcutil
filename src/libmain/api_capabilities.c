/** api_capabilities.c
 *
 *  Capabilities related functions of the API
 */

// Copyright (C) 2015-2025 Sanford Rockowitz <rockowitz@minsoft.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config.h"

#include <assert.h>
#include <glib-2.0/glib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "public/ddcutil_c_api.h"
#include "public/ddcutil_status_codes.h"

#include "util/error_info.h"
#include "util/report_util.h"
#include "util/string_util.h"

#include "base/ddc_command_codes.h"
#include "base/displays.h"
#include "base/feature_metadata.h"
#include "base/rtti.h"
#include "base/vcp_version.h"

#include "vcp/parse_capabilities.h"
#include "vcp/parsed_capabilities_feature.h"
#include "vcp/vcp_feature_codes.h"

#include "dynvcp/dyn_feature_codes.h"
#include "dynvcp/dyn_parsed_capabilities.h"

#include "ddc/ddc_read_capabilities.h"
#include "ddc/ddc_vcp_version.h"

#include "libmain/api_base_internal.h"
#include "libmain/api_displays_internal.h"
#include "libmain/api_metadata_internal.h"

#include "libmain/api_error_info_internal.h"
#include "libmain/api_capabilities_internal.h"
 

//
// Monitor Capabilities
//

DDCA_Status
ddca_get_capabilities_string(
      DDCA_Display_Handle  ddca_dh,
      char**               pcaps_loc)
{
   bool debug = false;
   free_thread_error_detail();
   API_PROLOGX(debug, RESPECT_QUIESCE, "ddca_dh=%s", dh_repr((Display_Handle *) ddca_dh ) );
   API_PRECOND_W_EPILOG(pcaps_loc);
   *pcaps_loc = NULL;
   Error_Info * ddc_excp = NULL;
   DDCA_Status psc = 0;

   WITH_VALIDATED_DH3(ddca_dh, psc,
      {
         char * p_cap_string = NULL;
         ddc_excp = ddc_get_capabilities_string(dh, &p_cap_string);
         psc = (ddc_excp) ? ddc_excp->status_code : 0;
         save_thread_error_detail(error_info_to_ddca_detail(ddc_excp));
         errinfo_free(ddc_excp);
         if (psc == 0) {
            // make copy to prevent caller from mucking around in ddcutil's
            // internal data structures
            *pcaps_loc = g_strdup(p_cap_string);
         }
         ASSERT_IFF(psc==0, *pcaps_loc);
      }
   );

#ifdef TMI
   API_EPILOG_RET_DDCRC(debug, RESPECT_QUIESCE, psc, "ddca_dh=%s, *pcaps_loc=%p -> |%s|",
                     dh_repr((Display_Handle *) ddca_dh),
                     *pcaps_loc, *pcaps_loc );
#endif
   API_EPILOG_RET_DDCRC(debug, RESPECT_QUIESCE, psc, "ddca_dh=%s, *pcaps_loc=%p",
                     dh_repr((Display_Handle *) ddca_dh),
                     *pcaps_loc );
}


#ifdef UNUSED
void
dbgrpt_ddca_cap_vcp(DDCA_Cap_Vcp * cap, int depth) {
   rpt_structure_loc("DDCA_Cap_Vcp", cap, depth);
   int d1 = depth+1;
   int d2 = depth+2;
   rpt_vstring(d1, "feature code:    0x%02x", cap->feature_code);
   rpt_vstring(d1, "value_ct:        %d", cap->value_ct);
   if (cap->value_ct > 0) {
      rpt_label(d1, "Values: ");
      for (int ndx = 0; ndx < cap->value_ct; ndx++) {
         rpt_vstring(d2, "Value:   0x%02x", cap->values[ndx]);
      }
   }
}
#endif


#ifdef UNUSED
void
dbgrpt_ddca_capabilities(DDCA_Capabilities * p_caps, int depth) {
   rpt_structure_loc("DDCA_Capabilities", p_caps, depth);
   int d1 = depth+1;
   int d2 = depth+2;
   rpt_vstring(d1, "Unparsed string: %s", p_caps->unparsed_string);
   rpt_vstring(d1, "Version spec:    %d.%d", p_caps->version_spec.major, p_caps->version_spec.minor);
   rpt_label(d1, "Command codes:");
   for (int ndx = 0; ndx < p_caps->cmd_ct; ndx++) {
      rpt_vstring(d2, "0x%02x", p_caps->cmd_codes[ndx]);
   }
   rpt_vstring(d1, "Feature code count: %d", p_caps->vcp_code_ct);
   for (int ndx = 0; ndx < p_caps->vcp_code_ct; ndx++) {
      DDCA_Cap_Vcp * cur = &p_caps->vcp_codes[ndx];
      dbgrpt_ddca_cap_vcp(cur, d2);
   }
   rpt_vstring(d1, "msg_ct:       %d", p_caps->msg_ct);
   if (p_caps->msg_ct > 0) {
      rpt_label(d1, "messages: ");
      for (int ndx = 0; ndx < p_caps->msg_ct; ndx++) {
         rpt_vstring(d2, "Message:   %s", p_caps->messages[ndx]);
      }
   }
}
#endif


DDCA_Status
ddca_parse_capabilities_string(
      char *                   capabilities_string,
      DDCA_Capabilities **     parsed_capabilities_loc)
{
   bool debug = false;
   free_thread_error_detail();
   API_PROLOGX(debug, NORESPECT_QUIESCE, "parsed_capabilities_loc=%p, capabilities_string: |%s|",
                     parsed_capabilities_loc, capabilities_string);
   API_PRECOND_W_EPILOG(parsed_capabilities_loc);
   DDCA_Status ddcrc = DDCRC_BAD_DATA;
   DDCA_Capabilities * result = NULL;

   // need to control messages?
   Parsed_Capabilities * pcaps = parse_capabilities_string(capabilities_string);
   if (pcaps) {
      if (debug) {
         DBGMSG("Parsing succeeded: ");
         dyn_report_parsed_capabilities(pcaps, NULL, NULL, 2);
         DBGMSG("Convert to DDCA_Capabilities...");
      }
      result = calloc(1, sizeof(DDCA_Capabilities));
      memcpy(result->marker, DDCA_CAPABILITIES_MARKER, 4);
      result->unparsed_string = g_strdup(capabilities_string);     // needed?
      result->version_spec = pcaps->parsed_mccs_version;
      DBGMSF(debug, "version: %d.%d", result->version_spec.major,  result->version_spec.minor);
      Byte_Value_Array bva = pcaps->commands;
      if (bva) {
         result->cmd_ct = bva_length(bva);
         result->cmd_codes = malloc(result->cmd_ct);
         memcpy(result->cmd_codes, bva_bytes(bva), result->cmd_ct);
      }
      // n. needen't set vcp_code_ct if !pcaps, calloc() has done it
      if (pcaps->vcp_features) {
         result->vcp_code_ct = pcaps->vcp_features->len;
         result->vcp_codes = calloc(result->vcp_code_ct, sizeof(DDCA_Cap_Vcp));
         DBGMSF(debug, "allocate %d bytes at %p", result->vcp_code_ct * sizeof(DDCA_Cap_Vcp), result->vcp_codes);
         for (int ndx = 0; ndx < result->vcp_code_ct; ndx++) {
            DDCA_Cap_Vcp * cur_cap_vcp = &result->vcp_codes[ndx];
            DBGMSF(debug, "cur_cap_vcp = %p", &result->vcp_codes[ndx]);
            memcpy(cur_cap_vcp->marker, DDCA_CAP_VCP_MARKER, 4);
            Capabilities_Feature_Record * cur_cfr = g_ptr_array_index(pcaps->vcp_features, ndx);
            DBGMSF(debug, "Capabilities_Feature_Record * cur_cfr = %p", cur_cfr);
            assert(memcmp(cur_cfr->marker, CAPABILITIES_FEATURE_MARKER, 4) == 0);
            if (debug)
               dbgrpt_capabilities_feature_record(cur_cfr, 2);
            //    show_capabilities_feature(cur_cfr, result->version_spec);
            cur_cap_vcp->feature_code = cur_cfr->feature_id;
            DBGMSF(debug, "cur_cfr = %p, feature_code - 0x%02x", cur_cfr, cur_cfr->feature_id);

            // cur_cap_vcp->raw_values = g_strdup(cur_cfr->value_string);
            // TODO: get values from Byte_Bit_Flags cur_cfr->bbflags
#ifdef CFR_BVA
            Byte_Value_Array bva = cur_cfr->values;
            if (bva) {
               cur_cap_vcp->value_ct = bva_length(bva);
               cur_cap_vcp->values = calloc( cur_cap_vcp->value_ct, sizeof(Byte));
               memcpy(cur_cap_vcp->values, bva_bytes(bva), cur_cap_vcp->value_ct);
            }
#endif
#ifdef CFR_BBF
            if (cur_cfr->bbflags) {
               cur_cap_vcp->value_ct = bbf_count_set(cur_cfr->bbflags);
               cur_cap_vcp->values   = calloc(1, cur_cap_vcp->value_ct);
               bbf_to_bytes(cur_cfr->bbflags, cur_cap_vcp->values, cur_cap_vcp->value_ct);
            }
#endif
         }
      }

      // DBGMSG("pcaps->messages = %p", pcaps->messages);
      // if (pcaps->messages) {
      //    DBGMSG("pcaps->messages->len = %d", pcaps->messages->len);
      // }

      if (pcaps->messages && pcaps->messages->len > 0) {
         result->msg_ct = pcaps->messages->len;
         result->messages = g_ptr_array_to_ntsa(pcaps->messages, /*duplicate=*/ true);
      }

      ddcrc = 0;
      free_parsed_capabilities(pcaps);
   }
   *parsed_capabilities_loc = result;
   API_EPILOG_BEFORE_RETURN(debug, NORESPECT_QUIESCE, ddcrc,
         "*parsed_capabilities_loc=%p", *parsed_capabilities_loc);
   ASSERT_IFF(ddcrc==0, *parsed_capabilities_loc);
   // if ( IS_DBGTRC(debug, DDCA_TRC_API) && *parsed_capabilities_loc)
#ifdef TMI
   if (is_traced_api_call(__func__) && *parsed_capabilities_loc)
      dbgrpt_ddca_capabilities(*parsed_capabilities_loc, 2);
#endif
   return ddcrc;
}


void
ddca_free_parsed_capabilities(
      DDCA_Capabilities * pcaps)
{
   bool debug = false;
   reset_current_traced_function_stack();
   DBGTRC_STARTING(debug, DDCA_TRC_API, "pcaps=%p", pcaps);
   if (pcaps) {
      assert(memcmp(pcaps->marker, DDCA_CAPABILITIES_MARKER, 4) == 0);
      free(pcaps->unparsed_string);
      DBGMSF(debug, "vcp_code_ct = %d", pcaps->vcp_code_ct);
      for (int ndx = 0; ndx < pcaps->vcp_code_ct; ndx++) {
         DDCA_Cap_Vcp * cur_vcp = &pcaps->vcp_codes[ndx];
         assert(memcmp(cur_vcp->marker, DDCA_CAP_VCP_MARKER, 4) == 0);
         cur_vcp->marker[3] = 'x';
         free(cur_vcp->values);
      }
      free(pcaps->vcp_codes);
      free(pcaps->cmd_codes);
      ntsa_free(pcaps->messages, true);
      pcaps->marker[3] = 'x';
      free(pcaps);
   }
   DBGTRC_DONE(debug, DDCA_TRC_API, "");
}

#ifdef OLD
DDCA_Status
ddca_report_parsed_capabilities_by_dref(
      DDCA_Capabilities *      p_caps,
      DDCA_Display_Ref         ddca_dref,
      int                      depth)
{
   bool debug = false;
   DDCA_Status ddcrc = 0;
   free_thread_error_detail();
   API_PROLOGX(debug, RESPECT_QUIESCE, "Starting. p_caps=%p", //  ddca_dref=%s",
                      p_caps);      // , dref_repr_t((Display_Ref*) ddca_dref));
   API_PRECOND_W_EPILOG(p_caps);   // no need to check marker, DDCA_CAPABILITIES not opaque

   Display_Ref * dref = NULL;
   // dref may be NULL, but if not it must be valid
   if (ddca_dref) {
      dref = dref_from_published_ddca_dref(ddca_dref);
      // DREF_VALIDAT_BASIC_ONLY?
      ddcrc = (dref) ? ddc_validate_display_ref2(dref, DREF_VALIDATE_BASIC_ONLY) : DDCRC_ARG;
      if (ddcrc != 0) {
         goto bye;
      }
   }

   int d0 = depth;
   int d1 = depth+1;
   int d2 = depth+2;
   int d3 = depth+3;

   DDCA_Output_Level ol = get_output_level();

   if (ol >= DDCA_OL_VERBOSE)
      rpt_vstring(d0, "Unparsed string: %s", p_caps->unparsed_string);

   char * s = NULL;
   if (vcp_version_eq(p_caps->version_spec, DDCA_VSPEC_UNQUERIED))
      s = "Not present";
   else if (vcp_version_eq(p_caps->version_spec, DDCA_VSPEC_UNKNOWN))
      s = "Invalid value";
   else
      s = format_vspec(p_caps->version_spec);
   rpt_vstring(d0, "VCP version: %s", s);
   if (ol >= DDCA_OL_VERBOSE) {
      rpt_label  (d0, "Command codes: ");
      for (int cmd_ndx = 0; cmd_ndx < p_caps->cmd_ct; cmd_ndx++) {
         uint8_t cur_code = p_caps->cmd_codes[cmd_ndx];
         char * cmd_name = ddc_cmd_code_name(cur_code);
         rpt_vstring(d1, "0x%02x (%s)", cur_code, cmd_name);
      }
   }

   rpt_vstring(d0, "VCP Feature codes:");
   for (int code_ndx = 0; code_ndx < p_caps->vcp_code_ct; code_ndx++) {
      DDCA_Cap_Vcp * cur_vcp = &p_caps->vcp_codes[code_ndx];
      assert( memcmp(cur_vcp->marker, DDCA_CAP_VCP_MARKER, 4) == 0);

      Display_Feature_Metadata * dfm =
         dyn_get_feature_metadata_by_dref(
               cur_vcp->feature_code,
               dref,
               true,     // check_udf
               true);    // create_default_if_not_found);
      assert(dfm);
      // dbgrpt_display_feature_metadata(dfm, 3);

      rpt_vstring(d1, "Feature:  0x%02x (%s)", cur_vcp->feature_code, dfm->feature_name);

      if (cur_vcp->value_ct > 0) {
         if (ol > DDCA_OL_VERBOSE)
            rpt_vstring(d2, "Unparsed values:     %s", hexstring_t(cur_vcp->values, cur_vcp->value_ct) );

         DDCA_Feature_Value_Entry * feature_value_table = dfm->sl_values;
         rpt_label(d2, "Values:");
         for (int ndx = 0; ndx < cur_vcp->value_ct; ndx++) {
            char * value_desc = "No lookup table";
            if (feature_value_table) {
               value_desc =
                   sl_value_table_lookup(feature_value_table, cur_vcp->values[ndx]);
               if (!value_desc)
                  value_desc = "Unrecognized feature value";
            }
            rpt_vstring(d3, "0x%02x: %s", cur_vcp->values[ndx], value_desc);
         }
      }
      dfm_free(dfm);
   } // one feature code

   if (p_caps->messages && *p_caps->messages) {
      rpt_nl();
      rpt_label(d0, "Parsing errors:");
      char ** m = p_caps->messages;
      while (*m) {
         rpt_label(d1, *m);
         m++;
      }
   }
   else {
      DBGMSF(debug, "No error messages");
   }

bye:
   API_EPILOG_RET_DDCRC(debug, RESPECT_QUIESCE, ddcrc, "");
}
#endif

DDCA_Status
ddci_report_parsed_capabilities_by_dref(
      DDCA_Capabilities *      p_caps,
      Display_Ref *            dref,
      int                      depth)
{
   bool debug = true;
   DBGTRC_STARTING(debug, DDCA_TRC_API, "");

   int d0 = depth;
   int d1 = depth+1;
   int d2 = depth+2;
   int d3 = depth+3;

   DDCA_Status ddcrc = 0;

   DDCA_Output_Level ol = get_output_level();

   if (ol >= DDCA_OL_VERBOSE)
      rpt_vstring(d0, "Unparsed string: %s", p_caps->unparsed_string);

   char * s = NULL;
   if (vcp_version_eq(p_caps->version_spec, DDCA_VSPEC_UNQUERIED))
      s = "Not present";
   else if (vcp_version_eq(p_caps->version_spec, DDCA_VSPEC_UNKNOWN))
      s = "Invalid value";
   else
      s = format_vspec(p_caps->version_spec);
   rpt_vstring(d0, "VCP version: %s", s);
   if (ol >= DDCA_OL_VERBOSE) {
      rpt_label  (d0, "Command codes: ");
      for (int cmd_ndx = 0; cmd_ndx < p_caps->cmd_ct; cmd_ndx++) {
         uint8_t cur_code = p_caps->cmd_codes[cmd_ndx];
         char * cmd_name = ddc_cmd_code_name(cur_code);
         rpt_vstring(d1, "0x%02x (%s)", cur_code, cmd_name);
      }
   }

   rpt_vstring(d0, "VCP Feature codes:");
   for (int code_ndx = 0; code_ndx < p_caps->vcp_code_ct; code_ndx++) {
      DDCA_Cap_Vcp * cur_vcp = &p_caps->vcp_codes[code_ndx];
      assert( memcmp(cur_vcp->marker, DDCA_CAP_VCP_MARKER, 4) == 0);

      Display_Feature_Metadata * dfm =
         dyn_get_feature_metadata_by_dref(
               cur_vcp->feature_code,
               dref,
               true,     // check_udf
               true);    // create_default_if_not_found);
      assert(dfm);
      // dbgrpt_display_feature_metadata(dfm, 3);

      rpt_vstring(d1, "Feature:  0x%02x (%s)", cur_vcp->feature_code, dfm->feature_name);

      if (cur_vcp->value_ct > 0) {
         if (ol > DDCA_OL_VERBOSE)
            rpt_vstring(d2, "Unparsed values:     %s", hexstring_t(cur_vcp->values, cur_vcp->value_ct) );

         DDCA_Feature_Value_Entry * feature_value_table = dfm->sl_values;
         rpt_label(d2, "Values:");
         for (int ndx = 0; ndx < cur_vcp->value_ct; ndx++) {
            char * value_desc = "No lookup table";
            if (feature_value_table) {
               value_desc =
                   sl_value_table_lookup(feature_value_table, cur_vcp->values[ndx]);
               if (!value_desc)
                  value_desc = "Unrecognized feature value";
            }
            rpt_vstring(d3, "0x%02x: %s", cur_vcp->values[ndx], value_desc);
         }
      }
      dfm_free(dfm);
   } // one feature code

   if (p_caps->messages && *p_caps->messages) {
      rpt_nl();
      rpt_label(d0, "Parsing errors:");
      char ** m = p_caps->messages;
      while (*m) {
         rpt_label(d1, *m);
         m++;
      }
   }
   else {
      DBGMSF(debug, "No error messages");
   }

   DBGTRC_RET_DDCRC(debug, DDCA_TRC_API, ddcrc, "");
   return ddcrc;
}


DDCA_Status
ddca_report_parsed_capabilities_by_dref(
      DDCA_Capabilities *      p_caps,
      DDCA_Display_Ref         ddca_dref,
      int                      depth)
{
   bool debug = false;
   DDCA_Status ddcrc = 0;
   free_thread_error_detail();
   API_PROLOGX(debug, RESPECT_QUIESCE, "Starting. p_caps=%p", //  ddca_dref=%s",
                      p_caps);      // , dref_repr_t((Display_Ref*) ddca_dref));
   API_PRECOND_W_EPILOG(p_caps);   // no need to check marker, DDCA_CAPABILITIES not opaque

   Display_Ref * dref = NULL;
   // dref may be NULL, but if not it must be valid
   if (ddca_dref) {
      dref = dref_from_published_ddca_dref(ddca_dref);
      ddcrc = (dref) ? ddc_validate_display_ref2(dref, DREF_VALIDATE_BASIC_ONLY) : DDCRC_ARG;
   }

   if (ddcrc == 0) {
      ddcrc = ddci_report_parsed_capabilities_by_dref(p_caps, dref, depth);
   }

   API_EPILOG_RET_DDCRC(debug, RESPECT_QUIESCE, ddcrc, "");
}


void
ddca_report_parsed_capabilities(
      DDCA_Capabilities *      p_caps,
      int                      depth)
{
   ddci_report_parsed_capabilities_by_dref(p_caps, NULL, depth);
}


DDCA_Status
ddca_report_parsed_capabilities_by_dh(
      DDCA_Capabilities *      p_caps,
      DDCA_Display_Handle      ddca_dh,
      int                      depth)
{
   bool debug = false;
   free_thread_error_detail();
   API_PROLOGX(debug, RESPECT_QUIESCE, "p_caps=%p, ddca_dh=%s, depth=%d",
                      p_caps, ddca_dh_repr(ddca_dh), depth);
   DDCA_Status ddcrc = 0;
   Display_Handle * dh = (Display_Handle *) ddca_dh;
   if (dh == NULL || memcmp(dh->marker, DISPLAY_HANDLE_MARKER, 4) != 0 ) {
      ddcrc = DDCRC_ARG;
      goto bye;
   }

   // Ensure dh->dref->vcp_version is not unqueried,
   // ddca_report_parsed_capabilities_by_dref() will fail trying to lock the already open device
   get_vcp_version_by_dh(dh);
   DBGMSF(debug, "After get_vcp_version_by_dh(), dh->dref->vcp_version_df=%s",
                 format_vspec_verbose(dh->dref->vcp_version_xdf));

   ddci_report_parsed_capabilities_by_dref(p_caps, dh->dref, depth);

bye:
   API_EPILOG_RET_DDCRC(debug, RESPECT_QUIESCE, ddcrc, "");
}


#ifdef UNPUBLISHED
// UNPUBLISHED
/** Parses a capabilities string, and reports the parsed string
 *  using the code of command "ddcutil capabilities".
 *
 *  The report is written to the current FOUT location.
 *
 *  The detail level written is sensitive to the current output level.
 *
 *  @param[in]  capabilities_string  capabilities string
 *  @param[in]  dref                 display reference
 *  @param[in]  depth  logical       indentation depth
 *
 *  @remark
 *  This function exists as a development aide.  Internally, ddcutil uses
 *  a different data structure than DDCA_Parsed_Capabilities.  That
 *  data structure uses internal collections that are not exposed at the
 *  API level.
 *  @remark
 *  Signature changed in 0.9.3
 *  @since 0.9.0
 */
void ddca_parse_and_report_capabilities(
      char *                    capabilities_string,
      DDCA_Display_Ref          dref,
      int                       depth);


// UNPUBLISHED
void
ddca_parse_and_report_capabilities(
      char *                    capabilities_string,
      DDCA_Display_Ref          dref,
      int                       depth)
{
      Parsed_Capabilities* pcaps = parse_capabilities_string(capabilities_string);
      dyn_report_parsed_capabilities(pcaps, NULL, dref, 0);
      free_parsed_capabilities(pcaps);
}
#endif


DDCA_Feature_List
ddca_feature_list_from_capabilities(
      DDCA_Capabilities * parsed_caps)
{
   DDCA_Feature_List result = {{0}};
   for (int ndx = 0; ndx < parsed_caps->vcp_code_ct; ndx++) {
      DDCA_Cap_Vcp curVcp = parsed_caps->vcp_codes[ndx];
      ddca_feature_list_add(&result, curVcp.feature_code);
   }
   return result;
}

void init_api_capabilities() {
   RTTI_ADD_FUNC(ddca_free_parsed_capabilities);
   RTTI_ADD_FUNC(ddca_get_capabilities_string);
   RTTI_ADD_FUNC(ddca_parse_capabilities_string);
   RTTI_ADD_FUNC(ddci_report_parsed_capabilities_by_dref);
   RTTI_ADD_FUNC(ddca_report_parsed_capabilities_by_dref);
   RTTI_ADD_FUNC(ddca_report_parsed_capabilities_by_dh);
}
