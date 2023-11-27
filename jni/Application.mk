APP_PROJECT_PATH    := $(call my-dir)
#NDK_PROJECT_PATH = ./

APP_STL := c++_static
APP_BUILD_SCRIPT    := ./Android.mk
APP_ALLOW_MISSING_DEPS=true
APP_PLATFORM := android-21
APP_ABI      := armeabi-v7a

NDK_MODULE_PATH		:= $(NDK_MODULE_PATH):$(call my-dir)
