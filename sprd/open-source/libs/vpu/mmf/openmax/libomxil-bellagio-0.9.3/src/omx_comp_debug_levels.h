/**
  src/omx_comp_debug_levels.h

  Define the level of debug prints on standard err. The different levels can
  be composed with binary OR.
  The debug levels defined here belong to OpenMAX components and IL core

  Copyright (C) 2007-2011 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/

#ifndef __OMX_COMP_DEBUG_LEVELS_H__
#define __OMX_COMP_DEBUG_LEVELS_H__

#include <stdio.h>
#ifdef ANDROID
#ifdef CNM_SPRD_PLATFORM
#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "OmxBellagio"
#endif //CNM_SPRD_PLATFORM
#include <utils/Log.h>
#endif
/** Remove all debug output lines
 */
#define DEB_LEV_NO_OUTPUT  0

/** Messages explaining the reason of critical errors
 */
#define DEB_LEV_ERR        1

/** Messages showing values related to the test and the component/s used
 */
#define DEB_LEV_PARAMS     2

/** Messages representing steps in the execution. These are the simple messages, because
 * they avoid iterations
 */
#define DEB_LEV_SIMPLE_SEQ 4

/** Messages representing steps in the execution. All the steps are described,
 * also with iterations. With this level of output the performances are
 * seriously compromised
 */
#define DEB_LEV_FULL_SEQ   8

/** Messages that indicates the beginning and the end of a function.
 * It can be used to trace the execution
 */
#define DEB_LEV_FUNCTION_NAME 16

/** Messages that are the default test application output. These message should be
 * shown every time
 */
#define DEFAULT_MESSAGES 32

/** All the messages - max value
 */
#define DEB_ALL_MESS   255

#define CONFIG_DEBUG_LEVEL DEB_LEV_SIMPLE_SEQ | DEB_LEV_PARAMS | DEB_LEV_ERR

#ifdef CONFIG_DEBUG_LEVEL
/** \def DEBUG_LEVEL is the current level do debug output on standard err */
#define DEBUG_LEVEL (DEB_LEV_ERR | CONFIG_DEBUG_LEVEL)
#else
#define DEBUG_LEVEL (DEB_LEV_ERR | DEB_ALL_MESS )
#endif

#if DEBUG_LEVEL > 0
#ifdef CNM_SPRD_PLATFORM
#define DEBUG(n, fmt, args...) \
do { \
    if (n == DEB_LEV_ERR) \
    { \
        ALOGE("[TID:%d-%s-%d] " fmt, gettid(), __func__, __LINE__, ##args); \
    } \
    else if (DEBUG_LEVEL & (n)) \
    { \
        ALOGI("[TID:%d-%s-%d] " fmt, gettid(), __func__, __LINE__, ##args); \
    } \
    else \
    { \
    } \
} while (0)
#else
#ifndef ANDROID
#undef LOG_NDEBUG 
#define LOG_NDEBUG 0 
#undef LOG_TAG
#define LOG_TAG "VPUOMX"
#ifdef ALOGE
#define DEBUG(n, fmt, args...) do { if (DEBUG_LEVEL & (n)){ALOGE("OMX-" fmt, ##args);} } while (0) // for android 4.1 later
#endif
#ifdef LOGE
#define DEBUG(n, fmt, args...) do { if (DEBUG_LEVEL & (n)){LOGE("OMX-" fmt, ##args);} } while (0)
#endif
#else
#define DEBUG(n, fmt, args...) do { if (DEBUG_LEVEL & (n)){fprintf(stderr, "OMX-" fmt, ##args);} } while (0)
#endif
#endif //CNM_SPRD_PLATFORM
#else
#define DEBUG(n, fmt, args...) {}  
#endif

#endif
