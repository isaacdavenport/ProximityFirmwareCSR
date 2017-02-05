/******************************************************************************
T2L  Toys to Life.  This program sets the CSR Radio to listen for other radios
  then turn around and broadcast RSSI values for radios it heard advertising.
  This will give a map of which radios are closer to which radios that can 
  update in real time.
 *****************************************************************************/

#include <main.h>
#include <mem.h>
#include <gatt.h>
#include <gatt_prim.h>
#include <gatt_uuid.h>
#include <ls_app_if.h>
#include <gap_app_if.h>
#include <config_store.h>
#include <timer.h>
#include "app_common.h"
#include "app_debug.h"
#include "gap_conn_params.h"
#include <bt_event_types.h>
#include <pio.h> 

#define CHARACTER_NUM 5

#define BEACON_ADVERT_SIZE (16) /* use PACKET_SIZE now */
#define TX_BUFF_SIZE 16
#define PACKET_SIZE 9
#define MY_SPOT_MARK_FF (0xFF)
static uint16 uartRxDataCallback(void   *p_rx_buffer, uint16  length, uint16 *p_additional_req_data_length);

#define MAX_APP_TIMERS       2
static uint16 app_timers[ SIZEOF_APP_TIMER * MAX_APP_TIMERS ];

static void InitHeatMap(void);
static void HandleTimer(timer_id tid);
void SetRadioToListen(void);
void SetRadioToAdvertise(void);
void SetRadioToIdle(void);
void ProcessPacket(LM_EVENT_T *event_data);
bool CopyArrayCheckIfDuplicate(uint8 *source, uint8 *dest, uint8 length);
void SendSerialDebugVars(void);

timer_id broadcast_tid;
uint8 advData[PACKET_SIZE];
uint8 priorAdvData[PACKET_SIZE];
static uint8 autoIncrement = 0xFE;
static uint16 processedPacketCalls;
static uint16 processedPacketSuccess;
static uint16 handleTimerCalls;
static uint16 setRadioToAdvertiseCalls;
static uint16 setRadioToListenCalls;
static uint16 appProcessLmEventCalls;

void AppPowerOnReset(void)
{
}

void AppProcessSystemEvent(sys_event_id id, void *data)
{
}

static uint16 uartRxDataCallback(void *p_rx_buffer, uint16 length, uint16 *p_additional_req_data_length)
{   /* Inform the UART that we'd like another byte when it becomes available */
    *p_additional_req_data_length = 1;    
    /* Return the number of bytes that have been processed */
    return length;
}

bool CopyArrayCheckIfDuplicate(uint8 *source, uint8 *dest, uint8 length) {
    uint8 l = 0;
    bool isDupe = TRUE;
    
    for (l = 0; l < length; l++)
    {
        if(source[l] != dest[l])
        {
            isDupe = FALSE;
            dest[l] = source[l];
        }
    }
    return isDupe; 
}

static void InitHeatMap(void)
{
    /* clear the existing advertisement data */
    LsStoreAdvScanData(0, NULL, ad_src_advertise);

    /* store the manufacturing data */
    advData[0] = AD_TYPE_MANUF;  /* 0xFF meaning the data payload is manufacturer specific */
    advData[1] = 0x00; 
    advData[2] = 0x00;     
    advData[3] = 0x00;  /* make all these zero inits meaning not recently seen */
    advData[4] = 0x00;  
    advData[5] = 0x00;  
    advData[6] = 0x00;  
    advData[7] = 0xA5;  /* no character 7 now, validation byte  */
    advData[8] = 0x20;  /* rolling packet tag number */
    
    advData[CHARACTER_NUM + 1] = MY_SPOT_MARK_FF;  /* fill this characters RSSI to FF to demark which character transmited */

    priorAdvData[0] = AD_TYPE_MANUF;  /* initialize to not be equal to force initial processing */
    priorAdvData[1] = 0x10; 
    priorAdvData[2] = 0x10;     
    priorAdvData[3] = 0x10;  
    priorAdvData[4] = 0x10;  
    priorAdvData[5] = 0x10;  
    priorAdvData[6] = 0x10;  
    priorAdvData[7] = 0xA5;  
    priorAdvData[8] = 0x1F;  /* rolling packet tag number */
    
    priorAdvData[CHARACTER_NUM + 1] = MY_SPOT_MARK_FF;  /* fill this characters RSSI to FF to demark which character transmited */
    
    LsStoreAdvScanData(PACKET_SIZE, advData, ad_src_advertise);
}

