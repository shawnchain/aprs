#include "cfg/cfg_softser.h"

#include <ctype.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <cfg/macros.h>
#include <cpu/irq.h>
#include <string.h>
#include <cfg/compiler.h>
#include "hw_soft_ser.h"
#include "hw_pins_arduino.h"

static char _receive_buffer[_SS_MAX_RX_BUFF];
static volatile uint8_t _receive_buffer_tail = 0;
static volatile uint8_t _receive_buffer_head = 0;
SoftSerial *pActiveSerPort = NULL;

static void hw_soft_ser_handle_interrupt(void);
//
// Lookup table
//
typedef struct _DELAY_TABLE {
	long baud;
	unsigned short rx_delay_centering;
	unsigned short rx_delay_intrabit;
	unsigned short rx_delay_stopbit;
	unsigned short tx_delay;
} DELAY_TABLE;

#if 1 //CPU_FREQ == 16000000

static const DELAY_TABLE PROGMEM table[] = {
	//  baud    rxcenter   rxintra    rxstop    tx
	{115200, 1, 17, 17, 12,},
	{57600, 10, 37, 37, 33,},
	{38400, 25, 57, 57, 54,},
	{31250, 31, 70, 70, 68,},
	{28800, 34, 77, 77, 74,},
	{19200, 54, 117, 117, 114,},
	{14400, 74, 156, 156, 153,},
	{9600, 114, 236, 236, 233,},
	{4800, 233, 474, 474, 471,},
	{2400, 471, 950, 950, 947,},
	{1200, 947, 1902, 1902, 1899,},
	{600, 1902, 3804, 3804, 3800,},
	{300,3804, 7617, 7617, 7614,},};

const int XMIT_START_ADJUSTMENT = 5;
#else

#error This version of SoftwareSerial supports only 16MHz processors

#endif

static void pinMode(uint8_t pin, uint8_t mode) {
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	volatile uint8_t *reg, *out;

	if (port == NOT_A_PIN)
		return;

	// JWS: can I let the optimizer do this?
	reg = portModeRegister(port);
	out = portOutputRegister(port);

	if (mode == INPUT) {
		//uint8_t oldSREG = SREG;
		//cli();
		ATOMIC(*reg &= ~bit; *out &= ~bit;);
		//SREG = oldSREG;
	} else if (mode == INPUT_PULLUP) {
//		uint8_t oldSREG = SREG;
//		cli();
		ATOMIC(*reg &= ~bit; *out |= bit;);
//		SREG = oldSREG;
	} else {
//		uint8_t oldSREG = SREG;
//		cli();
		ATOMIC(*reg |= bit
		;
	);
//		SREG = oldSREG;
}
}

static void digitalWrite(uint8_t pin, uint8_t val) {
uint8_t bit = digitalPinToBitMask(pin);
uint8_t port = digitalPinToPort(pin);
volatile uint8_t *out;

if (port == NOT_A_PIN)
	return;

out = portOutputRegister(port);

uint8_t oldSREG = SREG;
cli();
if (val == LOW) {
	*out &= ~bit;
} else {
	*out |= bit;
}
SREG = oldSREG;
}

/*
 static int digitalRead(uint8_t pin) {
 uint8_t bit = digitalPinToBitMask(pin);
 uint8_t port = digitalPinToPort(pin);
 if (port == NOT_A_PIN)
 return LOW;

 if (*portInputRegister(port) & bit)
 return HIGH;
 return LOW;
 }
 */

//
// Debugging
//
// This function generates a brief pulse
// for debugging or measuring on an oscilloscope.
INLINE void DebugPulse(uint8_t pin, uint8_t count) {
(void) pin;
(void) count;
#if CFG_SOFTSER_DEBUG
volatile uint8_t *pport = portOutputRegister(digitalPinToPort(pin));

uint8_t val = *pport;
while (count--)
{
	*pport = val | digitalPinToBitMask(pin);
	*pport = val;
}
#endif
}

/* static */INLINE void hw_soft_ser_tunedDelay(uint16_t delay) {
uint8_t tmp = 0;

asm volatile("sbiw    %0, 0x01 \n\t"
		"ldi %1, 0xFF \n\t"
		"cpi %A0, 0xFF \n\t"
		"cpc %B0, %1 \n\t"
		"brne .-10 \n\t"
		: "+r" (delay), "+a" (tmp)
		: "0" (delay)
);
}

void hw_soft_ser_init(SoftSerial *pSSer, uint8_t pinRX, uint8_t pinTX) {
uint8_t port;
memset(pSSer, 0, sizeof(SoftSerial));
pSSer->_inverse_logic = false;

pinMode(pinTX, OUTPUT);
digitalWrite(pinTX, HIGH);

pSSer->_transmitBitMask = digitalPinToBitMask(pinTX);
port = digitalPinToPort(pinTX);
pSSer->_transmitPortRegister = portOutputRegister(port);

pinMode(pinRX, INPUT);
if (!pSSer->_inverse_logic)
	digitalWrite(pinRX, HIGH);  // pullup for normal logic!
pSSer->_receivePin = pinRX;
pSSer->_receiveBitMask = digitalPinToBitMask(pinRX);
port = digitalPinToPort(pinRX);
pSSer->_receivePortRegister = portInputRegister(port);

/*
 uint8_t oldSREG = SREG;
 IRQ_DISABLE;

 // Configure PortD2 as INPUT and set level to low
 DDRD &= ~BV(DDD2); // as input
 PORTD &= ~BV(PD2); // go low

 MCUCR &= ~BV(PUD);
 PORTD |= BV(PD2); // go high

 SREG = oldSREG;
 IRQ_ENABLE;
 */
pActiveSerPort = pSSer;
}

