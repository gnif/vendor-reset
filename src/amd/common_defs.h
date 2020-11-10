/*
Vendor Reset - Vendor Specific Reset
Copyright (C) 2020 Geoffrey McRae <geoff@hostfission.com>
Copyright (C) 2020 Adam Madsen <adam@ajmadsen.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef __VENDOR_RESET_COMMON_DEFS_H__
#define __VENDOR_RESET_COMMON_DEFS_H__

/* from amdgpu_discovery.c */
#ifndef mmMM_INDEX
#define mmRCC_CONFIG_MEMSIZE 0xde3
#define mmMM_INDEX 0x0
#define mmMM_INDEX_HI 0x6
#define mmMM_DATA 0x1
#define HW_ID_MAX 300
#endif
/* end from amdgpu_discovery.c */

#endif
