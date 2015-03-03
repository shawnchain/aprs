
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
 * KISS queue length
 * for AVR chip with 4k ram, 1 or 2 is enough
 * set 0 to disable the queue for Atmega328P with 2K ram
 */
#define CONFIG_KISS_QUEUE	0


#endif /* CFG_KISS_H */