//static bool hw_soft_ser_listen(SoftSerial *pSSer) {
//	if (pActiveSerPort != pSSer) {
//		pSSer->_buffer_overflow = false;
//		uint8_t oldSREG = SREG;
//		cli();
//		_receive_buffer_head = 0;
//		_receive_buffer_tail = 0;
//		pActiveSerPort = pSSer;
//		SREG = oldSREG;
//		return true;
//	}
//
//	return false;
//}

void hw_soft_ser_start(SoftSerial *pSSer, long speed) {
//	// Enable the PinChangeInterrupt
//	PCICR |= BV(PCIE2);
//	PCMSK2 |= BV(PCINT18);// PORTD2 corresponding ISR mask is PCINT18
for (unsigned i = 0; i < sizeof(table) / sizeof(table[0]); ++i) {
	long baud = pgm_read_dword(&table[i].baud);
	if (baud == speed) {
		pSSer->_rx_delay_centering =
				pgm_read_word(&table[i].rx_delay_centering);
		pSSer->_rx_delay_intrabit = pgm_read_word(&table[i].rx_delay_intrabit);
		pSSer->_rx_delay_stopbit = pgm_read_word(&table[i].rx_delay_stopbit);
		pSSer->_tx_delay = pgm_read_word(&table[i].tx_delay);
		break;
	}
}

// Set up RX interrupts, but only if we have a valid RX baud rate
uint8_t rPin = pSSer->_receivePin;

if (pSSer->_rx_delay_stopbit) {
	/*
	 uint8_t *p = digitalPinToPCICR(rPin);
	 if (p > 0) {
	 *p |= _BV(digitalPinToPCICRbit(rPin));
	 (*digitalPinToPCMSK(rPin)) |= _BV(digitalPinToPCMSKbit(rPin));
	 }
	 */
	if (rPin <= 21) {
		PCICR |= _BV(digitalPinToPCICRbit(rPin));
		(*digitalPinToPCMSK(rPin)) |= _BV(digitalPinToPCMSKbit(rPin));
	}
	hw_soft_ser_tunedDelay(pSSer->_tx_delay); // if we were low this establishes the end
}

#if CFG_SOFTSER_DEBUG
pinMode(CFG_SOFTSER_DEBUG_PIN1, OUTPUT);
pinMode(CFG_SOFTSER_DEBUG_PIN2, OUTPUT);
#endif

//hw_soft_ser_listen(pSSer);
if (pActiveSerPort != pSSer) {
	pSSer->_buffer_overflow = false;
	//uint8_t oldSREG = SREG;
	//cli();
	ATOMIC(
			_receive_buffer_head = 0; _receive_buffer_tail = 0; pActiveSerPort = pSSer;);
	//SREG = oldSREG;
}
}

void hw_soft_ser_stop(SoftSerial *pSSer) {
/*
 // Disable the PinChangeInterrupt
 PCICR &= ~BV(PCIE2);
 PCMSK2 &= ~BV(PCINT18);// PORTD2 corresponding ISR mask is PCINT18
 */
uint8_t rPin = pSSer->_receivePin;
if (digitalPinToPCMSK(rPin)) {
	*digitalPinToPCMSK(rPin) &= ~_BV(digitalPinToPCMSKbit(rPin));
}

}

// Read data from buffer
int hw_soft_ser_read(SoftSerial *pSSer) {
if (pActiveSerPort != pSSer)
	return -1;

// Empty buffer?
if (_receive_buffer_head == _receive_buffer_tail)
	return -1;

// Read from "head"
uint8_t d = _receive_buffer[_receive_buffer_head]; // grab next byte
_receive_buffer_head = (_receive_buffer_head + 1) % _SS_MAX_RX_BUFF;
return d;
}

int hw_soft_ser_available(SoftSerial *pSSer) {
if (pActiveSerPort != pSSer) {
	return 0;
}
return (_receive_buffer_tail + _SS_MAX_RX_BUFF - _receive_buffer_head)
		% _SS_MAX_RX_BUFF;
}

#define tx_pin_write(pSSer,pin_state) \
  if (pin_state == LOW){ \
	  *(pSSer->_transmitPortRegister) &= ~(pSSer->_transmitBitMask); \
  }else{ \
	  *(pSSer->_transmitPortRegister) |= pSSer->_transmitBitMask; \
  }


#define rx_pin_read(pSSer) \
  (*(pSSer->_receivePortRegister) & (pSSer->_receiveBitMask))

