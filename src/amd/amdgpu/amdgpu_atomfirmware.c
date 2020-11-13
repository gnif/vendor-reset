/*
 * Copyright 2016 Advanced Micro Devices, Inc.
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
 */
//#include <drm/drm_print.h>

#include "atomfirmware.h"
#include "amdgpu_atomfirmware.h"
#include "atom.h"
#include "atombios.h"
#include "soc15_hw_ip.h"
#include "common.h"

bool amdgpu_atomfirmware_gpu_supports_virtualization(struct amd_fake_dev *adev)
{
	int index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
																					firmwareinfo);
	uint16_t data_offset;

	if (amdgpu_atom_parse_data_header(adev->atom_context, index, NULL,
																		NULL, NULL, &data_offset))
	{
		struct atom_firmware_info_v3_1 *firmware_info =
				(struct atom_firmware_info_v3_1 *)(adev->atom_context->bios +
																					 data_offset);

		if (le32_to_cpu(firmware_info->firmware_capability) &
				ATOM_FIRMWARE_CAP_GPU_VIRTUALIZATION)
			return true;
	}
	return false;
}

void amdgpu_atomfirmware_scratch_regs_init(struct amd_fake_dev *adev)
{
	int index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
																					firmwareinfo);
	uint16_t data_offset;

	if (amdgpu_atom_parse_data_header(adev->atom_context, index, NULL,
																		NULL, NULL, &data_offset))
	{
		struct atom_firmware_info_v3_1 *firmware_info =
				(struct atom_firmware_info_v3_1 *)(adev->atom_context->bios +
																					 data_offset);

		adev->bios_scratch_reg_offset =
				le32_to_cpu(firmware_info->bios_scratch_reg_startaddr);
		DRM_INFO(
				"atomfirmware: bios_scratch_reg_offset initialized to %x\n",
				adev->bios_scratch_reg_offset);
	}
}
