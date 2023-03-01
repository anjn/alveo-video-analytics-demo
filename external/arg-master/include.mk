MYDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include_dirs += $(MYDIR)
