PRODUCT = NLOBBY
PLATFORMS += nabu_cpm

# Add-only local NABU wrapper.
# This Makefile exists so the NABU client can live inside the FujiNet lobby tree
# without modifying the upstream clients/Makefile.

SRC_DIRS = ../src/nabu
INCLUDE_DIRS = ../include ../include/nabu ../src/nabu

# This local NABU client uses NABULIB/RetroNET, not the FujiNet platform libs.
undefine FUJINET_LIB

include ../mekkogx/toplevel-rules.mk
