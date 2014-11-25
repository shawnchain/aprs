/**
 * \file
 * <!--
 * This file is part of BeRTOS.
 *
 * Bertos is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 *
 * Copyright 2006 Develer S.r.l. (http://www.develer.com/)
 * All Rights Reserved.
 * -->
 *
 * \brief AFSK modem hardware-specific definitions.
 *
 *
 * \author Francesco Sacchi <batt@develer.com>
 */

#ifndef HW_AFSK_H
#define HW_AFSK_H

#include "cfg/cfg_arch.h"

#include <avr/io.h>

struct Afsk;
void hw_afsk_adcInit(int ch, struct Afsk *_ctx);
void hw_afsk_dacInit(int ch, struct Afsk *_ctx);

/* ------------------------------------------------------------------------
 *  Configurations:
 *    D4-D7  -->  Data OUT
 *    D8     -->  PTT OUT
 *    D9     -->  TX(RED) LED OUT
 *    D10    -->  RX(GRN) LED OUT
 * ------------------------------------------------------------------------
 */

/**
 * Initialize the specified channel of the ADC for AFSK needs.
 * The adc should be configured to have a continuos stream of convertions.
 * For every convertion there must be an ISR that read the sample
 * and call afsk_adc_isr(), passing the context and the sample.
 *
 * \param ch channel to be used for AFSK demodulation.
 * \param ctx AFSK context (\see Afsk). This parameter must be saved and
 *            passed back to afsk_adc_isr() for every convertion.
 */

/*
 * This macro will be called for AFSK initialization. We could implement everything here as a macro,
 * but since initialization is rather complicated we decided to split its own function. Such function
 * is defined in hw_afsk.c.
 * Remember: since this .c file is not created by the wizard, you must add it to your_project_name.mk.
 * If you create the file using BeRTOS SDK, it will be added for you.
 */
#define AFSK_ADC_INIT(ch, ctx) hw_afsk_adcInit(ch, ctx)


/*
 * Here's some macros for controlling the RX/TX LEDs
 * THE _INIT() functions writes to the DDRB register
 * to configure the pins as output pins, and the _ON()
 * and _OFF() functions writes to the PORT registers
 * to turn the pins on or off.
 *
 * Use D9 for TX, D10 for RX, where the register is:
 * D8:  PORTB |= BV(0)  PTT
 * D9:  PORTB |= BV(1)  TX
 * D10: PORTB |= BV(2)  RX
 */
// Use PIN9 for TX, D10 for RX, the corresponding register:
#define AFSK_LED_INIT() do { DDRB |= BV(1)|BV(2);/*PIN9, PIN10*/ } while (0)
#define AFSK_LED_TX_ON()   do { PORTB |= BV(1); } while (0) // PIN9
#define AFSK_LED_TX_OFF()  do { PORTB &= ~BV(1); } while (0)
#define AFSK_LED_RX_ON()   do { PORTB |= BV(2); } while (0) // PIN10
#define AFSK_LED_RX_OFF()  do { PORTB &= ~BV(2); } while (0)


/**
 * Initialize the specified channel of the DAC for AFSK needs.
 * The DAC has to be configured in order to call an ISR for every sample sent.
 * The DAC doesn't have to start the IRQ immediatly but have to wait
 * the AFSK driver to call AFSK_DAC_IRQ_START().
 * The ISR must then call afsk_dac_isr() passing the AFSK context.
 * \param ch DAC channel to be used for AFSK modulation.
 * \param ctx AFSK context (\see Afsk).  This parameter must be saved and
 *             passed back to afsk_dac_isr() for every convertion.
 */
#define AFSK_DAC_INIT(ch, ctx)   do { (void)ch, (void)ctx; DDRD |= 0xF0/*D4-D7 as data*/; DDRB |= BV(0)/*D8 as PTT*/; } while (0)

/**
 * Start DAC convertions on channel \a ch.
 * \param ch DAC channel.
 */
#define AFSK_DAC_IRQ_START(ch)   do { (void)ch; extern bool hw_afsk_dac_isr; PORTB |= BV(0)/*PTT on*/; hw_afsk_dac_isr = true; } while (0)

/**
 * Stop DAC convertions on channel \a ch.
 * \param ch DAC channel.
 */
#define AFSK_DAC_IRQ_STOP(ch)    do { (void)ch; extern bool hw_afsk_dac_isr; PORTB &= ~BV(0)/*PTT off*/; hw_afsk_dac_isr = false; } while (0)

#endif /* HW_AFSK_H */
