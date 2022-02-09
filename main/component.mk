#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_ADD_LDFLAGS=-Wl,--whole-archive build/$(COMPONENT_NAME)/lib$(COMPONENT_NAME).a -Wl,--no-whole-archive