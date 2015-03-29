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

#define KISS_FEND  0xc0
#define KISS_FESC  0xdb
#define KISS_TFEND 0xdc
#define KISS_TFESC 0xdd

enum {
	KISS_CMD_DATA = 0,
	KISS_CMD_TXDELAY,
	KISS_CMD_P,
	KISS_CMD_SlotTime,
	KISS_CMD_TXtail,
	KISS_CMD_FullDuplex,
	KISS_CMD_SetHardware,
	KISS_CMD_Return = 0xff
};

enum {
	KISS_DUPLEX_HALF = 0,
	KISS_DUPLEX_FULL
};

enum {
	KISS_QUEUE_IDLE,
	KISS_QUEUE_DELAYED,
};

//typedef void (*kiss_in_callback_t)(uint8_t *buf, size_t len);
typedef void (*kiss_exit_callback_t)(void);

struct Kiss_msg {
	uint8_t buf[ CONFIG_AX25_FRAME_BUF_LEN ];
	size_t pos;           // next byte to fill
	ticks_t last_tick;     // timestamp of last byte into buf
};

void kiss_init(KFile *fd, AX25Ctx *ax25, Afsk *afsk, kiss_exit_callback_t hook);

void kiss_serial_poll(void);
void kiss_queue_message(uint8_t *buf, size_t len);
void kiss_queue_process(void);
void kiss_send_host(uint8_t port, uint8_t *buf, size_t len);

#endif

