# makefile to set-up the environment for building...

# This is probably only working with GNU make. The file is structured
# to have variables in the beginning, some helper functions, then so
# implicit rules and in the end targets that invoke them all.
#
# If you need to change sysmocom release or poky base branch then the
# two variables need to be changed.
# If you add a machine then MACHINES need to be adjusted and a new config
# in cfg/NAME/10_NAME needs to be created.
#
# Implicit rules: I used WILDCARD-action for the rules and then get the
# name of the rule from $@ in the rule itself or % when still being in
# the dependency list.


# Make everything more verbose by V=1. Taken from kbuild
ifeq ("$(origin V)", "command line")
 Q =
else
 Q = @
endif

# Variables
SYSMOCOM_RELEASE=201705
POKY_RELEASE=pyro
REPOS=poky meta-telephony meta-sysmocom-bsp meta-qt5 meta-sysmocom-bsp meta-smalltalk
MACHINES=sysmobts sysmobts2100 sysmocom-apu2 sysmocom-alix
FEED_NAME=$(SYSMOCOM_RELEASE)-testing

# The default targets to pick depending on machine
BUILD_TARGET_sysmobts = "meta-toolchain-osmo task-sysmocom-feed sysmocom-core-image sysmocom-nitb-rauc-image image-rauc-rescue-initramfs image-rauc-slot-initramfs image-rauc-ubi"
BUILD_TARGET_sysmobts2100 = "meta-toolchain-osmo task-sysmocom-feed sysmocom-core-image"
BUILD_TARGET_sysmocom-apu2 = "core-image-minimal-initramfs meta-toolchain-osmo task-sysmocom-feed sysmocom-core-image core-image-minimal-initramfs"
BUILD_TARGET_sysmocom-alix = "core-image-minimal-initramfs meta-toolchain-osmo task-sysmocom-feed sysmocom-core-image core-image-minimal-initramfs"

# Pick the one depending on $@. Not sure if .SECONDEXPANSION is more
# approiate here or not.
BUILD_TARGETS="$(BUILD_TARGET_$(CUR_MACHINE))"

# Jenkins jobs...
JOB_FILES=merge_diff.xml nightly.xml show_diff.xml testing.xml test_upgrade_alix.xml test_upgrade_apu2.xml testbranch.xml
JOB_NAME_merge_diff.xml=$(SYSMOCOM_RELEASE)-merge-diff
JOB_NAME_nightly.xml=$(SYSMOCOM_RELEASE)-nightly
JOB_NAME_show_diff.xml=$(SYSMOCOM_RELEASE)-show-diff
JOB_NAME_testing.xml=$(SYSMOCOM_RELEASE)-testing
JOB_NAME_test_upgrade_alix.xml=$(SYSMOCOM_RELEASE)-test-upgrade-alix
JOB_NAME_test_upgrade_apu2.xml=$(SYSMOCOM_RELEASE)-test-upgrade-apu2
JOB_NAME_testbranch.xml=$(SYSMOCOM_RELEASE)-testbranch
JOB_NAME="$(JOB_NAME_$(job))"
VIEW_FILES=view.xml
ALL_JENKINS_FILES=$(JOB_FILES) $(VIEW_FILES)
SED=sed

#
usage:
	@echo "Pick a target like help, update or sysmocom-alix-setup"

