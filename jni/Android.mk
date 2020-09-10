LOCAL_PATH := $(call my-dir)

ROOT_DIR   := $(LOCAL_PATH)/..
MELON_DIR  := $(ROOT_DIR)/src
CORE_DIR   := $(MELON_DIR)/libretro

include $(ROOT_DIR)/Makefile.common

CORE_FLAGS := -D__LIBRETRO__ $(INCFLAGS)

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C) $(SOURCES_CXX)
LOCAL_CFLAGS    := $(CORE_FLAGS)
LOCAL_CPPFLAGS  := -std=c++11 $(CORE_FLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)
