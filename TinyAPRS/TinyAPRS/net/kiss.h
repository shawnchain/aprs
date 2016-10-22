 #ifndef KISS_h_
#define KISS_h_

#include <stdint.h>
#include <string.h>
#include <drv/timer.h>

#include "cfg/cfg_kiss.h"

struct Serial;
struct SerialReader;
struct AX25Ctx;

typedef struct KissCtx{
	struct SerialReader *serialReader;
	struct AX25Ctx *modem;

	ticks_t  rxTick;
#if 0
	struct Serial  *serial;
	uint8_t *rxBuf;
	uint16_t rxBufLen;
	uint16_t rxPos;
#endif

#if 0 // TX Buffering Enabled
	uint8_t *txBuf;
	uint16_t txBufLen;
	uint16_t txPos;
	ticks_t txTick;
#endif

}KissCtx;

void kiss_init(struct SerialReader *serialReader,struct AX25Ctx *modem);
void kiss_poll(void);
void kiss_send_to_modem(uint8_t *buf, size_t len);
void kiss_send_to_serial(uint8_t port, uint8_t cmd, uint8_t *buf, size_t len);

#endif

