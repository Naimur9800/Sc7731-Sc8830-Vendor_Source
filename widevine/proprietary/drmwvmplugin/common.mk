LOCAL_C_INCLUDES:= \
    $(TOP)/bionic \
    $(TOP)/bionic/libstdc++/include \
    $(TOP)/external/stlport/stlport \
    $(TOP)/vendor/widevine/proprietary/streamcontrol/include \
    $(TOP)/vendor/widevine/proprietary/drmwvmplugin/include \
    $(TOP)/frameworks/av/drm/libdrmframework/include \
    $(TOP)/frameworks/av/drm/libdrmframework/plugins/common/include \
    $(TOP)/frameworks/av/include

LOCAL_C_INCLUDES_x86 += $(TOP)/system/core/include/arch/linux-x86
