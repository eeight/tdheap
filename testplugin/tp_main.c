
/*--------------------------------------------------------------------*/
/*--- Testplugin: Test plugin.                           tp_main.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of Testplugin, the simplest possible Valgrind tool,
   which does something unclear.

   Copyright (C) 2002-2008 Petr Prokhorenkov
      prokhorenkov@gmail.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"

static void tp_post_clo_init(void)
{
}

static
IRSB* tp_instrument ( VgCallbackClosure* closure,
                      IRSB* bb,
                      VexGuestLayout* layout, 
                      VexGuestExtents* vge,
                      IRType gWordTy, IRType hWordTy )
{
    return bb;
}

static void tp_fini(Int exitcode)
{
}

static void tp_pre_clo_init(void)
{
   VG_(details_name)            ("Testplugin");
   VG_(details_version)         (NULL);
   VG_(details_description)     ("some unclear tool");
   VG_(details_copyright_author)(
      "Copyright (C) 2002-2008 Petr Prokhorenkov.");
   VG_(details_bug_reports_to)  (VG_BUGS_TO);

   VG_(basic_tool_funcs)        (tp_post_clo_init,
                                 tp_instrument,
                                 tp_fini);

   /* No needs, no core events to track */
}

VG_DETERMINE_INTERFACE_VERSION(tp_pre_clo_init)

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
