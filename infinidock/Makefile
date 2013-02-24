export GO_EASY_ON_ME=1

export TARGET_CXX = xcrun -sdk iphoneos clang++
export TARGET_LD = xcrun -sdk iphoneos clang++
export TARGET = iphone:latest:5.0
export ARCHS = armv7 # pending theos bug: armv7s

include theos/makefiles/common.mk

TWEAK_NAME = Infinidock
Infinidock_FILES = Infinidock.xmi
Infinidock_FRAMEWORKS = UIKit CoreGraphics QuartzCore CoreTelephony

include $(THEOS_MAKE_PATH)/tweak.mk

