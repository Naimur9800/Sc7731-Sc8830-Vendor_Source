
ifeq ($(PLATFORM_VERSION),4.4.4)
major := $(word 1, $(subst ., , $(PLATFORM_VERSION)))
minor := $(word 2, $(subst ., , $(PLATFORM_VERSION)))

PRODUCT_COPY_FILES += \
	vendor/sprd/wcn/gps/gnss/android44/gps.default.so:/system/lib/hw/gps.default.so \
	vendor/sprd/wcn/gps/gnss/android44/libsupl.so:/system/lib/libsupl.so \
	vendor/sprd/wcn/gps/gnss/android44/liblcsagent.so:/system/lib/liblcsagent.so \
	vendor/sprd/wcn/gps/gnss/android44/liblcscp.so:/system/lib/liblcscp.so \
	vendor/sprd/wcn/gps/gnss/android44/liblte.so:/system/lib/liblte.so \
	vendor/sprd/wcn/gps/gnss/android44/supl.xml:/system/etc/supl.xml   \
	vendor/sprd/wcn/gps/gnss/android44/config.xml:/system/etc/config.xml \
	vendor/sprd/wcn/gps/gnss/android44/jpleph.405:/system/etc/jpleph.405 \
	vendor/sprd/wcn/gps/gnss/android44/raw.obs:/system/etc/raw.obs
else
ifneq ($(filter scx35l64_sp9838aea_5mod scx35l64_sp9836aea_4mod scx35l64_sp9836aea_3mod, $(TARGET_BOARD)),)
PRODUCT_COPY_FILES += \
	vendor/sprd/wcn/gps/gnss/android51_64/gps.default.so:/system/lib64/hw/gps.default.so \
	vendor/sprd/wcn/gps/gnss/android51_64/libsupl.so:/system/lib64/libsupl.so \
	vendor/sprd/wcn/gps/gnss/android51_64/liblcsagent.so:/system/lib64/liblcsagent.so \
	vendor/sprd/wcn/gps/gnss/android51_64/liblcscp.so:/system/lib64/liblcscp.so \
	vendor/sprd/wcn/gps/gnss/android51_64/liblte.so:/system/lib64/liblte.so \
	vendor/sprd/wcn/gps/gnss/android51_32/gps.default.so:/system/lib/hw/gps.default.so \
	vendor/sprd/wcn/gps/gnss/android51_32/libsupl.so:/system/lib/libsupl.so \
	vendor/sprd/wcn/gps/gnss/android51_32/liblcsagent.so:/system/lib/liblcsagent.so \
	vendor/sprd/wcn/gps/gnss/android51_32/liblcscp.so:/system/lib/liblcscp.so \
	vendor/sprd/wcn/gps/gnss/android51_32/liblte.so:/system/lib/liblte.so \
	vendor/sprd/wcn/gps/gnss/android51_32/supl.xml:/system/etc/supl.xml   \
	vendor/sprd/wcn/gps/gnss/android51_32/config.xml:/system/etc/config.xml \
	vendor/sprd/wcn/gps/gnss/android51_32/jpleph.405:/system/etc/jpleph.405 \
	vendor/sprd/wcn/gps/gnss/android51_32/gnssmodem.bin:/system/etc/gnssmodem.bin \
	vendor/sprd/wcn/gps/gnss/android51_32/raw.obs:/system/etc/raw.obs
else
PRODUCT_COPY_FILES += \
	vendor/sprd/wcn/gps/gnss/android51_32/gps.default.so:/system/lib/hw/gps.default.so \
	vendor/sprd/wcn/gps/gnss/android51_32/libsupl.so:/system/lib/libsupl.so \
	vendor/sprd/wcn/gps/gnss/android51_32/liblcsagent.so:/system/lib/liblcsagent.so \
	vendor/sprd/wcn/gps/gnss/android51_32/liblcscp.so:/system/lib/liblcscp.so \
	vendor/sprd/wcn/gps/gnss/android51_32/liblte.so:/system/lib/liblte.so \
	vendor/sprd/wcn/gps/gnss/android51_32/supl.xml:/system/etc/supl.xml   \
	vendor/sprd/wcn/gps/gnss/android51_32/config.xml:/system/etc/config.xml \
	vendor/sprd/wcn/gps/gnss/android51_32/jpleph.405:/system/etc/jpleph.405 \
	vendor/sprd/wcn/gps/gnss/android51_32/gnssmodem.bin:/system/etc/gnssmodem.bin \
	vendor/sprd/wcn/gps/gnss/android51_32/raw.obs:/system/etc/raw.obs
endif
endif


