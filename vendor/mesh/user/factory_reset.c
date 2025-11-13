
/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "../../../proj/tl_common.h"
#include "../../../proj_lib/ble/blt_config.h"
#include "../../../proj_lib/ble/service/ble_ll_ota.h"
#include "../../../proj_lib/ble/ll//ll.h"
#include "../../common/app_beacon.h"
#include "../../common/generic_model.h"
#include "utilities.h"
#include "led.h"
#include "factory_reset.h"

#include "debug.h"
#ifdef  FACTORY_RST_DBG_EN
#define DBG_FACTORY_RST_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_FACTORY_RST_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_FACTORY_RST_SEND_HEX(x)   Dbg_sendHex(x)
#else
#define DBG_FACTORY_RST_SEND_STR(x)
#define DBG_FACTORY_RST_SEND_INT(x)
#define DBG_FACTORY_RST_SEND_HEX(x)
#endif
/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

FactoryReset_str factoryReset = {false, 0 , 0};

/******************************************************************************/
/*                        PRIVATE FUNCTIONS DECLERATION                       */
/******************************************************************************/

/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/
/**
 * @func    start_factory_reset
 * @brief
 * @param   None
 * @retval  None
 */
void start_factory_reset(void)
{
	DBG_FACTORY_RST_SEND_STR("\n start_factory_reset");
	sleep_us(500000);   // wait tx buffer send completed.
	irq_disable();
	factory_reset();
	//show_ota_result(OTA_SUCCESS);
	start_reboot();
}

/**
 * @func    start_factory_reset
 * @brief
 * @param   None
 * @retval  None
 */
void handle_factory_reset_with_delay(void)
{
	if(factoryReset.flag == true){
		if(clock_time_exceed_ms(factoryReset.startTimeDelay, factoryReset.delayTime)) {
			if((clock_time_ms()     \
					- factoryReset.startTimeDelay) > MAX_FACTORY_RESET_DELAY_MS) {
				factoryReset.flag = false;
			}else{
			    start_factory_reset();
			}
		}
	}
}
/**
 * @func    start_factory_reset
 * @brief
 * @param   None
 * @retval  None
 */
void setup_factory_reset_with_delay(u8 enable)
{
	DBG_FACTORY_RST_SEND_STR("\n setup_factory_reset_with_delay");
	if(enable == true) {
		factoryReset.flag = true;
		factoryReset.startTimeDelay = clock_time_ms();
		factoryReset.delayTime = 1000;
		send_config_node_reset_status_manual();
		led_off_all();
		led_blink_color(
				LED_CONFIG_MASK, LED_COLOR_PINK, 2, LAST_STATE_REFRESH_LED, 180
			);
	}
	else {
		factoryReset.flag = false;
	}
}
/**
 * @func    factory_reset
 * @brief
 * @param   None
 * @retval  None
 */
int factory_reset()
{
	DBG_FACTORY_RST_SEND_STR("\n factory_reset");
	u8 r = irq_disable ();
	for(int i = 0; i < (FLASH_ADR_AREA_1_END - FLASH_ADR_AREA_1_START) / 4096; ++i) {
	    u32 adr = FLASH_ADR_AREA_1_START + i*0x1000;
		flash_erase_sector(adr);
	}
	for(int i = 0; i < (FLASH_ADR_AREA_2_END - FLASH_ADR_AREA_2_START) / 4096; ++i) {
	    u32 adr = FLASH_ADR_AREA_2_START + i*0x1000;
		flash_erase_sector(adr);
	}
    irq_restore(r);
    flash_erase_sector(FLASH_ADR_BLE_SWITCH_CONFIG);
    flash_erase_sector( FLASH_ADR_DIMMING_CONFIG);
    flash_erase_sector(FLASH_ADR_LOCK_SCHEDULE);
    flash_erase_sector(FLASH_ADR_EXECUTION_SCENE);
    flash_erase_sector(FLASH_ADR_LOCK_STATUS);
    flash_erase_sector(FLASH_ADR_BINDING_PARAMS);
	return 0;
}
/**
 * @func    kick_out
 * @brief
 * @param   None
 * @retval  None
 */
void kick_out(int led_en)
{
	DBG_FACTORY_RST_SEND_STR("\n kick_out");
	if(bls_ll_isConnectState()) {
		bls_ll_terminateConnection (0x13);
	}
	setup_factory_reset_with_delay(ENABLE);
}
