LOCAL_C_INCLUDES:=                      \
    bionic                              \
    bionic/libstdc++                    \
    external/stlport/stlport            \
    frameworks/av/media/libstagefright/include \
    vendor/widevine/proprietary/streamcontrol/include \
    vendor/widevine/proprietary/wvm/include

LOCAL_C_INCLUDES_x86 += $(TOP)/system/core/include/arch/linux-x86
