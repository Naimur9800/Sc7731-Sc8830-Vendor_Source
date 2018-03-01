# Spreadtrum
# Currently, specs are different with their build packages.

include vendor/sprd/operator/cmcc/spec_common.mk

# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
    QQBrowser_CMCC \
    QQLive_CMCC \
    ChinaShici_CMCC \
    Hao123_CMCC \
    IReader_CMCC \
    IFlyIME_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
    AndContacts_CMCC \
    AndLife_CMCC \
    AndVideo_CMCC \
    CMCCFetion \
    CMCCGameHall \
    CMCCNavigation \
    AndCloud_CMCC

# Add the vendor(CMCC) apks into build list
ifneq ($(strip $(TARGET_DISABLE_VENDOR_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_VENDOR_PACKAGES)
endif

# Add the 3rd-party apks into build list
ifneq ($(strip $(TARGET_DISABLE_THIRDPATY_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_THIRDPARTY_PACKAGES)
endif

PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator.version=spec3

