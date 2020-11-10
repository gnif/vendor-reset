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

#ifndef __VENDOR_RESET_FTRACE_H__
#define __VENDOR_RESET_FTRACE_H__

#include <linux/ftrace.h>

struct ftrace_hook
{
  const char *name;

  void *function;
  void *original;
  unsigned long address;

  struct ftrace_ops ops;
};

#define to_ftrace_hook(ops) container_of(ops, struct ftrace_hook, ops)

#define HOOK(_name, orig, replacement) \
  {                                    \
    .name = (_name),                   \
    .original = (orig),                \
    .function = (replacement),         \
  }

int fh_install_hooks(struct ftrace_hook *hooks);
void fh_remove_hooks(struct ftrace_hook *hooks);

#endif