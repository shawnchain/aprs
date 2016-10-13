 #ifndef KISS_h_
#define KISS_h_

#include <stdint.h>
#include <string.h>

#include "cfg/cfg_kiss.h"

#include <io/kfile.h>
#include <net/afsk.h>
#include <net/ax25.h>
#include <drv/timer.h>
#include <algo/crc_ccitt.h>

//typedef void (*kiss_in_callback_t)(uint8_t *buf, size_t len);
typedef void (*kiss_exit_callback_t)(void);

typedef struct KissParam{
	uint8_t txdelay;
	uint8_t txtail;
	uint8_t persistence;
	uint8_t slot_time;
	uint8_t duplex;
}KissParam;

typedef struct KissCtx{
	KFile *serial;
	AX25Ctx *modem;
	uint8_t *rxBuf;
	uint16_t rxBufLen;
	uint16_t rxPos;
	ticks_t rxTick;

	/*
	uint8_t *txBuf;
	uint16_t txBufLen;
	uint16_t txPos;
	ticks_t txTick;
	*/
	KissParam param;
}KissCtx;

void kiss_init(KFile *serial, AX25Ctx *modem, uint8_t *buf, uint16_t bufLen, kiss_exit_callback_t hook);
void kiss_poll(void);
void kiss_send_to_modem(uint8_t *buf, size_t len);
void kiss_send_to_serial(uint8_t port, uint8_t *buf, size_t len);

#endif

