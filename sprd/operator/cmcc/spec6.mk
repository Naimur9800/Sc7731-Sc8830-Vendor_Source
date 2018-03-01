# Spreadtrum
# Currently, specs are different with their build packages.

include vendor/sprd/operator/cmcc/spec_common.mk

# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
    QQLive_CMCC \
    BaiduSearch_CMCC \
    TMall_CMCC \
    Opera_CMCC \
    QQMusic_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
    IFlyIME_CMCC \
    CMCCNavigation \
    CMCCGameHall \
    AndContacts_CMCC

# Add the vendor(CMCC) apks into build list
ifneq ($(strip $(TARGET_DISABLE_VENDOR_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_VENDOR_PACKAGES)
endif

# Add the 3rd-party apks into build list
ifneq ($(strip $(TARGET_DISABLE_THIRDPATY_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_THIRDPARTY_PACKAGES)
endif

PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator.version=spec6

