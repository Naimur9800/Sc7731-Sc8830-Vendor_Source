#!/bin/sh

if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "Android build environment not set"
    exit -1
fi

. $ANDROID_BUILD_TOP/build/envsetup.sh

cd $ANDROID_BUILD_TOP/external/gtest/
pwd
mm

cd $ANDROID_BUILD_TOP/vendor/widevine/libwvdrmengine
pwd
mm

cd $ANDROID_BUILD_TOP/vendor/widevine/libwvdrmengine/test/gmock
pwd
mm

cd $ANDROID_BUILD_TOP/vendor/widevine/libwvdrmengine/cdm/test
pwd
mm

cd $ANDROID_BUILD_TOP/vendor/widevine/libwvdrmengine/mediacrypto/test
pwd
mm

cd $ANDROID_BUILD_TOP/vendor/widevine/libwvdrmengine/mediadrm/test
pwd
mm

cd $ANDROID_BUILD_TOP/vendor/widevine/libwvdrmengine/oemcrypto/test
pwd
mm

cd $ANDROID_BUILD_TOP/vendor/widevine/libwvdrmengine/test/unit
pwd
mm

cd $ANDROID_BUILD_TOP/vendor/widevine/libwvdrmengine/test/java/src/com/widevine/test
pwd
mm

echo "waiting for device"
adb root && adb wait-for-device remount


adb push $OUT/system/bin/oemcrypto_test /system/bin
adb push $OUT/system/bin/request_license_test /system/bin
adb push $OUT/system/bin/cdm_extended_duration_test /system/bin
adb push $OUT/system/bin/max_res_engine_unittest /system/bin
adb push $OUT/system/bin/policy_engine_unittest /system/bin
adb push $OUT/system/bin/libwvdrmmediacrypto_test /system/bin
adb push $OUT/system/bin/libwvdrmdrmplugin_test /system/bin
adb push $OUT/system/bin/cdm_engine_test /system/bin
adb push $OUT/system/bin/cdm_session_unittest /system/bin
adb push $OUT/system/bin/file_store_unittest /system/bin
adb push $OUT/system/bin/device_files_unittest /system/bin
adb push $OUT/system/bin/timer_unittest /system/bin
adb push $OUT/system/bin/libwvdrmengine_test /system/bin
if [ -r $OUT/system/app/MediaDrmAPITest.apk ]; then
    adb install -r $OUT/system/app/MediaDrmAPITest.apk
else
    adb install -r $OUT/system/app/MediaDrmAPITest/MediaDrmAPITest.apk
fi

cd $ANDROID_BUILD_TOP/vendor/widevine/libwvdrmengine
./run_all_unit_tests.sh
