LOCAL_PATH := $(call my-dir)

ROOT_DIR   := $(LOCAL_PATH)/..
MELON_DIR  := $(ROOT_DIR)/src
CORE_DIR   := $(MELON_DIR)/libretro
JIT_ARCH   :=

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
  JIT_ARCH := aarch64
else ifeq ($(TARGET_ARCH_ABI),x86_64)
  JIT_ARCH := x64
endif

include $(ROOT_DIR)/Makefile.common

CORE_FLAGS := -D__LIBRETRO__ $(INCFLAGS) $(DEFINES)

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C) $(SOURCES_CXX) $(SOURCES_S)
LOCAL_CFLAGS    := $(CORE_FLAGS)
LOCAL_CPPFLAGS  := -std=c++11 $(CORE_FLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)
