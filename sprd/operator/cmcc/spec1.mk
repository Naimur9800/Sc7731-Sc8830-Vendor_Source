# Spreadtrum
# Currently, specs are different with their build packages.

# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
    QQBrowser_CMCC \
    Weibo_CMCC \
    SohuNew_CMCC \
    QQBuy_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
    CMCCGameHall \
    CMCCMobileMusic \
    CMCCFetion

include vendor/sprd/operator/cmcc/spec_common.mk

PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator.version=spec1

