#ifndef __WCND_SM_H__
#define __WCND_SM_H__

#include "wcnd.h"


//WCND STATE
#define WCND_STATE_CP2_STARTED		0
#define WCND_STATE_CP2_STARTING		1
#define WCND_STATE_CP2_STOPPED		2
#define WCND_STATE_CP2_STOPPING		3
#define WCND_STATE_CP2_ASSERT		4


//WCND EVENT
#define WCND_EVENT_BT_OPEN			0x0001
#define WCND_EVENT_WIFI_OPEN			0x0002
#define WCND_EVENT_BT_CLOSE			0x0004
#define WCND_EVENT_WIFI_CLOSE			0x0008
#define WCND_EVENT_CP2_OK				0x0010
#define WCND_EVENT_CP2_ASSERT		0x0020
#define WCND_EVENT_CP2_DOWN			0x0040
#define WCND_EVENT_PENGING_EVENT		0x0080
#define WCND_EVENT_CP2POWERON_REQ	0x0100
#define WCND_EVENT_CP2POWEROFF_REQ	0x0200


//WCND BT/WIFI STATE
#define WCND_BTWIFI_STATE_BT_ON		0x01
#define WCND_BTWIFI_STATE_WIFI_ON	0x02


/**
* NOTE: the commnad send to wcn, will prefix with "wcn ", such as "wcn BT-CLOSE"
*/


//BT/WIFI CMD STRING
#define WCND_CMD_BT_CLOSE_STRING		"BT-CLOSE"
#define WCND_CMD_BT_OPEN_STRING		"BT-OPEN"
#define WCND_CMD_WIFI_CLOSE_STRING	"WIFI-CLOSE"
#define WCND_CMD_WIFI_OPEN_STRING	"WIFI-OPEN"

#define WCND_CMD_CP2_POWER_ON		"poweron"
#define WCND_CMD_CP2_POWER_OFF		"poweroff"

#define WCND_CMD_START_ENGPC 		"startengpc"
#define WCND_CMD_STOP_ENGPC   		"stopengpc"


#define WCND_CMD_CP2_DUMP_ON 		"dump_enable"
#define WCND_CMD_CP2_DUMP_OFF 		"dump_disable"
#define WCND_CMD_CP2_DUMPMEM		"dumpmem"
#define WCND_CMD_CP2_DUMPQUERY		"dump?"




//Below cmd used internal
#define WCND_SELF_CMD_START_CP2		"startcp2"
#define WCND_SELF_CMD_STOP_CP2		"stopcp2"
#define WCND_SELF_CMD_PENDINGEVENT	"pendingevent"
#define WCND_SELF_CMD_CONFIG_CP2 	"configcp2"
#define WCND_SELF_CMD_CP2_VERSION	"getcp2version"




#define WCND_SELF_EVENT_CP2_ASSERT	"cp2assert"


#define WCND_CMD_RESPONSE_STRING		"WCNBTWIFI-CMD"


//WCND CLIENT TYPE
#define WCND_CLIENT_TYPE_SLEEP			0x30
#define WCND_CLIENT_TYPE_NOTIFY			0x00
#define WCND_CLIENT_TYPE_CMD				0x10
#define WCND_CLIENT_TYPE_CMD_PENDING	0x20
#define WCND_CLIENT_TYPE_CMD_SUBTYPE_CLOSE		0x11
#define WCND_CLIENT_TYPE_CMD_SUBTYPE_OPEN		0x12

#define WCND_CLIENT_TYPE_CMD_MASK 0xF0

int wcnd_sm_init(WcndManager *pWcndManger);
int wcnd_sm_step(WcndManager *pWcndManger, WcndMessage *pMessage);


#endif
