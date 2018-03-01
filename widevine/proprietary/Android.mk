# widevine prebuilts only available for ARM
# To build this dir you must define BOARD_WIDEVINE_OEMCRYPTO_LEVEL in the board config.
ifdef BOARD_WIDEVINE_OEMCRYPTO_LEVEL

include $(call all-subdir-makefiles)

endif # BOARD_WIDEVINE_OEMCRYPTO_LEVEL
