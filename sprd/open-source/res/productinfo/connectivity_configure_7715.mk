
LOCAL_PATH:= vendor/sprd/open-source/res/productinfo

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/scx15_sp7715_connectivity_configure_hw100.ini:system/etc/connectivity_configure_hw100.ini \
	$(LOCAL_PATH)/scx15_sp7715_connectivity_configure_hw102.ini:system/etc/connectivity_configure_hw102.ini \
	$(LOCAL_PATH)/scx15_sp7715_connectivity_configure_hw104.ini:system/etc/connectivity_configure_hw104.ini
