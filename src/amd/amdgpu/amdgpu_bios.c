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

#include "atom.h"
#include "common.h"

#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/types.h>
#include <drm/drm_print.h>
/*
 * BIOS.
 */

#define AMD_VBIOS_SIGNATURE " 761295520"
#define AMD_VBIOS_SIGNATURE_OFFSET 0x30
#define AMD_VBIOS_SIGNATURE_SIZE sizeof(AMD_VBIOS_SIGNATURE)
#define AMD_VBIOS_SIGNATURE_END (AMD_VBIOS_SIGNATURE_OFFSET + AMD_VBIOS_SIGNATURE_SIZE)
#define AMD_IS_VALID_VBIOS(p) ((p)[0] == 0x55 && (p)[1] == 0xAA)
#define AMD_VBIOS_LENGTH(p) ((p)[2] << 9)

/* Check if current bios is an ATOM BIOS.
 * Return true if it is ATOM BIOS. Otherwise, return false.
 */
static bool check_atom_bios(uint8_t *bios, size_t size)
{
	uint16_t tmp, bios_header_start;

	if (!bios || size < 0x49)
	{
		DRM_INFO("vbios mem is null or mem size is wrong\n");
		return false;
	}

	if (!AMD_IS_VALID_VBIOS(bios))
	{
		DRM_INFO("BIOS signature incorrect %x %x\n", bios[0], bios[1]);
		return false;
	}

	bios_header_start = bios[0x48] | (bios[0x49] << 8);
	if (!bios_header_start)
	{
		DRM_INFO("Can't locate bios header\n");
		return false;
	}

	tmp = bios_header_start + 4;
	if (size < tmp)
	{
		DRM_INFO("BIOS header is broken\n");
		return false;
	}

	if (!memcmp(bios + tmp, "ATOM", 4) ||
			!memcmp(bios + tmp, "MOTA", 4))
	{
		DRM_DEBUG("ATOMBIOS detected\n");
		return true;
	}

	return false;
}

bool amdgpu_read_bios(struct amd_fake_dev *adev)
{
	uint8_t __iomem *bios;
	size_t size;

	adev->bios = NULL;
	/* XXX: some cards may return 0 for rom size? ddx has a workaround */
	bios = pci_map_rom(adev->pdev, &size);
	if (!bios)
	{
		return false;
	}

	adev->bios = kzalloc(size, GFP_KERNEL);
	if (adev->bios == NULL)
	{
		pci_unmap_rom(adev->pdev, bios);
		return false;
	}
	adev->bios_size = size;
	memcpy_fromio(adev->bios, bios, size);
	pci_unmap_rom(adev->pdev, bios);

	if (!check_atom_bios(adev->bios, size))
	{
		kfree(adev->bios);
		return false;
	}

	return true;
}

static bool amdgpu_read_platform_bios(struct amd_fake_dev *adev)
{
	phys_addr_t rom = adev->pdev->rom;
	size_t romlen = adev->pdev->romlen;
	void __iomem *bios;

	adev->bios = NULL;

	if (!rom || romlen == 0)
		return false;

	adev->bios = kzalloc(romlen, GFP_KERNEL);
	if (!adev->bios)
		return false;

	bios = ioremap(rom, romlen);
	if (!bios)
		goto free_bios;

	memcpy_fromio(adev->bios, bios, romlen);
	iounmap(bios);

	if (!check_atom_bios(adev->bios, romlen))
		goto free_bios;

	adev->bios_size = romlen;

	return true;
free_bios:
	kfree(adev->bios);
	return false;
}

bool amdgpu_get_bios(struct amd_fake_dev *adev)
{
	if (amdgpu_read_bios(adev))
		goto success;

	if (amdgpu_read_platform_bios(adev))
		goto success;

	DRM_ERROR("Unable to locate a BIOS ROM\n");
	return false;

success:
	return true;
}