help:
	@echo "Set-up build environment and execute builds. This is intended"
	@echo "for customers and employees."
	@echo ""
	@echo "Available targets:"
	@echo " usage			- Default target and print usage"
	@echo " help			- This output"
	@echo " update			- git pull --rebase and initial git clone"
	@echo " setup-all		- Set-up all build directories"
	@echo " build-all		- Build all targets"
	@echo " clean-all		- Clean all targets after build"
	@echo " upload-all		- Upload all targets"
	@echo ' install-ssh-config	- Install Host to $$HOME/.ssh/config'
	@echo "Board specific targets:"
	@$(foreach machine, $(MACHINES),		\
		printf " %-16s	- Configure build directory\\n" $(machine)-setup;)
	@$(foreach machine, $(MACHINES),		\
		printf " %-16s	- Configure build directory\\n" $(machine)-build;)
	@$(foreach machine, $(MACHINES),		\
		printf " %-16s	- Configure build directory\\n" $(machine)-upload;)
	@echo "Server targets:"
	@echo " make-server-structure	- Create directories for machine/release"
	@echo "Jenkins targets:"
	@echo " create-jenkins-jobs-xml	- Create XML files from the templates"
	@echo " create-jenkins-jobs	- Create the Jobs using jenkins-cli.jar"
	@echo "Available variables:"
	@echo " V=1			- Enable verbose command output"
	@echo " SYSMOCOM_RELEASE=name	- Pick branch during clone"
	@echo " POKY_RELEASE=name	- Pick branch during clone"
	@echo " JENKINS_HOST=name	- Hostname of Jenkins"
	@echo " JENKINS_USER=user	- Username for Jenkins"
	@echo " JENKINS_PASS=pass	- Password for Jenkins"
	@echo " WEB_FILES=dir		- Directory name for make-server-structure"
	@echo " SSH_HOST=host		- Hostname for ssh config"
	@echo " SSH_PORT=port		- Port for ssh config"
	@echo " SSH_USER=username	- Username for ssh config"

# Fetch/update all repos... Expand REPOS and append -update to the rule
# e.g. poky-update meta-telephony-update
update: $(foreach repo, $(REPOS), $(repo)-update)

# helper rules

# crazy as I don't know a split()[0]
CUR_MACHINE=$(subst build.,,$(subst -upload,,$(subst -clean,,$(subst -build,,$@))))


## Create a new directory
git:
	$(V)mkdir $@

## Clone repositories. The other option is by variable something like BRNACH_poky, REPO_poky
git/poky: | git
	$(V)cd git && git clone --branch=$(POKY_RELEASE) --depth=1 git://git.yoctoproject.org/poky
git/meta-sysmocom-bsp: | git
	cd git && git clone --branch=$(SYSMOCOM_RELEASE) git://git.sysmocom.de/poky/meta-sysmocom-bsp
git/meta-telephony: | git
	cd git && git clone --branch=$(SYSMOCOM_RELEASE) git://git.osmocom.org/meta-telephony
git/meta-smalltalk: | git
	cd git && git clone --branch=$(SYSMOCOM_RELEASE) git://github.com/sysmocom/meta-smalltalk
git/meta-qt5: | git
	cd git && git clone --branch=$(SYSMOCOM_RELEASE) git://github.com/sysmocom/meta-qt5

