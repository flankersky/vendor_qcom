ifeq ($(call is-board-platform-in-list,$(QSD8K_BOARD_PLATFORMS)),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libmm-qcelp13-enc-def := -g -O3
libmm-qcelp13-enc-def += -DQC_MODIFIED
libmm-qcelp13-enc-def += -D_ANDROID_
libmm-qcelp13-enc-def += -D_DEBUG

# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxQcelp13Dec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libmm-qcelp13-enc-inc	:= $(LOCAL_PATH)/inc
libmm-qcelp13-enc-inc   += $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_MODULE            := libOmxQcelp13Enc
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libmm-qcelp13-enc-def)
LOCAL_C_INCLUDES        := $(libmm-qcelp13-enc-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := src/aenc_svr.c
LOCAL_SRC_FILES         += src/omx_qcelp13_aenc.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-aenc-omxqcelp13-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-qcelp13-enc-test-inc := $(LOCAL_PATH)/test
mm-qcelp13-enc-test-inc += $(LOCAL_PATH)/inc
mm-qcelp13-enc-test-inc += $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_MODULE            := mm-aenc-omxqcelp13-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libmm-qcelp13-enc-def)
LOCAL_C_INCLUDES        := $(mm-qcelp13-enc-test-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxQcelp13Enc

LOCAL_SRC_FILES         := test/omx_qcelp13_enc_test.c

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID
endif # is-board-platform-in-list
# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------
