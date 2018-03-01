#ifndef APE_ALL_H
#define APE_ALL_H

/*****************************************************************************************
Global includes
*****************************************************************************************/

#ifdef WIN32
	#include <windows.h>
    //#include <mmsystem.h>
    //#include <tchar.h>
	#include <memory.h>
#else
//    #include <unistd.h> 
//    #include <time.h>
//    #include <sys/time.h>
//    #include <sys/types.h>
#define ARM9E
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define SPEEDOPT
/*****************************************************************************************
Global compiler settings (useful for porting)
*****************************************************************************************/

#define BACKWARDS_COMPATIBILITY

#define ENABLE_COMPRESSION_MODE_FAST
#define ENABLE_COMPRESSION_MODE_NORMAL
#define ENABLE_COMPRESSION_MODE_HIGH
#define ENABLE_COMPRESSION_MODE_EXTRA_HIGH

typedef unsigned short uint16;
typedef unsigned int uint32;
typedef short int16;
typedef int int32;


/*****************************************************************************************
Global defines
*****************************************************************************************/
#define MAC_VERSION_NUMBER                              3990
#define MAC_VERSION_STRING                              _T("3.99")
#define MAC_NAME                                        _T("Monkey's Audio 3.99")
#define PLUGIN_NAME                                     "Monkey's Audio Player v3.99"
#define MJ_PLUGIN_NAME                                  _T("APE Plugin (v3.99)")
#define CONSOLE_NAME                                    "--- Monkey's Audio Console Front End (v 3.99) (c) Matthew T. Ashland ---\n"
#define PLUGIN_ABOUT                                    _T("Monkey's Audio Player v3.99\nCopyrighted (c) 2000-2004 by Matthew T. Ashland")
#define MAC_DLL_INTERFACE_VERSION_NUMBER                1000

/*****************************************************************************************
Macros
*****************************************************************************************/

#define SAFE_DELETE(POINTER) if (POINTER) {  free(POINTER); POINTER = 0; }
#define SAFE_ARRAY_DELETE(POINTER) if (POINTER) {  free(POINTER); POINTER = 0; }

#define EXPAND_1_TIMES(CODE) CODE
#define EXPAND_2_TIMES(CODE) CODE CODE
#define EXPAND_3_TIMES(CODE) CODE CODE CODE
#define EXPAND_4_TIMES(CODE) CODE CODE CODE CODE
#define EXPAND_5_TIMES(CODE) CODE CODE CODE CODE CODE
#define EXPAND_6_TIMES(CODE) CODE CODE CODE CODE CODE CODE
#define EXPAND_7_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_8_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_9_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_12_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_14_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_15_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_16_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_30_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_31_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_32_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_64_TIMES(CODE) CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE CODE
#define EXPAND_N_TIMES(NUMBER, CODE) EXPAND_##NUMBER##_TIMES(CODE)

