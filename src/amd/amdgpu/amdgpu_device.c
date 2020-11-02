/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 * Copyright 2009 Jerome Glisse.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 *          Alex Deucher
 *          Jerome Glisse
 */

#include <linux/module.h>

#include "common.h"

/*
 * VRAM access helper functions
 */

/**
 * amdgpu_device_vram_access - read/write a buffer in vram
 *
 * @adev: amdgpu_device pointer
 * @pos: offset of the buffer in vram
 * @buf: virtual address of the buffer in system memory
 * @size: read/write size, sizeof(@buf) must > @size
 * @write: true - write to vram, otherwise - read from vram
 */
void amdgpu_device_vram_access(struct amd_fake_dev *adev, loff_t pos,
                               uint32_t *buf, size_t size, bool write)
{
//        unsigned long flags;
        uint32_t hi = ~0;
        uint64_t last;

//        spin_lock_irqsave(&adev->mmio_idx_lock, flags);
        for (last = pos + size; pos < last; pos += 4) {
                uint32_t tmp = pos >> 31;

                WREG32_NO_KIQ(mmMM_INDEX, ((uint32_t)pos) | 0x80000000);
                if (tmp != hi) {
                        WREG32_NO_KIQ(mmMM_INDEX_HI, tmp);
                        hi = tmp;
                }
                if (write)
                        WREG32_NO_KIQ(mmMM_DATA, *buf++);
                else
                        *buf++ = RREG32_NO_KIQ(mmMM_DATA);
        }
//        spin_unlock_irqrestore(&adev->mmio_idx_lock, flags);
}
