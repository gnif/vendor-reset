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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "vendor-reset-ioctl.h"
#include "ucommon.h"

void help_(const char *prog);

void help_(const char *prog)
{
  fprintf(stderr,
          "Usage:\n"
          "  %s [domain:]bus:dev.fn\n",
          prog);
  exit(1);
}
#define help() help_(argv[0])

int main(int argc, char *argv[])
{
  int ret;
  struct dbdf devref;

  if (argc < 2)
    help();

  if (parse_dbdf(argv[1], &devref))
    help();

  int fd = open("/dev/vendor_reset", O_RDWR);
  if (fd < 0)
  {
    perror("open");
    return fd;
  }

  struct vendor_reset_ioctl dev = {
    .domain = devref.domain,
    .bus = devref.bus,
    .devfn = ((devref.device & 0x1f) << 3) | (devref.function & 0x07),
  };

  ret = ioctl(fd, VENDOR_RESET_RESET, &dev);
  if (ret < 0)
  {
    perror("ioctl");
    goto err;
  }

err:
  close(fd);
  return 0;
}
