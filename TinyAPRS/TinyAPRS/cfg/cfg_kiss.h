
#ifndef CFG_KISS_H
#define CFG_KISS_H

/**
 * Module logging level.
 *
 * $WIZ$ type = "enum"
 * $WIZ$ value_list = "log_level"
 */
#define KISS_LOG_LEVEL      LOG_LVL_ERR

/**
 * Module logging format.
 *
 * $WIZ$ type = "enum"
 * $WIZ$ value_list = "log_format"
 */
#define KISS_LOG_FORMAT     LOG_FMT_TERSE


/**
 * Module Enable/Disable flag
 */
#define CONFIG_KISS_ENABLED 1

/**
 * KISS queue length
 * for AVR chip with 4k ram, 1 or 2 is enough
 * set 0 to disable the queue for Atmega328P with 2K ram
 */
#define CONFIG_KISS_QUEUE	0

/**
 * Number of KISS port to support.
 * The index of port id is started from 1
 */
#define CONFIG_KISS_PORT 1
#endif /* CFG_KISS_H */