void SendSerialDebugVars() {
    DebugWriteUint16(processedPacketCalls);
    DebugWriteUint16(processedPacketSuccess);
    DebugWriteUint16(handleTimerCalls);
    DebugWriteUint16(setRadioToAdvertiseCalls);
    DebugWriteUint16(setRadioToListenCalls);
    DebugWriteUint16(appProcessLmEventCalls);              
    AppDebugWriteString("\r\n");
}

static void HandleTimer(timer_id tid)
{
    static uint8 state;
    static uint8 ledBlinkCount;
    /* step through listen, transmit, and low power states */
    /* TODO why does it advertise for more than 7ms? */
    handleTimerCalls++;
    if (tid == broadcast_tid) /* warning if variable unused only one tid */
    {
        switch (state) {
           case 1 :
              broadcast_tid = TimerCreate(43*MILLISECOND, TRUE, HandleTimer); 
              SetRadioToIdle();
              break;
           case 2 :
              broadcast_tid = TimerCreate(7*MILLISECOND, TRUE, HandleTimer); 
              SetRadioToAdvertise();
              break;
           case 3 :
              broadcast_tid = TimerCreate(43*MILLISECOND, TRUE, HandleTimer); 
              SetRadioToIdle();
              break;
           case 4 :
              broadcast_tid = TimerCreate(7*MILLISECOND, TRUE, HandleTimer); 
              SetRadioToAdvertise();
              break;
           case 5 :
              broadcast_tid = TimerCreate(43*MILLISECOND, TRUE, HandleTimer); 
              SetRadioToIdle();
              break;
           case 6 :
              broadcast_tid = TimerCreate(7*MILLISECOND, TRUE, HandleTimer); 
              SetRadioToAdvertise();
              break;
           case 7 :
              broadcast_tid = TimerCreate(50*MILLISECOND, TRUE, HandleTimer); 
              SetRadioToListen();
              break;
           case 8 :
              broadcast_tid = TimerCreate(7*MILLISECOND, TRUE, HandleTimer); 
              if (ledBlinkCount > 130)  {
                 PioSet(LED_PIO, TRUE); 
                 ledBlinkCount = 0; 
                 SendSerialDebugVars();
              }     
              SetRadioToAdvertise();
              break;
           default :
              PioSet(LED_PIO, FALSE); 
              broadcast_tid = TimerCreate(1*MILLISECOND, TRUE, HandleTimer); 
              state = 0;
        }        
        state++;
        ledBlinkCount++;
        
    } else {
       AppDebugWriteString("Invalid Timer!!!!!");        
    }
}

void SetRadioToIdle()
{
    LsStartStopScan( FALSE, whitelist_disabled, ls_addr_type_public ); 
    LsStartStopAdvertise(FALSE, whitelist_disabled, ls_addr_type_random);    
}

void SetRadioToListen()
{
    setRadioToListenCalls++;
    LsStartStopAdvertise(FALSE, whitelist_disabled, ls_addr_type_random);    
    GapSetMode( gap_role_observer, gap_mode_discover_no, gap_mode_connect_no, gap_mode_bond_no, gap_mode_security_none ); 
    LsStartStopScan( TRUE, whitelist_disabled, ls_addr_type_public ); 
}

void SetRadioToAdvertise()
{
    bool isDupe = TRUE;
    uint8 i = 0;
    
    setRadioToAdvertiseCalls++;
    LsStartStopScan( FALSE, whitelist_disabled, ls_addr_type_public ); 
    isDupe = CopyArrayCheckIfDuplicate(advData, priorAdvData, PACKET_SIZE);
    if (!isDupe) {
        autoIncrement++;
        if (autoIncrement == 0xFF) autoIncrement++;  /* 0xFF is the tag for the transmitting character */
        advData[8] = autoIncrement;  /* put a rolling tag number on packets for packet tracking */
        priorAdvData[8] = advData[8];  /* so we don't change each time based on autoincrement change */
                /* TODO  block out 0xFE as an error code when you see someone transmit to you with your own character num */
        for (i=0; i < PACKET_SIZE; i++)
            DebugWriteUint8(advData[i]); /* my observations won't come in over RF, send now */
    
        DebugWriteString("\r\n");   
    }
    LsStoreAdvScanData(0, NULL, ad_src_advertise);  /* clear advertising data */
    GapSetMode(gap_role_broadcaster, gap_mode_discover_no, gap_mode_connect_no, gap_mode_bond_no, gap_mode_security_none);
    LsStoreAdvScanData(9, advData, ad_src_advertise); /* put latest copy of heat map in advertising header */
    LsStartStopAdvertise(TRUE, whitelist_disabled, ls_addr_type_random);    
}

