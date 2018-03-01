
LOCAL_PATH:= vendor/sprd/open-source/res/productinfo

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/scx35l_sp9830iea_5m_connectivity_configure.ini:system/etc/connectivity_configure.ini \
	$(LOCAL_PATH)/scx35l_sp9830iea_5m_connectivity_calibration.ini:system/etc/connectivity_calibration.ini
