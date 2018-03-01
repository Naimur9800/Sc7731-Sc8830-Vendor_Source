#ifndef MENUTITLE

#define __MAKE_MENUTITLE_ENUM__
#define MENUTITLE(num,id ,title, func)	D_PHONE_##title,
enum {
#endif
	MENUTITLE(0,27,TEST_TEL, test_tel_start)
	MENUTITLE(1,26,TEST_OTG, test_otg_start)

#ifdef __MAKE_MENUTITLE_ENUM__
	K_MENU_NOT_AUTO_TEST_CNT,
};
#undef __MAKE_MENUTITLE_ENUM__
#undef MENUTITLE
#endif