#define UNROLL_4_TIMES(MACRO) MACRO(0) MACRO(1) MACRO(2) MACRO(3)
#define UNROLL_8_TIMES(MACRO) MACRO(0) MACRO(1) MACRO(2) MACRO(3) MACRO(4) MACRO(5) MACRO(6) MACRO(7)
#define UNROLL_15_TIMES(MACRO) MACRO(0) MACRO(1) MACRO(2) MACRO(3) MACRO(4) MACRO(5) MACRO(6) MACRO(7) MACRO(8) MACRO(9) MACRO(10) MACRO(11) MACRO(12) MACRO(13) MACRO(14)
#define UNROLL_16_TIMES(MACRO) MACRO(0) MACRO(1) MACRO(2) MACRO(3) MACRO(4) MACRO(5) MACRO(6) MACRO(7) MACRO(8) MACRO(9) MACRO(10) MACRO(11) MACRO(12) MACRO(13) MACRO(14) MACRO(15)
#define UNROLL_64_TIMES(MACRO) MACRO(0) MACRO(1) MACRO(2) MACRO(3) MACRO(4) MACRO(5) MACRO(6) MACRO(7) MACRO(8) MACRO(9) MACRO(10) MACRO(11) MACRO(12) MACRO(13) MACRO(14) MACRO(15) MACRO(16) MACRO(17) MACRO(18) MACRO(19) MACRO(20) MACRO(21) MACRO(22) MACRO(23) MACRO(24) MACRO(25) MACRO(26) MACRO(27) MACRO(28) MACRO(29) MACRO(30) MACRO(31) MACRO(32) MACRO(33) MACRO(34) MACRO(35) MACRO(36) MACRO(37) MACRO(38) MACRO(39) MACRO(40) MACRO(41) MACRO(42) MACRO(43) MACRO(44) MACRO(45) MACRO(46) MACRO(47) MACRO(48) MACRO(49) MACRO(50) MACRO(51) MACRO(52) MACRO(53) MACRO(54) MACRO(55) MACRO(56) MACRO(57) MACRO(58) MACRO(59) MACRO(60) MACRO(61) MACRO(62) MACRO(63)
#define UNROLL_128_TIMES(MACRO) MACRO(0) MACRO(1) MACRO(2) MACRO(3) MACRO(4) MACRO(5) MACRO(6) MACRO(7) MACRO(8) MACRO(9) MACRO(10) MACRO(11) MACRO(12) MACRO(13) MACRO(14) MACRO(15) MACRO(16) MACRO(17) MACRO(18) MACRO(19) MACRO(20) MACRO(21) MACRO(22) MACRO(23) MACRO(24) MACRO(25) MACRO(26) MACRO(27) MACRO(28) MACRO(29) MACRO(30) MACRO(31) MACRO(32) MACRO(33) MACRO(34) MACRO(35) MACRO(36) MACRO(37) MACRO(38) MACRO(39) MACRO(40) MACRO(41) MACRO(42) MACRO(43) MACRO(44) MACRO(45) MACRO(46) MACRO(47) MACRO(48) MACRO(49) MACRO(50) MACRO(51) MACRO(52) MACRO(53) MACRO(54) MACRO(55) MACRO(56) MACRO(57) MACRO(58) MACRO(59) MACRO(60) MACRO(61) MACRO(62) MACRO(63) MACRO(64) MACRO(65) MACRO(66) MACRO(67) MACRO(68) MACRO(69) MACRO(70) MACRO(71) MACRO(72) MACRO(73) MACRO(74) MACRO(75) MACRO(76) MACRO(77) MACRO(78) MACRO(79) MACRO(80) MACRO(81) MACRO(82) MACRO(83) MACRO(84) MACRO(85) MACRO(86) MACRO(87) MACRO(88) MACRO(89) MACRO(90) MACRO(91) MACRO(92) MACRO(93) MACRO(94) MACRO(95) MACRO(96) MACRO(97) MACRO(98) MACRO(99) MACRO(100) MACRO(101) MACRO(102) MACRO(103) MACRO(104) MACRO(105) MACRO(106) MACRO(107) MACRO(108) MACRO(109) MACRO(110) MACRO(111) MACRO(112) MACRO(113) MACRO(114) MACRO(115) MACRO(116) MACRO(117) MACRO(118) MACRO(119) MACRO(120) MACRO(121) MACRO(122) MACRO(123) MACRO(124) MACRO(125) MACRO(126) MACRO(127)
#define UNROLL_256_TIMES(MACRO) MACRO(0) MACRO(1) MACRO(2) MACRO(3) MACRO(4) MACRO(5) MACRO(6) MACRO(7) MACRO(8) MACRO(9) MACRO(10) MACRO(11) MACRO(12) MACRO(13) MACRO(14) MACRO(15) MACRO(16) MACRO(17) MACRO(18) MACRO(19) MACRO(20) MACRO(21) MACRO(22) MACRO(23) MACRO(24) MACRO(25) MACRO(26) MACRO(27) MACRO(28) MACRO(29) MACRO(30) MACRO(31) MACRO(32) MACRO(33) MACRO(34) MACRO(35) MACRO(36) MACRO(37) MACRO(38) MACRO(39) MACRO(40) MACRO(41) MACRO(42) MACRO(43) MACRO(44) MACRO(45) MACRO(46) MACRO(47) MACRO(48) MACRO(49) MACRO(50) MACRO(51) MACRO(52) MACRO(53) MACRO(54) MACRO(55) MACRO(56) MACRO(57) MACRO(58) MACRO(59) MACRO(60) MACRO(61) MACRO(62) MACRO(63) MACRO(64) MACRO(65) MACRO(66) MACRO(67) MACRO(68) MACRO(69) MACRO(70) MACRO(71) MACRO(72) MACRO(73) MACRO(74) MACRO(75) MACRO(76) MACRO(77) MACRO(78) MACRO(79) MACRO(80) MACRO(81) MACRO(82) MACRO(83) MACRO(84) MACRO(85) MACRO(86) MACRO(87) MACRO(88) MACRO(89) MACRO(90) MACRO(91) MACRO(92) MACRO(93) MACRO(94) MACRO(95) MACRO(96) MACRO(97) MACRO(98) MACRO(99) MACRO(100) MACRO(101) MACRO(102) MACRO(103) MACRO(104) MACRO(105) MACRO(106) MACRO(107) MACRO(108) MACRO(109) MACRO(110) MACRO(111) MACRO(112) MACRO(113) MACRO(114) MACRO(115) MACRO(116) MACRO(117) MACRO(118) MACRO(119) MACRO(120) MACRO(121) MACRO(122) MACRO(123) MACRO(124) MACRO(125) MACRO(126) MACRO(127)    \
    MACRO(128) MACRO(129) MACRO(130) MACRO(131) MACRO(132) MACRO(133) MACRO(134) MACRO(135) MACRO(136) MACRO(137) MACRO(138) MACRO(139) MACRO(140) MACRO(141) MACRO(142) MACRO(143) MACRO(144) MACRO(145) MACRO(146) MACRO(147) MACRO(148) MACRO(149) MACRO(150) MACRO(151) MACRO(152) MACRO(153) MACRO(154) MACRO(155) MACRO(156) MACRO(157) MACRO(158) MACRO(159) MACRO(160) MACRO(161) MACRO(162) MACRO(163) MACRO(164) MACRO(165) MACRO(166) MACRO(167) MACRO(168) MACRO(169) MACRO(170) MACRO(171) MACRO(172) MACRO(173) MACRO(174) MACRO(175) MACRO(176) MACRO(177) MACRO(178) MACRO(179) MACRO(180) MACRO(181) MACRO(182) MACRO(183) MACRO(184) MACRO(185) MACRO(186) MACRO(187) MACRO(188) MACRO(189) MACRO(190) MACRO(191) MACRO(192) MACRO(193) MACRO(194) MACRO(195) MACRO(196) MACRO(197) MACRO(198) MACRO(199) MACRO(200) MACRO(201) MACRO(202) MACRO(203) MACRO(204) MACRO(205) MACRO(206) MACRO(207) MACRO(208) MACRO(209) MACRO(210) MACRO(211) MACRO(212) MACRO(213) MACRO(214) MACRO(215) MACRO(216) MACRO(217) MACRO(218) MACRO(219) MACRO(220) MACRO(221) MACRO(222) MACRO(223) MACRO(224) MACRO(225) MACRO(226) MACRO(227) MACRO(228) MACRO(229) MACRO(230) MACRO(231) MACRO(232) MACRO(233) MACRO(234) MACRO(235) MACRO(236) MACRO(237) MACRO(238) MACRO(239) MACRO(240) MACRO(241) MACRO(242) MACRO(243) MACRO(244) MACRO(245) MACRO(246) MACRO(247) MACRO(248) MACRO(249) MACRO(250) MACRO(251) MACRO(252) MACRO(253) MACRO(254) MACRO(255)

