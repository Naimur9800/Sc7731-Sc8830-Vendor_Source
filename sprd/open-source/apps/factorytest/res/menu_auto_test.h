#ifndef MENUTITLE

#define __MAKE_MENUTITLE_ENUM__
#define MENUTITLE(num,id ,title, func)	A_PHONE_##title,
enum {
#endif
	MENUTITLE(0,1,TEST_LCD, test_lcd_start)
	MENUTITLE(1,2,TEST_TP, test_tp_start)
	MENUTITLE(2,5,TEST_VB_BL, test_vb_bl_start)
	MENUTITLE(3,4,TEST_KEY, test_key_start)
	MENUTITLE(4,7,TEST_FCAMERA, test_fcamera_start)
	MENUTITLE(5,8,TEST_BCAMERA, test_bcamera_start)
	MENUTITLE(6,10,TEST_MAINLOOP, test_mainloopback_start)
	MENUTITLE(7,11,TEST_ASSISLOOP, test_assisloopback_start)
	MENUTITLE(8,13,TEST_RECEIVER, test_receiver_start)
	MENUTITLE(9,17,TEST_CHARGE, test_charge_start)
	MENUTITLE(10,15,TEST_SDCARD, test_sdcard_start)
	MENUTITLE(11,16,TEST_SIMCARD, test_sim_start)
	MENUTITLE(12,14,TEST_HEADSET, test_headset_start)
	MENUTITLE(13,19,TEST_FM, test_fm_start)
	MENUTITLE(14,39,TEST_GSENSOR, test_gsensor_start)
	MENUTITLE(15,36,TEST_LSENSOR, test_lsensor_start)
	MENUTITLE(16,22,TEST_BT, test_bt_start)
	MENUTITLE(17,23,TEST_WIFI, test_wifi_start)
	MENUTITLE(18,24,TEST_GPS, test_gps_start)

#ifdef __MAKE_MENUTITLE_ENUM__
	K_MENU_AUTO_TEST_CNT,
};
#undef __MAKE_MENUTITLE_ENUM__
#undef MENUTITLE
#endif

