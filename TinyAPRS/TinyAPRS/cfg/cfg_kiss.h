
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
 * Kiss queue length
 * for AVR chip with 4k ram, 1 or 2 is enough
 */
#define CONFIG_KISS_QUEUE	2
#endif /* CFG_KISS_H */
