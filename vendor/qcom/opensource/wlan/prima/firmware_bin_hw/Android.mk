# Android makefile for the WLAN WCNSS/TOOLS
ifeq ($(CONFIG_HUAWEI_ANDROID),true)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_mla_al00.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_mla_al00.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)

LOCAL_MODULE := WCNSS_hw_wlan_nv_mla_cl00.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_mla_cl00.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_mla_tl00.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_mla_tl00.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_mla_l01.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_mla_l01.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_mla_l02.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_mla_l02.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_mla_l03.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_mla_l03.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_can_al00.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_can_al00.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_can_cl00.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_can_cl00.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_can_tl00.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_can_tl00.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_can_l01.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_can_l01.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_can_l02.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_can_l02.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_can_l03.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_can_l03.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_trt_tl10.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_trt_tl10.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_trt_al00.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_trt_al00.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_trt_l21.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_trt_l21.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_can_al00_no_fem.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_can_al00_no_fem.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_trt_l22.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_trt_l22.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_trt_l23.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_trt_l23.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_kob_l09.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_kob_l09.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_kob_w09c.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_kob_w09c.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_trt_l53.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_trt_l53.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_ags_l09.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_ags_l09.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_ags_l09hn.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_ags_l09hn.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_ags_l03.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_ags_l03.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_ags_w09.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_ags_w09.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_ags_w09hn.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_ags_w09hn.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_bah_l09.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_bah_l09.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_bah_al00.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_bah_al00.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_bah_w09.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_bah_w09.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WCNSS_hw_wlan_nv_trt_a1.bin
LOCAL_SRC_FILES := WCNSS_hw_wlan_nv_trt_a1.bin
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_ODM_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := wifi
include $(BUILD_PREBUILT)

endif
