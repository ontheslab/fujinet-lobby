EXECUTABLE = $(R2R_PD)/$(PRODUCT_BASE).com
LIBRARY = $(R2R_PD)/$(PRODUCT_BASE).$(PLATFORM).lib

MWD := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))..)
include $(MWD)/common.mk
include $(MWD)/toolchains/z88dk.mk

NABU_CPM_FLAGS = +cpm -subtype=nabu -create-app -compiler=sdcc -m -vn --list -O3 --opt-code-speed
CFLAGS += $(NABU_CPM_FLAGS)
LDFLAGS += $(NABU_CPM_FLAGS)

r2r:: $(BUILD_EXEC) $(BUILD_LIB) $(R2R_EXTRA_DEPS)
	make -f $(PLATFORM_MK) $(PLATFORM)/r2r-post
