GO_EASY_ON_ME=1
TWEAK_NAME = Infinidock
OPTFLAG = -O1
Infinidock_OBJCC_FILES = Base.xmi Wildcat.xmi Three.xmi Four.xmi infinishared/Infinishared.xmi
Infinidock_FRAMEWORKS = UIKit CoreGraphics QuartzCore

include framework/makefiles/common.mk

ifeq ($(FW_TARGET_NAME),iphone)
Infinidock_LDFLAGS = -llockdown
endif

include framework/makefiles/tweak.mk
