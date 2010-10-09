GO_EASY_ON_ME=1
TWEAK_NAME = Infinidock
Infinidock_OBJCC_FILES = Base.mm Wildcat.mm Three.mm Four.mm infinishared/Infinishared.mm
Infinidock_FRAMEWORKS = UIKit CoreGraphics QuartzCore


include framework/makefiles/common.mk

ifeq ($(FW_TARGET_NAME),iphone)
Infinidock_LDFLAGS = -llockdown
endif

include framework/makefiles/tweak.mk
