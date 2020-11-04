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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ucommon.h"

int parse_dbdf(const char *dbdf_str, struct dbdf *out)
{
  char *dbdf, *tok;

  if (!dbdf_str || !(dbdf = strdup(dbdf_str)))
    return 1;

  if (!(tok = strtok(dbdf, ":")))
    goto err;

  out->domain = 0;
  out->bus = strtoul(tok, NULL, 16);

  tok = strtok(NULL, ":");
  if (strtok(NULL, ":"))
  {
    out->domain = out->bus;
    out->bus = strtoul(tok, NULL, 16);
    /* guaranteed to be okay */
    tok += strlen(tok) + 1;
  }

  if (!(tok = strtok(tok, ".")))
    goto err;

  out->device = strtoul(tok, NULL, 16);

  if (!(tok = strtok(NULL, ".")))
    goto err;

  out->function = strtoul(tok, NULL, 16);

  free(dbdf);
  return 0;

err:
  free(dbdf);
  return 1;
}