export GO_EASY_ON_ME=1

export TARGET_CXX = xcrun -sdk iphoneos clang++
export TARGET_LD = xcrun -sdk iphoneos clang++
export TARGET = iphone:latest:5.0
export ARCHS = armv7 # pending theos bug: armv7s

COUNTLY_TOKEN = 76599efdd227d40fdc92300bf2d607a0cb93fd23
COUNTLY_NAME = $(shell cat layout/DEBIAN/control | grep Name | cut -d ":" -f 2- | cut -c 2-)
COUNTLY_VERSION = $(shell cat layout/DEBIAN/control | grep Version | cut -d ":" -f 2- | cut -c 2-)
COUNTLY_IDENTIFIER = $(shell cat layout/DEBIAN/control | grep Package | cut -d ":" -f 2- | cut -c 2-)

include theos/makefiles/common.mk

TWEAK_NAME = Infinifolders
Infinifolders_FILES = Infinifolders.xmi ../countly/Countly.m
Infinifolders_FRAMEWORKS = UIKit CoreGraphics QuartzCore CoreTelephony
Infinifolders_CFLAGS = -DCountlyAppVersion=$(COUNTLY_VERSION) -DCountlyPrefix=$(COUNTLY_NAME) -DCountlyAppIdentifier=$(COUNTLY_IDENTIFIER) -DCountlyAppToken=$(COUNTLY_TOKEN)

include $(THEOS_MAKE_PATH)/tweak.mk

