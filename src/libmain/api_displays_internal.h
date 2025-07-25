/** @file api_displays_internal.h
 *
 *  For use only by other api_... files.
 */

// Copyright (C) 2015-2024 Sanford Rockowitz <rockowitz@minsoft.com>
// SPDX-License-Identifier: GPL-2.0-or-later
 
#ifndef API_DISPLAYS_INTERNAL_H_
#define API_DISPLAYS_INTERNAL_H_

#include "public/ddcutil_types.h"

#include "base/core.h"
#include "base/displays.h"
#include "ddc/ddc_displays.h"    // for Dref_Validation_Options

DDCA_Status      ddci_validate_ddca_display_ref2(
      DDCA_Display_Ref        ddca_dref,
      Dref_Validation_Options validation_options,
      Display_Ref**           dref_loc);

DDCA_Status validate_ddca_display_handle(DDCA_Display_Handle ddca_dh, Display_Handle** dh_loc);

#define WITH_VALIDATED_DR4(_ddca_dref, _ddcrc, _validation_options, _action) \
   do { \
      assert(library_initialized); \
      _ddcrc = 0; \
      free_thread_error_detail(); \
      Display_Ref * dref0 = dref_from_published_ddca_dref(ddca_dref); \
      Display_Ref * dref = NULL; \
      if (dref0) \
         _ddcrc = dref_lock(dref0); \
      if (_ddcrc == 0) \
         _ddcrc = ddci_validate_ddca_display_ref2(_ddca_dref, _validation_options, &dref); \
      if (_ddcrc == 0) { \
         (_action); \
      } \
      if (dref0) \
         dref_unlock(dref0); \
   } while(0);

#define WITH_VALIDATED_DH3(ddca_dh, _ddcrc, action) \
   do { \
      assert(library_initialized); \
      _ddcrc = 0; \
      free_thread_error_detail(); \
      Display_Handle * dh = NULL; \
      _ddcrc = validate_ddca_display_handle(ddca_dh, &dh ); \
      if (_ddcrc == 0)  { \
         (action); \
      } \
   } while(0);

const char *
ddci_dh_repr(DDCA_Display_Handle ddca_dh);

void init_api_displays();

#endif /* API_DISPLAYS_INTERNAL_H_ */
