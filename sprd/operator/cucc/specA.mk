# CUCC SpecA
# $(call inherit-product, vendor/sprd/operator/cucc/specA.mk)
# CUCC Prebuild packages
# Attention the packages name is treat as LOCAL_MODULE when building,
# the items should be unique.
PRODUCT_PACKAGES += \
	SohuNewsClient_CUCC \
	Weibo_CUCC \
	BaiduMap_CUCC \
	SogouInput_CUCC \
	Wo116114 \
	UnicomClient \
	WoStore \
	WPSOffice_CUCC \
	Wo3G \
	SystemHelper_Debug

#	sprdcudm

#   	wostore.apk \
#   Unicomclient_SP7710-4.1.apk \
#	wo116114.apk \
#	SogouInput_cu.apk \
# 	AIMail_Android_V375a_orange.apk \
#	Wo3G \
#	WoReader

DEVICE_PACKAGE_OVERLAYS += \
    vendor/sprd/operator/cucc/specA/overlay

PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator=cucc \
	ro.operator.version=specA

$(call inherit-product, vendor/sprd/operator/cucc/specA/res/boot/boot_res.mk)