#ifdef ARM9E
#define ZeroMemory(POINTER, Length) memset(POINTER, 0, Length);
#endif
/*****************************************************************************************
Error Codes
*****************************************************************************************/

// success
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS                                   0
#endif

// file and i/o errors (1000's)
#define ERROR_IO_READ                                   1000
#define ERROR_IO_WRITE                                  1001

#define ERROR_INVALID_INPUT_FILE                        1002
#define ERROR_INVALID_OUTPUT_FILE                       1003
#define ERROR_INPUT_FILE_TOO_LARGE                      1004
#define ERROR_INPUT_FILE_UNSUPPORTED_BIT_DEPTH          1005
#define ERROR_INPUT_FILE_UNSUPPORTED_SAMPLE_RATE        1006
#define ERROR_INPUT_FILE_UNSUPPORTED_CHANNEL_COUNT      1007
#define ERROR_INPUT_FILE_TOO_SMALL                      1008
#define ERROR_UNSUPPORTED_FILE_TYPE                     1009
#define ERROR_UPSUPPORTED_FILE_VERSION                  1010

// memory errors (2000's)
#define ERROR_INSUFFICIENT_MEMORY                       2000

// dll errors (3000's)
#define ERROR_LOADINGAPE_DLL                            3000
#define ERROR_LOADINGAPE_INFO_DLL                       3001
#define ERROR_LOADING_UNMAC_DLL                         3002

