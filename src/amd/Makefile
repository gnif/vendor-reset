vendor-reset-y += \
	src/amd/common.o \
	src/amd/compat.o \
	src/amd/firmware.o \
	src/amd/polaris10.o \
	src/amd/vega10.o \
	src/amd/vega20.o \
	src/amd/navi10.o \
	src/amd/amdgpu/common_baco.o \
	src/amd/amdgpu/vega10_reg_init.o \
	src/amd/amdgpu/navi10_reg_init.o \
	src/amd/amdgpu/navi12_reg_init.o \
	src/amd/amdgpu/navi14_reg_init.o \
	src/amd/amdgpu/amdgpu_device.o \
	src/amd/amdgpu/amdgpu_discovery.o \
	src/amd/amdgpu/atom.o \
	src/amd/amdgpu/amdgpu_bios.o \
	src/amd/amdgpu/amdgpu_atomfirmware.o 

ccflags-y += \
	-I$(src)/src/amd \
	-I$(src)/src/amd/amdgpu \
	-I$(src)/src/amd/amdgpu/include \
	-I$(src)/src/amd/amdgpu/include/asic_reg \
	$(foreach inc,$(wildcard $(src)/src/amd/amdgpu/include/asic_reg/*/.),-I$(dir $(inc)) )