void AppInit(sleep_state last_sleep_state)
{
    /* Initialise application timers as soon as possible */
    TimerInit(MAX_APP_TIMERS, (void*)app_timers);
    SetRadioToListen();
    GapSetScanType(ls_scan_type_passive);  
    GapSetScanInterval (10000, 10000);  /* set to continuous when used. scan window and interval same */
    AppDebugInit();  
    GattInit();
    DebugInit(1, uartRxDataCallback, NULL);
    AppDebugWriteString("T2L Character ");
    DebugWriteUint8(CHARACTER_NUM);             
    AppDebugWriteString("  v2.50\r\n");
    PioSetDir(LED_PIO, TRUE);  /* set LED pin as output */
    PioSet(LED_PIO, TRUE);  /* set LED pin to high */
    broadcast_tid = TimerCreate( 300*MILLISECOND, TRUE, HandleTimer);
    InitHeatMap(); 
}

void ProcessPacket(LM_EVENT_T *event_data)
{
    uint8 i = 0, j = 0, k = 0, m = 0, lsb = 0, msb = 0;
    uint8 receivedData[PACKET_SIZE];
    static uint8 priorReceivedData[PACKET_SIZE] = {0};

    processedPacketCalls++;
    /* uint8 pointer arithmetic seems to move in 16 bit increments so on packed bytes use uint16 */
    uint8* transptr = NULL;
    transptr = (uint8*)event_data + sizeof(LM_EV_ADVERTISING_REPORT_T);
    /* the data payload is after the struct see CSR forum 
         https://forum.csr.com/forum/main-category/main-forum/software/295-entering-observer-role */
    transptr += 2; 
    uint16* eventDataPtr = (uint16*) transptr;
    uint16 the_word = 0;           
    
    if(WORD_LSB(*eventDataPtr) != AD_TYPE_MANUF) 
        return;  /* zero character in data packet array is not correct */
    
    for (i = 0; i <= PACKET_SIZE/2; i++)
    {
        the_word = *eventDataPtr;
        lsb = WORD_LSB(the_word);  /* TODO remove cleanup intermediaries */
        receivedData[j++] = lsb;

        if(i<4) /* don't want the byte from the last half of last word */
        {  
            msb = WORD_MSB(the_word);
            receivedData[j++] = msb;
        }   
        eventDataPtr++;
    }
    
    for (k = 1; k < PACKET_SIZE - 2; k++)  /* skip array slots that are not character RSSIs 1-6 only */
    {
        if(receivedData[k] == MY_SPOT_MARK_FF && event_data->adv_report.rssi < 0xFE &&
            event_data->adv_report.rssi > 0x10)
        {
            advData[k] = event_data->adv_report.rssi; /* record the new RSSI seen for character number k */
               /* TODO add recursive filter here but don't kill derivative for thrown doll detect */
            processedPacketSuccess++;
        }
    }
    if(CopyArrayCheckIfDuplicate(receivedData, priorReceivedData, PACKET_SIZE)) 
        return;  /* no need for UART sending repeated packet */
    
    for (m = 0; m < PACKET_SIZE; m++)            
       DebugWriteUint8(receivedData[m]); 

    DebugWriteString("\r\n");       
}

extern bool AppProcessLmEvent(lm_event_code event_code, LM_EVENT_T *event_data)
{   /* TODO determine if we want to mask and fix the advertising channel or use all 3 */
        
     appProcessLmEventCalls++;
     switch (event_code) 
     {
        case LM_EV_ADVERTISING_REPORT:
        {
            if (event_data->adv_report.data.event_type != 3 || event_data->adv_report.data.length_data != 0x0D)
                break;  /* we only want to see broadcasts with our packet length */ 
           
            ProcessPacket(event_data);            
        } /* end case LM_EV_ADVERTISING_REPORT */  
        break;
        
        default: 
        {
        }
        break;
     }
 
    return TRUE;
}
