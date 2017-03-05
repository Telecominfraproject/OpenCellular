libhw-dir := $(dir $(lastword $(MAKEFILE_LIST)))

DESTDIR ?= dest

name ?= hw
hw-install = Makefile Makefile.proof gnat.adc spark.adc

top := $(CURDIR)
cnf := .config
obj := build

link-type ?= archive

ifeq ($(link-type),archive)
prefixed-name ?= lib$(name)
binary-suffix ?= .a
else ifeq ($(link-type),program)
prefixed-name ?= $(name)
binary-suffix ?=
endif
binary := $(obj)/$(prefixed-name)$(binary-suffix)
$(binary):

space :=
space +=
comma := ,

strip_quotes = $(strip $(subst ",,$(1)))
# fix syntax highlighting with an odd emoticon "))

$(foreach dep,$($(name)-deplibs), \
	$(eval -include $($(dep)-dir)/config))
-include $(cnf)

include $(libhw-dir)/Makefile.proof

CC       = $(CROSS_COMPILE)gcc
GNATBIND = $(CROSS_COMPILE)gnatbind

CFLAGS += -Wuninitialized -Wall -Werror
CFLAGS += -pipe -g
CFLAGS += -Wstrict-aliasing -Wshadow
CFLAGS += -fno-common -fomit-frame-pointer
CFLAGS += -ffunction-sections -fdata-sections

ADAFLAGS += $(CFLAGS) -gnatA -gnatec=$(libhw-dir)/gnat.adc -gnatp
# Ada warning options:
#
#  a   Activate most optional warnings.
# .e   Activate every optional warnings.
#  e   Treat warnings and style checks as errors.
#
#  D   Suppress warnings on implicit dereferences:
#      As SPARK does not accept access types we have to map the
#      dynamically chosen register locations to a static SPARK
#      variable.
#
# .H   Suppress warnings on holes/gaps in records:
#      We are modelling hardware here!
#
#  H   Suppress warnings on hiding:
#      It's too annoying, you run out of ideas for identifiers fast.
#
#  T   Suppress warnings for tracking of deleted conditional code:
#      We use static options to select code paths at compile time.
#
#  U   Suppress warnings on unused entities:
#      Would have lots of warnings for unused register definitions,
#      `withs` for debugging etc.
#
# .U   Deactivate warnings on unordered enumeration types:
#      As SPARK doesn't support `pragma Ordered` by now, we don't
#      use that, yet.
#
# .W   Suppress warnings on unnecessary Warnings Off pragmas:
#      Things get really messy when you use different compiler
#      versions, otherwise.
# .Y   Disable information messages for why package spec needs body:
#      Those messages are annoying. But don't forget to enable those,
#      if you need the information.
ADAFLAGS += -gnatwa.eeD.HHTU.U.W.Y
# Disable style checks for now
ADAFLAGS += -gnatyN

MAKEFLAGS += -rR

# Make is silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
.SILENT:
endif

# must come rather early
.SECONDEXPANSION:

ifeq ($(link-type),archive)
$(binary): $$($(name)-objs)
	@printf "    AR         $(subst $(obj)/,,$@)\n"
	ar cr $@ $^
else ifeq ($(link-type),program)
$(binary): $$($(name)-objs)
	@printf "    LINK       $(subst $(obj)/,,$@)\n"
	$(CC) -o $@ $^ \
		$(addprefix -L,$(abspath $($(name)-extra-objs))) \
		$(addprefix -l,$(patsubst lib%,%,$($(name)-deplibs))) \
		-lgnat
endif

$(foreach dep,$($(name)-deplibs), \
	$(eval $(name)-extra-objs += $($(dep)-dir)/lib) \
	$(eval $(name)-extra-incs += $($(dep)-dir)/include))

# Converts one or more source file paths to their corresponding build/ paths.
# Only .ads and .adb get converted to .o, other files keep their names.
# $1 file path (list)
src-to-obj = \
	$(addprefix $(obj)/,\
	$(patsubst $(obj)/%,%, \
	$(patsubst %.c,%.o,\
	$(patsubst %.ads,%.o,\
	$(patsubst %.adb,%.o,\
	$(2))))))

# Converts one or more source file paths to the corresponding build/ paths
# of their Ada library information (.ali) files.
# $1 file path (list)
src-to-ali = \
	$(addprefix $(obj)/,\
	$(patsubst $(obj)/%,%, \
	$(patsubst %.ads,%.ali,\
	$(patsubst %.adb,%.ali,\
	$(filter %.ads %.adb,$(2))))))

# Clean -y variables, include Makefile.inc
# Add paths to files in $(name)-y to $(name)-srcs
# Add subdirs-y to subdirs
# $1 dir to read Makefile.inc from
includemakefile = \
	$(eval $(name)-y :=) \
	$(eval $(name)-main-y :=) \
	$(eval $(name)-gen-y :=) \
	$(eval $(name)-proof-y :=) \
	$(eval subdirs-y :=) \
	$(eval include $(1)/Makefile.inc) \
	$(eval $(name)-main-src += \
		$(subst $(top)/,, \
		$(abspath $(patsubst $(1)//%,/%,$(addprefix $(1)/,$($(name)-main-y)))))) \
	$(eval $(name)-srcs += \
		$(subst $(top)/,, \
		$(abspath $(patsubst $(1)//%,/%,$(addprefix $(1)/,$($(name)-y)))))) \
	$(eval $(name)-gens += \
		$(subst $(top)/,, \
		$(abspath $($(name)-gen-y)))) \
	$(eval $(name)-proof += \
		$(subst $(top)/,, \
		$(abspath $(addprefix $(1)/,$($(name)-proof-y))))) \
	$(eval subdirs += \
		$(subst $(top)/,, \
		$(abspath $(addprefix $(1)/,$(subdirs-y))))) \

# For each path in $(subdirs) call includemakefiles
# Repeat until subdirs is empty
evaluate_subdirs = \
	$(eval cursubdirs := $(subdirs)) \
	$(eval subdirs :=) \
	$(foreach dir,$(cursubdirs), \
		$(call includemakefile,$(dir))) \
	$(if $(subdirs), \
		$(call evaluate_subdirs))

# collect all object files eligible for building
subdirs := .
$(call evaluate_subdirs)

$(name)-srcs += $($(name)-main-src)

# Eliminate duplicate mentions of source files
$(name)-srcs := $(sort $($(name)-srcs))
$(name)-gens := $(sort $($(name)-gens))

# For Ada includes
$(name)-ada-dirs := $(sort $(dir $(filter %.ads %.adb,$($(name)-srcs) $($(name)-gens))))

# To track dependencies, we need all Ada specification (.ads) files in
# *-srcs. Extract / filter all specification files that have a matching
# body (.adb) file here (specifications without a body are valid sources
# in Ada).
$(name)-extra-specs := \
	$(filter \
		$(addprefix %/,$(patsubst %.adb,%.ads,$(notdir $(filter %.adb,$($(name)-srcs))))), \
		$(filter %.ads,$($(name)-srcs) $($(name)-gens)))
$(name)-srcs := $(filter-out $($(name)-extra-specs),$($(name)-srcs))
$(name)-gens := $(filter-out $($(name)-extra-specs),$($(name)-gens))

$(name)-objs := $(call src-to-obj,,$($(name)-srcs) $($(name)-gens))
$(name)-alis := $(call src-to-ali,,$($(name)-srcs) $($(name)-gens))

# For gnatprove
$(name)-proof-dirs := \
	$(sort	$(dir $($(name)-proof)) \
		$(filter-out ada/% %/ada/%,$($(name)-ada-dirs)))

# For mkdir
alldirs := $(abspath $(dir $(sort $($(name)-objs))))

# Reads dependencies from an Ada library information (.ali) file
# Only basenames (with suffix) are preserved so we have to look the
# paths up in $($(name)-srcs) $($(name)-gens).
# $1 ali file
create_ada_deps = \
	$(if $(wildcard $(1)),\
		$(filter \
			$(addprefix %/,$(shell sed -ne's/^D \([^\t]\+\).*$$/\1/p' $(1) 2>/dev/null)), \
			$($(name)-srcs) $($(name)-gens) $($(name)-extra-specs)), \
		$($(name)-gens))

# Adds dependency rules
# $1 source file
add_ada_deps = \
	$(call src-to-obj,,$(1)): $(1) \
		$(call create_ada_deps,$(call src-to-ali,,$(1)))

# Writes a compilation rule for Ada sources
# $1 source type (ads, adb)
# $2 source files (including the colon)
# $3 obj path prefix (including the trailing slash)
define add_ada_rule
$(2) $(3)%.o: %.$(1)
	@printf "    COMPILE    $$(subst $(obj)/,,$$@)\n"
	$(CC) \
		$(ADAFLAGS) $(addprefix -I,$($(name)-ada-dirs) $($(name)-extra-incs)) \
		-c -o $$@ $$<
endef

# For sources
$(foreach type,ads adb, \
	$(eval $(call add_ada_rule,$(type),$(call src-to-obj,,$(filter %.$(type),$($(name)-srcs))):,$(obj)/)))
$(foreach file,$($(name)-srcs) $($(name)-gens), \
	$(eval $(call add_ada_deps,$(file))))

# For generated sources already in $(obj)/
$(foreach type,ads adb, \
	$(eval $(call add_ada_rule,$(type),$(call src-to-obj,,$(filter %.$(type),$($(name)-gens))):,)))

# Ali files are generated along with the object file. They need an empty
# recipe for correct updating.
$($(name)-alis): %.ali: %.o ;

# To support complex package initializations, we need to call the
# emitted code explicitly. gnatbind gathers all the calls for us
# and exports them as a procedure $(name)_adainit().
ifeq ($(link-type),archive)
$(name)-bind-dirs  := $($(name)-extra-objs)
$(name)-bind-flags := -a -n -L$(name)_ada
$(name)-bind-alis   = $(patsubst /%,%,$(subst $(dir $@),,$^))
else ifeq ($(link-type),program)
$(name)-bind-dirs  := $(sort $(dir $($(name)-objs))) $($(name)-extra-objs)
$(name)-bind-flags :=
$(name)-bind-alis   = $(subst $(dir $@),,$(call src-to-ali,,$($(name)-main-src)))
endif
$(obj)/b__$(prefixed-name).adb: $($(name)-alis)
	@printf "    BIND       $(subst $(obj)/,,$@)\n"
	# We have to give gnatbind a simple filename (without leading
	# path components) so just cd there.
	cd $(dir $@) && \
		$(GNATBIND) $(addprefix -aO,$(abspath $($(name)-bind-dirs))) \
			$($(name)-bind-flags) -o $(notdir $@) $($(name)-bind-alis)
$(eval $(call add_ada_rule,adb,$(obj)/b__$(prefixed-name).o:,))
$(name)-objs += $(obj)/b__$(prefixed-name).o

# Compilation rule for C sources
$(call src-to-obj,,$(filter %.c,$($(name)-srcs))): $(obj)/%.o: %.c
	@printf "    COMPILE    $(subst $(obj)/,,$@)\n"
	$(CC) $(CFLAGS) -c -o $@ $<

$(shell mkdir -p $(alldirs))

$(name)-install-srcs = $(sort \
	$($(name)-extra-specs) $(filter %.ads,$($(name)-srcs)) \
	$(foreach adb,$(filter %.adb,$($(name)-srcs)), \
		$(shell grep -q '^U .*\<BN\>' $(call src-to-ali,,$(adb)) 2>/dev/null && echo $(adb))))

$(name)-install-proof = \
	$(filter $(addprefix %/,$(notdir $($(name)-install-srcs))),$($(name)-proof)) \
	$(filter-out $(addprefix %/,$(notdir $($(name)-proof))),$($(name)-install-srcs))

install: $(binary) $($(name)-alis) $(libgpr)
	install -d $(DESTDIR)/lib $(DESTDIR)/include $(DESTDIR)/proof
	printf "    INSTALL    $(subst $(obj)/,,$(binary))\n"
	install $(binary) $(DESTDIR)/lib/
	$(foreach file,$($(name)-install) $(libgpr), \
		printf "    INSTALL    $(subst $(obj)/,,$(file))\n"; \
		install $(file) $(DESTDIR);)
	printf "    INSTALL    $(cnf)\n"
	install $(cnf) $(DESTDIR)/config
	printf "    INSTALL    %s\n" $(subst $(obj)/,,$($(name)-alis))
	install $($(name)-alis) $(DESTDIR)/lib/
	printf "    INSTALL    %s\n" $(subst $(obj)/,,$($(name)-install-srcs))
	install $($(name)-install-srcs) $(DESTDIR)/include/
	printf "    INSTALL    %s\n" $(subst $(obj)/,,$($(name)-install-proof))
	install $($(name)-install-proof) $(DESTDIR)/proof/

clean::
	rm -rf $(obj) $(name).gpr

distclean: clean
	rm -rf dest

.PHONY: install clean distclean