int hw_soft_ser_write(SoftSerial *pSSer, uint8_t b) {
if (pSSer->_tx_delay == 0) {
	return 0;
}

uint8_t oldSREG = SREG;
cli();
// turn off interrupts for a clean txmit

// Write the start bit
tx_pin_write(pSSer, pSSer->_inverse_logic ? HIGH : LOW);
hw_soft_ser_tunedDelay(pSSer->_tx_delay + XMIT_START_ADJUSTMENT);

// Write each of the 8 bits
if (pSSer->_inverse_logic) {
	for (uint8_t mask = 0x01; mask; mask <<= 1) {
		if (b & mask) { // choose bit
			tx_pin_write(pSSer, LOW); // send 1
		} else {
			tx_pin_write(pSSer, HIGH); // send 0
		}

		hw_soft_ser_tunedDelay(pSSer->_tx_delay);
	}

	tx_pin_write(pSSer, LOW); // restore pin to natural state
} else {
	for (uint8_t mask = 0x01; mask; mask <<= 1) {
		if (b & mask) { // choose bit
			tx_pin_write(pSSer, HIGH); // send 1
		} else {
			tx_pin_write(pSSer, LOW); // send 0
		}

		hw_soft_ser_tunedDelay(pSSer->_tx_delay);
	}

	tx_pin_write(pSSer, HIGH); // restore pin to natural state
}

SREG = oldSREG; // turn interrupts back on
hw_soft_ser_tunedDelay(pSSer->_tx_delay);

return 1;
}

//
// The receive routine called by the interrupt handler
//
static void hw_soft_ser_recv(SoftSerial *pSSer) {

#if GCC_VERSION < 40302
// Work-around for avr-gcc 4.3.0 OSX version bug
// Preserve the registers that the compiler misses
// (courtesy of Arduino forum user *etracer*)
asm volatile(
		"push r18 \n\t"
		"push r19 \n\t"
		"push r20 \n\t"
		"push r21 \n\t"
		"push r22 \n\t"
		"push r23 \n\t"
		"push r26 \n\t"
		"push r27 \n\t"
		::);
#endif

uint8_t d = 0;

// If RX line is high, then we don't see any start bit
// so interrupt is probably not for us
if (pSSer->_inverse_logic ? rx_pin_read(pSSer) : !rx_pin_read(pSSer)) {
	// Wait approximately 1/2 of a bit width to "center" the sample
	hw_soft_ser_tunedDelay(pSSer->_rx_delay_centering);
	DebugPulse(CFG_SOFTSER_DEBUG_PIN2, 1);

	// Read each of the 8 bits
	for (uint8_t i = 0x1; i; i <<= 1) {
		hw_soft_ser_tunedDelay(pSSer->_rx_delay_intrabit);
		DebugPulse(CFG_SOFTSER_DEBUG_PIN2, 1);
		uint8_t noti = ~i;
		if (rx_pin_read(pSSer))
			d |= i;
		else
			// else clause added to ensure function timing is ~balanced
			d &= noti;
	}

	// skip the stop bit
	hw_soft_ser_tunedDelay(pSSer->_rx_delay_stopbit);
	DebugPulse(CFG_SOFTSER_DEBUG_PIN2, 1);

	if (pSSer->_inverse_logic)
		d = ~d;

	// if buffer full, set the overflow flag and return
	if ((_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF != _receive_buffer_head) {
		// save new data in buffer: tail points to where byte goes
		_receive_buffer[_receive_buffer_tail] = d; // save new byte
		_receive_buffer_tail = (_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF;
	} else {
#if CFG_SOFTSER_DEBUG // for scope: pulse pin as overflow indictator
		DebugPulse(CFG_SOFTSER_DEBUG_PIN1, 1);
#endif
		pSSer->_buffer_overflow = true;
	}
}

#if GCC_VERSION < 40302
// Work-around for avr-gcc 4.3.0 OSX version bug
// Restore the registers that the compiler misses
asm volatile(
		"pop r27 \n\t"
		"pop r26 \n\t"
		"pop r23 \n\t"
		"pop r22 \n\t"
		"pop r21 \n\t"
		"pop r20 \n\t"
		"pop r19 \n\t"
		"pop r18 \n\t"
		::);
#endif
}

inline void hw_soft_ser_handle_interrupt() {
if (pActiveSerPort) {
	hw_soft_ser_recv(pActiveSerPort);
}
}
 #if defined(PCINT0_vect)
 DECLARE_ISR(PCINT0_vect)
 {
 hw_soft_ser_handle_interrupt();
 }
 #endif

 #if defined(PCINT1_vect)
 DECLARE_ISR(PCINT1_vect)
 {
 hw_soft_ser_handle_interrupt();
 }
 #endif

#if defined(PCINT2_vect)
DECLARE_ISR(PCINT2_vect)
{
hw_soft_ser_handle_interrupt();
}
#endif

/*
 #if defined(PCINT3_vect)
 DECLARE_ISR(PCINT3_vect)
 {
 hw_soft_ser_handle_interrupt();
 }
 #endif
 */
