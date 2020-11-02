vendor-reset-y += \
	src/amd/common.o \
	src/amd/vega10.o \
	src/amd/vega20.o \
	src/amd/navi10.o \
	src/amd/amdgpu/common_baco.o \
	src/amd/amdgpu/vega10_reg_init.o \
	src/amd/amdgpu/navi10_reg_init.o \
	src/amd/amdgpu/amdgpu_device.o \
	src/amd/amdgpu/amdgpu_discovery.o

ccflags-y += \
	-I$(src)/src/amd \
	-I$(src)/src/amd/amdgpu \
	-I$(src)/src/amd/amdgpu/include \
	$(foreach inc,$(wildcard $(src)/src/amd/amdgpu/include/asic_reg/*/.),-I$(dir $(inc)) )
