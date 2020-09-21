//Copyright 2015, 2018 <>< Charles Lohr, Adam Feinstein see LICENSE file.

/*==============================================================================
 * Includes
 *============================================================================*/

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "espconn.h"
#include "esp82xxutil.h"
#include "commonservices.h"
#include "vars.h"
#include <mdns.h>

/*==============================================================================
 * Process Defines
 *============================================================================*/

#define procTaskPrio        0
#define procTaskQueueLen    1
os_event_t    procTaskQueue[procTaskQueueLen];

/*==============================================================================
 * Variables
 *============================================================================*/

static os_timer_t some_timer;
static struct espconn *pUdpServer;
usr_conf_t * UsrCfg = (usr_conf_t*)(SETTINGS.UserData);

/*==============================================================================
 * Functions
 *============================================================================*/


extern uint8_t dmxsend[1024];
extern int dmxframesize;
extern int senddmxframe;
int senddmxfpl;

static void ICACHE_FLASH_ATTR procTask(os_event_t *events)
{
	CSTick( 0 );

	if( senddmxfpl )
	{
		if( senddmxfpl >= dmxframesize )
		{
			ets_delay_us( 125 );
			senddmxfpl = 0;
		}
		else
		{
			uart_tx_one_char(UART0, dmxsend[senddmxfpl] );
			senddmxfpl++;
		}
	}
	else
	{
		if( senddmxframe )
		{
			senddmxframe = 0;
			WRITE_PERI_REG(UART_CONF0(0), ((0 & UART_PARITY_EN_M)  <<  UART_PARITY_EN_S) //SET BIT AND PARITY MODE
		                                                                    | ((0 & UART_PARITY_M)  <<UART_PARITY_S )
		                                                                    | ((3 & UART_STOP_BIT_NUM) << UART_STOP_BIT_NUM_S)
		                                                                    | ((3 & UART_BIT_NUM) << UART_BIT_NUM_S));
		
			//UART_SetBaudrate(UART0, 250000);
			UART_SetBaudrate(UART0, 500000);

			PIN_FUNC_SELECT( PERIPHS_IO_MUX_U0TXD_U, 3); //Set to GPIO.  
			GPIO_OUTPUT_SET(GPIO_ID_PIN(1), 0 );
			senddmxfpl = 1;
			ets_delay_us( 125 );
			GPIO_OUTPUT_SET(GPIO_ID_PIN(1), 1 ); 
			PIN_FUNC_SELECT( PERIPHS_IO_MUX_U0TXD_U, 0);
			ets_delay_us( 20 );
			uart_tx_one_char(UART0, 0 );
			uart_tx_one_char(UART0, dmxsend[0] );
		}
	}

	// Post the task in order to have it called again
	system_os_post(procTaskPrio, 0, 0 );
}

/**
 * This is a timer set up in user_main() which is called every 100ms, forever
 * @param arg unused
 */
static void ICACHE_FLASH_ATTR timer100ms(void *arg)
{
	CSTick( 1 ); // Send a one to uart
}

/**
 * This callback is registered with espconn_regist_recvcb and is called whenever
 * a UDP packet is received
 *
 * @param arg pointer corresponding structure espconn. This pointer may be
 *            different in different callbacks, please don’t use this pointer
 *            directly to distinguish one from another in multiple connections,
 *            use remote_ip and remote_port in espconn instead.
 * @param pusrdata received data entry parameters
 * @param len      received data length
 */
static void ICACHE_FLASH_ATTR udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
	struct espconn *pespconn = (struct espconn *)arg;

	int lencp = len-3;
	if( lencp > sizeof(dmxsend) ) lencp = sizeof(dmxsend);
	ets_memcpy( dmxsend, pusrdata+3, lencp );
	senddmxframe = 1;
	dmxframesize = lencp;
}

/**
 * UART RX handler, called by the uart task. Currently does nothing
 *
 * @param c The char received on the UART
 */
void ICACHE_FLASH_ATTR charrx( uint8_t c )
{
	//Called from UART.
}

/**
 * This is called on boot for versions ESP8266_NONOS_SDK_v1.5.2 to
 * ESP8266_NONOS_SDK_v2.2.1. system_phy_set_rfoption() may be called here
 */
void user_rf_pre_init(void)
{
	; // nothing
}


void ICACHE_FLASH_ATTR user_pre_init(void)
{
	LoadDefaultPartitionMap(); //You must load the partition table so the NONOS SDK can find stuff.
}

/**
 * The default method, equivalent to main() in other environments. Handles all
 * initialization
 */
void ICACHE_FLASH_ATTR user_init(void)
{
	// Initialize the UART
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	os_printf("\r\nesp8266_dmx_explorer Web-GUI\r\n%s\b", VERSSTR);

	//Uncomment this to force a system restore.
	//	system_restore();

	// Load settings and pre-initialize common services
	CSSettingsLoad( 0 );
	CSPreInit();

	// Start a UDP Server
    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	espconn_create( pUdpServer );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = COM_PORT;
	espconn_regist_recvcb(pUdpServer, udpserver_recv);

	if( espconn_create( pUdpServer ) )
	{
		while(1) { uart0_sendStr( "\r\nFAULT\r\n" ); }
	}

	// Initialize common settings
	CSInit( 1 );

	// Start MDNS services
	SetServiceName( "esp8266dmx" );
	AddMDNSName(    "esp82xx" );
	AddMDNSName(    "esp8266dmx" );
	AddMDNSService( "_http._tcp",    "An ESP82XX Webserver", WEB_PORT );
	AddMDNSService( "_esp8266dmx._udp",  "ESP8266 Comunication", COM_PORT );
	AddMDNSService( "_esp82xx._udp", "ESP82XX Backend",      BACKEND_PORT );

	// Set timer100ms to be called every 100ms
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)timer100ms, NULL);
	os_timer_arm(&some_timer, 100, 1);

	os_printf( "Boot Ok.\n" );

	// Set the wifi sleep type
	wifi_set_sleep_type(LIGHT_SLEEP_T);
	wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);

	// Add a process and start it
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);
	system_os_post(procTaskPrio, 0, 0 );
}

/**
 * This will be called to disable any interrupts should the firmware enter a
 * section with critical timing. There is no code in this project that will
 * cause reboots if interrupts are disabled.
 */
void ICACHE_FLASH_ATTR EnterCritical(void)
{
	;
}

/**
 * This will be called to enable any interrupts after the firmware exits a
 * section with critical timing.
 */
void ICACHE_FLASH_ATTR ExitCritical(void)
{
	;
}