// general and misc errors
#define ERROR_USER_STOPPED_PROCESSING                   4000
#define ERROR_SKIPPED                                   4001

// programmer errors
#define ERROR_BAD_PARAMETER                             5000

// IAPECompress errors
#define ERROR_APE_COMPRESS_TOO_MUCH_DATA                6000

// decoding errors
#define ERROR_INVALID_CHECKSUM                          7000
#define ERROR_DECOMPRESSING_FRAME                       7001
#define ERROR_INITIALIZING_UNMAC                        7002
#define ERROR_INVALID_FUNCTION_PARAMETER                7003
#define ERROR_TEMP_OUTBUF_UNDERFLOW		                7004
#define ERROR_TEMP_INBUF_UNDERFLOW		                7005

// unknown error
#define ERROR_UNDEFINED                                -1

#define ERROR_EXPLANATION \
    { ERROR_IO_READ                               , "I/O read error" },                         \
    { ERROR_IO_WRITE                              , "I/O write error" },                        \
    { ERROR_INVALID_INPUT_FILE                    , "invalid input file" },                     \
    { ERROR_INVALID_OUTPUT_FILE                   , "invalid output file" },                    \
    { ERROR_INPUT_FILE_TOO_LARGE                  , "input file file too large" },              \
    { ERROR_INPUT_FILE_UNSUPPORTED_BIT_DEPTH      , "input file unsupported bit depth" },       \
    { ERROR_INPUT_FILE_UNSUPPORTED_SAMPLE_RATE    , "input file unsupported sample rate" },     \
    { ERROR_INPUT_FILE_UNSUPPORTED_CHANNEL_COUNT  , "input file unsupported channel count" },   \
    { ERROR_INPUT_FILE_TOO_SMALL                  , "input file too small" },                   \
    { ERROR_INVALID_CHECKSUM                      , "invalid checksum" },                       \
    { ERROR_DECOMPRESSING_FRAME                   , "decompressing frame" },                    \
    { ERROR_INITIALIZING_UNMAC                    , "initializing unmac" },                     \
    { ERROR_INVALID_FUNCTION_PARAMETER            , "invalid function parameter" },             \
    { ERROR_UNSUPPORTED_FILE_TYPE                 , "unsupported file type" },                  \
    { ERROR_INSUFFICIENT_MEMORY                   , "insufficient memory" },                    \
    { ERROR_LOADINGAPE_DLL                        , "loading MAC.dll" },                        \
    { ERROR_LOADINGAPE_INFO_DLL                   , "loading MACinfo.dll" },                    \
    { ERROR_LOADING_UNMAC_DLL                     , "loading UnMAC.dll" },                      \
    { ERROR_USER_STOPPED_PROCESSING               , "user stopped processing" },                \
    { ERROR_SKIPPED                               , "skipped" },                                \
    { ERROR_BAD_PARAMETER                         , "bad parameter" },                          \
    { ERROR_APE_COMPRESS_TOO_MUCH_DATA            , "APE compress too much data" },             \
    { ERROR_UNDEFINED                             , "undefined" },                              \


/*****************************************************************************************
Defines
*****************************************************************************************/
#define COMPRESSION_LEVEL_FAST          1000
#define COMPRESSION_LEVEL_NORMAL        2000
#define COMPRESSION_LEVEL_HIGH          3000
#define COMPRESSION_LEVEL_EXTRA_HIGH    4000
#define COMPRESSION_LEVEL_INSANE        5000

#define MAC_FORMAT_FLAG_8_BIT                 1    // is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CRC                   2    // uses the new CRC32 error detection [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4    // uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_24_BIT                8    // is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16    // has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32    // create the wave header on decompression (not stored)

#define CREATE_WAV_HEADER_ON_DECOMPRESSION    -1
#define MAX_AUDIO_BYTES_UNKNOWN -1

#endif // #ifndef APE_ALL_H
