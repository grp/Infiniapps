export GO_EASY_ON_ME=1
export TARGET_IPHONEOS_DEPLOYMENT_VERSION=3.0
export ARCHS=armv7

TWEAK_NAME = Infinidock
OPTFLAG = -O1
Infinidock_OBJCC_FILES = Base.xm Wildcat.xm Three.xm Four.xm infinishared/Infinishared.xmi
Infinidock_FRAMEWORKS = UIKit CoreGraphics QuartzCore

include theos/makefiles/common.mk

ifeq ($(FW_TARGET_NAME),iphone)
Infinidock_LDFLAGS = -llockdown
endif

include theos/makefiles/tweak.mk
