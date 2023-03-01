##############################################################
#
# ldd
#
##############################################################

# Fill up the contents below in order to reference your assignment 3 git contents
LDD_VERSION = 9000babccd1868aa52964189764957d2cd842a22

# Note: Be sure to reference the *ssh* repository URL here (not https) to work properly
# with ssh keys and the automated build/test system.
# Your site should start with git@github.com:
LDD_SITE = git@github.com:cu-ecen-aeld/assignment-7-LDAnderson.git
LDD_SITE_METHOD = git
LDD_GIT_SUBMODULES = YES


define LDD_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef

define LDD_INSTALL_TARGET_CMDS
$(INSTALL) -m 0755 $(@D)/* $(TARGET_DIR)/usr/bin
endef


$(eval $(generic-package))