## Create a build directory, e.g. build.sysmobts
## Use Poky to set-up the directory and then customize it. Copy files
## around and append to the local.conf and layers.conf
CFG_FILES = $(sort $(wildcard cfg/common/*)) $(sort $(wildcard cfg/$(CUR_MACHINE)/*))
build.%: | git/poky
	@echo "Creating build directory for $(CUR_MACHINE)"
	$(Q)/bin/bash -c "source git/poky/oe-init-build-env $@"

	# Append entries to conf/local.conf. Common first, machine second... filter
	$(Q)$(foreach file,$(CFG_FILES), \
		cat $(file) | sed s,BASE_DIR,$(PWD), >> $@/conf/local.conf;)
	@echo "require conf/distro/include/sysmocom-defaults.conf" >> $@/conf/local.conf

	$(Q)cat cfg/bblayers.conf | sed  s,BASE_DIR,$(PWD), > $@/conf/bblayers.conf


# generic git pull --rebase rule. Let's assume this is matching poky-update
# then the dependency will be "git/poky" and a clone rule will be built.
%-update: | git/$(subst -update,,%)
	@echo "Updating $(subst -update,,$@) ..."
	$(Q)cd git/$(subst -update,,$@) && git pull --rebase

# Setup a build directory
%-setup: | build.$(subst -setup,,%) git/poky
	@echo "Please place proprietary firmware into the downloads directory."

# Start a build..
%-build: | build.$(subst -build,,%) git/poky
	$(Q)/bin/bash -c "source git/poky/oe-init-build-env build.$(CUR_MACHINE) && bitbake $(BUILD_TARGETS)"

%-upload: | build.$(subst -upload,,%) git/poky
	$(Q)cd build.$(CUR_MACHINE) && ../scripts/upload-build.sh $(CUR_MACHINE) $(FEED_NAME)

%-clean: | build.$(subst -clean,,%) git/poky
	$(Q)cd build.$(CUR_MACHINE) && ../git/poky/scripts/sstate-cache-management.sh  --cache-dir=sstate-cache -y -L --stamps-dir=tmp/stamps/
	$(Q)cd build.$(CUR_MACHINE) && rm -rf tmp

# Create all build directories, build everything, upload everything, clean everything
setup-all: | $(foreach machine, $(MACHINES), $(machine)-setup)

build-all: | $(foreach machine, $(MACHINES), $(machine)-build)

upload-all: | $(foreach machine, $(MACHINES), $(machine)-upload)

clean-all: | $(foreach machine, $(MACHINES), $(machine)-clean)

make-server-structure:
ifndef WEB_FILES
	$(error "Please call with make make-server-structure WEB_FILES=...")
endif
	$(Q)$(foreach machine, $(MACHINES), \
		mkdir $(WEB_FILES)/$(machine); \
		mkdir $(WEB_FILES)/$(machine)/$(SYSMOCOM_RELEASE); \
		mkdir $(WEB_FILES)/$(machine)/$(SYSMOCOM_RELEASE)-testing; \
		mkdir $(WEB_FILES)/$(machine)/$(SYSMOCOM_RELEASE)-nightly; \
	)


install-ssh-config: | $(HOME)/.ssh
ifndef SSH_HOST
	$(error "Please call with make $@ SSH_HOST=xyz...")
endif
ifndef SSH_PORT
	$(error "Please call with make $@ SSH_PORT=abc...")
endif
ifndef SSH_USER
	$(error "Please call with make $@ SSH_USER=def...")
endif
	@echo "Host = sysmocom-downloads" >> $(HOME)/.ssh/config
	@echo "  HostName = $(SSH_HOST)" >> $(HOME)/.ssh/config
	@echo "  Port = $(SSH_PORT)" >> $(HOME)/.ssh/config
	@echo "  AddressFamily = inet" >> $(HOME)/.ssh/config
	@echo "  User = $(SSH_USER)" >> $(HOME)/.ssh/config

# Create jenkin job xmls
create-jenkins-jobs-xml:
	$(Q)$(foreach file, $(ALL_JENKINS_FILES), \
		cat jenkins/job_templates/$(file) | \
			$(SED) \
				-e s,PLACEHOLDER_SYSMOCOM_RELEASE,$(SYSMOCOM_RELEASE),g \
				-e s,PLACEHOLDER_POKY_RELEASE,$(POKY_RELEASE),g > jenkins/$(file); \
	)


create-jenkins-jobs: create-jenkins-jobs-xml jenkins-cli.jar
ifndef JENKINS_HOST
	$(error "Please call with make $@ JENKINS_HOST=xyz...")
endif
ifndef JENKINS_USER
	$(error "Please call with make $@ JENKINS_USER=xyz...")
endif
ifndef JENKINS_PASS
	$(error "Please call with make $@ JENKINS_PASS=xyz...")
endif
	$(Q)$(foreach view, $(VIEW_FILES), \
		cat jenkins/$(view) | java -jar jenkins-cli.jar -s http://$(JENKINS_HOST) \
				create-view --username $(JENKINS_USER) --password $(JENKINS_PASS); \
	)
	$(Q)$(foreach job, $(JOB_FILES), \
		cat jenkins/$(job) | java -jar jenkins-cli.jar -s http://$(JENKINS_HOST) \
				create-job --username $(JENKINS_USER) --password $(JENKINS_PASS) $(JOB_NAME); \
	)

jenkins-cli.jar:
ifndef JENKINS_HOST
	$(error "Please call with make $@ JENKINS_HOST=xyz...")
endif
	wget http://$(JENKINS_HOST)/jnlpJars/jenkins-cli.jar


# Target classification
.PHONY: update setup-all install-ssh-config create-jenkins-jobs-xml create-jenkins-jobs
.SECONDARY: $(foreach repo, $(REPOS), git/$(repo)) $(foreach machine, $(MACHINES), build.$(machine))
