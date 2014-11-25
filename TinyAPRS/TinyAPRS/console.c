#define ENABLE_HELP true

#include "console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "buildrev.h"
#include "config.h"
#include <cfg/cfg_afsk.h>

#include <lcd/hw_lcd_4884.h>

static bool PRINT_SRC = true;
static bool PRINT_DST = true;
static bool PRINT_PATH = true;
static bool PRINT_DATA = true;
static bool PRINT_INFO = true;
static bool VERBOSE = true;
static bool SILENT = false;
static bool SS_INIT = false;
static bool SS_DEFAULT_CONF = false;

static AX25Call src;
static AX25Call dst;
static AX25Call path1;
static AX25Call path2;

static char CALL[7] = DEFAULT_CALLSIGN;
static int CALL_SSID = 0;
static char DST[7] = DEFAULT_DESTINATION_CALL;
static int DST_SSID = 0;
static char PATH1[7] = "WIDE1";
static int PATH1_SSID = 1;
static char PATH2[7] = "WIDE2";
static int PATH2_SSID = 2;

static AX25Call path[4];
static AX25Ctx *ax25ctx;
static Serial *g_ser;

#define NV_MAGIC_BYTE 0x69
uint8_t EEMEM nvMagicByte;
uint8_t EEMEM nvCALL[6];
uint8_t EEMEM nvDST[6];
uint8_t EEMEM nvPATH1[6];
uint8_t EEMEM nvPATH2[6];
uint8_t EEMEM nvCALL_SSID;
uint8_t EEMEM nvDST_SSID;
uint8_t EEMEM nvPATH1_SSID;
uint8_t EEMEM nvPATH2_SSID;
bool EEMEM nvPRINT_SRC;
bool EEMEM nvPRINT_DST;
bool EEMEM nvPRINT_PATH;
bool EEMEM nvPRINT_DATA;
bool EEMEM nvPRINT_INFO;
bool EEMEM nvVERBOSE;
bool EEMEM nvSILENT;
uint8_t EEMEM nvPOWER;
uint8_t EEMEM nvHEIGHT;
uint8_t EEMEM nvGAIN;
uint8_t EEMEM nvDIRECTIVITY;
uint8_t EEMEM nvSYMBOL_TABLE;
uint8_t EEMEM nvSYMBOL;
uint8_t EEMEM nvAUTOACK;

// Location packet assembly fields
static char latitude[8] = "";
static char longtitude[9] = "";
static char symbolTable = '/';
static char symbol = 'n';

static uint8_t power = 10;
static uint8_t height = 10;
static uint8_t gain = 10;
static uint8_t directivity = 10;
/////////////////////////

// Message packet assembly fields
static char message_recip[6] = "";
static int message_recip_ssid = -1;

static int message_seq = 0;
static char lastMessage[67] = "";
static size_t lastMessageLen;
static bool message_autoAck = false;
/////////////////////////

static void enable_print_flag(bool* var, bool enabled, const char* name){
	*var = enabled;
#if 0
	kfile_print(&g_ser->fd,"OK");
#else
	if(!SILENT && VERBOSE){
		if(enabled){
			kprintf("Print %s enabled\r\n", name); 
		}else{
			kprintf("Print %s disabled\r\n", name); 
		}
	}
	if(!SILENT && !VERBOSE){
		kprintf("1\n");
	}
#endif
}

#if 1

#define respond_ok(str) kfile_print(&g_ser->fd,"OK\n");
#define respond_ok_with_ssid(msg,call,ssid) kfile_print(&g_ser->fd,"OK\n");
#define respond_error(str) kfile_print(&g_ser->fd,"ERROR\n");

#else

static inline void respond_ok(const char* str){
	if (!SILENT && VERBOSE){
		kprintf("%s\n", str);
	}
	if(!SILENT && !VERBOSE){
		kprintf("1\n");
	}
}
static inline void respond_ok_with_ssid(const char* msg, const char* call, uint8_t ssid){
	if(!SILENT && VERBOSE){
		kprintf("%s: %.6s-%d\n",msg, call, ssid); 
	}
	if(!SILENT && !VERBOSE){
		kprintf("1\n");
	}
}

#endif

void ss_init(AX25Ctx *ax25, Serial *ser) {
    ax25ctx = ax25;
	g_ser = ser;
    ss_loadSettings();
    SS_INIT = true;
	
	_delay_ms(300);
	kfile_printf(&ser->fd, "TinyAPRS 1.0(f%da%d)-r%d BG5HHP - ready\r\n",CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,VERS_BUILD);

//#define MicroAPRS2_HAS_LCD 1 - SEE config.h
#if defined(HAS_LCD) && HAS_LCD == 1
	lcd_4884_init();
	_delay_ms(100);
	lcd_4884_writeString("TinyAPRS",0);
	lcd_4884_writeString("Made by BG5HHP",2);
#endif
}

void ss_clearSettings(void) {
    eeprom_update_byte((void*)&nvMagicByte, 0xFF);
	respond_ok("Configuration cleared. Restart to load defaults");
}

void ss_loadSettings(void) {
    uint8_t verification = eeprom_read_byte((void*)&nvMagicByte);
    if (verification == NV_MAGIC_BYTE) {
        eeprom_read_block((void*)CALL, (void*)nvCALL, 6);
        eeprom_read_block((void*)DST, (void*)nvDST, 6);
        eeprom_read_block((void*)PATH1, (void*)nvPATH1, 6);
        eeprom_read_block((void*)PATH2, (void*)nvPATH2, 6);

        CALL_SSID = eeprom_read_byte((void*)&nvCALL_SSID);
        DST_SSID = eeprom_read_byte((void*)&nvDST_SSID);
        PATH1_SSID = eeprom_read_byte((void*)&nvPATH1_SSID);
        PATH2_SSID = eeprom_read_byte((void*)&nvPATH2_SSID);

        PRINT_SRC = eeprom_read_byte((void*)&nvPRINT_SRC);
        PRINT_DST = eeprom_read_byte((void*)&nvPRINT_DST);
        PRINT_PATH = eeprom_read_byte((void*)&nvPRINT_PATH);
        PRINT_DATA = eeprom_read_byte((void*)&nvPRINT_DATA);
        PRINT_INFO = eeprom_read_byte((void*)&nvPRINT_INFO);
        VERBOSE = eeprom_read_byte((void*)&nvVERBOSE);
        SILENT = eeprom_read_byte((void*)&nvSILENT);

        power = eeprom_read_byte((void*)&nvPOWER);
        height = eeprom_read_byte((void*)&nvHEIGHT);
        gain = eeprom_read_byte((void*)&nvGAIN);
        directivity = eeprom_read_byte((void*)&nvDIRECTIVITY);
        symbolTable = eeprom_read_byte((void*)&nvSYMBOL_TABLE);
        symbol = eeprom_read_byte((void*)&nvSYMBOL);
        message_autoAck = eeprom_read_byte((void*)&nvAUTOACK);

		if(SS_INIT){
			respond_ok("Config loaded");
		}
    } else {
		if(SS_INIT){
			respond_error("No stored config to load!");
		}
        SS_DEFAULT_CONF = true;
    }
}

void ss_saveSettings(void) {
    eeprom_update_block((void*)CALL, (void*)nvCALL, 6);
    eeprom_update_block((void*)DST, (void*)nvDST, 6);
    eeprom_update_block((void*)PATH1, (void*)nvPATH1, 6);
    eeprom_update_block((void*)PATH2, (void*)nvPATH2, 6);

    eeprom_update_byte((void*)&nvCALL_SSID, CALL_SSID);
    eeprom_update_byte((void*)&nvDST_SSID, DST_SSID);
    eeprom_update_byte((void*)&nvPATH1_SSID, PATH1_SSID);
    eeprom_update_byte((void*)&nvPATH2_SSID, PATH2_SSID);

    eeprom_update_byte((void*)&nvPRINT_SRC, PRINT_SRC);
    eeprom_update_byte((void*)&nvPRINT_DST, PRINT_DST);
    eeprom_update_byte((void*)&nvPRINT_PATH, PRINT_PATH);
    eeprom_update_byte((void*)&nvPRINT_DATA, PRINT_DATA);
    eeprom_update_byte((void*)&nvPRINT_INFO, PRINT_INFO);
    eeprom_update_byte((void*)&nvVERBOSE, VERBOSE);
    eeprom_update_byte((void*)&nvSILENT, SILENT);

    eeprom_update_byte((void*)&nvPOWER, power);
    eeprom_update_byte((void*)&nvHEIGHT, height);
    eeprom_update_byte((void*)&nvGAIN, gain);
    eeprom_update_byte((void*)&nvDIRECTIVITY, directivity);
    eeprom_update_byte((void*)&nvSYMBOL_TABLE, symbolTable);
    eeprom_update_byte((void*)&nvSYMBOL, symbol);
    eeprom_update_byte((void*)&nvAUTOACK, message_autoAck);

    eeprom_update_byte((void*)&nvMagicByte, NV_MAGIC_BYTE);
	respond_ok("Config saved\n");
}

static uint16_t message_count = 0;
void ss_messageCallback(struct AX25Msg *msg, Serial *ser) {
	message_count++;
	kfile_printf(&ser->fd, "%d",message_count);

#if defined(HAS_LCD) && HAS_LCD == 1
	lcd_4884_clear();
	char buf[16];
	buf[15] = 0;
	snprintf(buf,15,"%04d:%.6s-%d",message_count,msg->src.call,msg->src.ssid);
	lcd_4884_writeString(buf,0);
	/*
	snprintf(buf,15,"%.6s-%d>%.6s-%d",msg->src.call, msg->src.ssid,msg->dst.call, msg->dst.ssid);
	lcd_4884_writeString(buf,1);
	snprintf(buf,15," %.6s-%d",msg->dst.call, msg->dst.ssid);
	lcd_4884_writeString(buf,2);
	*/
#define SCREEN_WIDTH 15
	//snprintf(buf,31,"%.*s", 15, msg->info);
	short txtlen = msg->len;
	const uint8_t *txt = msg->info;
	for(int i = 0;i<5;i++){
		short len_to_print = txtlen > SCREEN_WIDTH ? SCREEN_WIDTH:txtlen;
		snprintf(buf,len_to_print,"%s",txt);
		lcd_4884_writeString(buf,i+1);
		txtlen -= len_to_print;
		if(txtlen > 0){
			txt += len_to_print;
			continue;
		}else{
			break;
		}
	}
#endif

/*
    if (PRINT_SRC) {
        if (PRINT_INFO) kfile_print(&ser->fd, "SRC: ");
        kfile_printf(&ser->fd, "[%.6s-%d] ", msg->src.call, msg->src.ssid);
    }
    if (PRINT_DST) {
        if (PRINT_INFO) kfile_printf(&ser->fd, "DST: ");
        kfile_printf(&ser->fd, "[%.6s-%d] ", msg->dst.call, msg->dst.ssid);
    }
*/
    kfile_print(&ser->fd, "A");
    kfile_printf(&ser->fd, "%.6s-%d>", msg->src.call, msg->src.ssid);
    kfile_printf(&ser->fd, "%.6s-%d,", msg->dst.call, msg->dst.ssid);
/*
    if (PRINT_PATH) {
        if (PRINT_INFO) kfile_print(&ser->fd, "PATH: ");
        for (int i = 0; i < msg->rpt_cnt; i++)
            kfile_printf(&ser->fd, "[%.6s-%d] ", msg->rpt_lst[i].call, msg->rpt_lst[i].ssid);
    }
*/
    for (int i = 0; i < msg->rpt_cnt; i++) {
        if (i == msg->rpt_cnt -1) {
            kfile_printf(&ser->fd, "%.6s-%d", msg->rpt_lst[i].call, msg->rpt_lst[i].ssid);
        } else {
            kfile_printf(&ser->fd, "%.6s-%d,", msg->rpt_lst[i].call, msg->rpt_lst[i].ssid);
        }
    }
/*    
    if (PRINT_DATA) {
        if (PRINT_INFO) kfile_print(&ser->fd, "DATA: ");
        kfile_printf(&ser->fd, "%.*s", msg->len, msg->info);
    }
*/
    kfile_print(&ser->fd, ":");
    kfile_printf(&ser->fd, "%.*s", msg->len, msg->info);

    kfile_print(&ser->fd, "\r\n");

    if (message_autoAck && msg->len > 11) {
        char mseq[6];
        bool shouldAck = true;
        int msl = 0;
        int loc = msg->len - 1;
        size_t i = 0;

        while (i<7 && i < msg->len) {
            if (msg->info[loc-i] == '{') {
                size_t p;
                for (p = 0; p <= i; p++) {
                    mseq[p] = msg->info[loc-i+p];
                    msl = i;
                }
            }
            i++;
        }

        if (msl != 0) {
            int pos = 1;
            int ssidPos = 0;
            while (pos < 7) {
                if (msg->info[pos] != CALL[pos-1]) {
                    shouldAck = false;
                    pos = 7;
                }
                pos++;
            }
            while (pos < 10) {
                if (msg->info[pos] == '-') ssidPos = pos;
                pos++;
            }
            if (ssidPos != 0) {
                if (msg->info[ssidPos+2] == ' ') {
                    if (msg->info[ssidPos+1]-48 != CALL_SSID) {
                        shouldAck = false;
                    }
                } else {
                    int assid = 10+(msg->info[ssidPos+2]-48);
                    if (assid != CALL_SSID) {
                        shouldAck = false;
                    }
                }
            }

            if (msl != 0 && shouldAck) {
                int ii = 0;
                char *ack = malloc(14+msl);

                for (ii = 0; ii < 9; ii++) {
                    ack[1+ii] = ' ';
                }
                int calllen = 0;
                for (ii = 0; ii < 6; ii++) {
                    if (msg->src.call[ii] != 0) {
                        ack[1+ii] = msg->src.call[ii];
                        calllen++;
                    }
                }

                if (msg->src.ssid != 0) {
                    ack[1+calllen] = '-';
                    if (msg->src.ssid < 10) {
                        ack[2+calllen] = msg->src.ssid+48;
                    } else {
                        ack[2+calllen] = 49;
                        ack[3+calllen] = msg->src.ssid-10+48;
                    }
                }

                ack[0] = ':';
                ack[10] = ':';
                ack[11] = 'a';
                ack[12] = 'c';
                ack[13] = 'k';
                
                for (ii = 0; ii < msl; ii++) {
                    ack[14+ii] = mseq[ii+1];
                }

                _delay_ms(1750);
                ss_sendPkt(ack, 14+msl, ax25ctx);

                free(ack);
            }
        }
    }
}

/*
 * Parse serial input data
 */
void ss_serialCallback(void *_buffer, size_t length, Serial *ser, AX25Ctx *ctx) {
    uint8_t *buffer = (uint8_t *)_buffer;
    if(length == 0) return;

	// ! as first char to send packet
	if (buffer[0] == '!' && length > 1) {
		buffer++; length--;
		ss_sendPkt(buffer, length, ctx);
		respond_ok("Packet sent");
	} else if (buffer[0] == '@') {
		buffer++; length--;
		ss_sendLoc(buffer, length, ctx);
		respond_ok("Location update sent");
	} else if (buffer[0] == '#') {
		buffer++; length--;
		ss_sendMsg(buffer, length, ctx);
		respond_ok("Message sent");
	}
	#if ENABLE_HELP
	else if (buffer[0] == '?') {
		ss_printHelp();
	}
	#endif
	else if (buffer[0] == 'H' || buffer[0] == 'h') {
		ss_printSettings();
	} else if (buffer[0] == 'S') {
		ss_saveSettings();
	} else if (buffer[0] == 'C') {
		ss_clearSettings();
	} else if (buffer[0] == 'L') {
		ss_loadSettings();
	} else if (buffer[0] == 'c' && length > 3) {
		buffer++; length--;
		int count = 0;
		while (length-- && count < 6) {
			char c = buffer[count];
			if (c != 0 && c != 10 && c != 13) {
				CALL[count] = c;
			} else {
				CALL[count] = 0x00;
			}
			count++;
		}
		while (count < 6) {
			CALL[count] = 0x00;
			count++;
		}
		respond_ok_with_ssid("Callsign", CALL, CALL_SSID);

	} else if (buffer[0] == 'd' && length > 3) {
		buffer++; length--;
		int count = 0;
		while (length-- && count < 6) {
			char c = buffer[count];
			if (c != 0 && c != 10 && c != 13) {
				DST[count] = c;
			} else {
				DST[count] = 0;
			}
			count++;
		}
		while (count < 6) {
			DST[count] = 0x00;
			count++;
		}
		respond_ok_with_ssid("Destination", CALL, CALL_SSID);


	} else if (buffer[0] == '1' && length > 1) {
		buffer++; length--;
		int count = 0;
		while (length-- && count < 6) {
			char c = buffer[count];
			if (c != 0 && c != 10 && c != 13) {
				PATH1[count] = c;
			} else {
				PATH1[count] = 0;
			}
			count++;
		}
		while (count < 6) {
			PATH1[count] = 0x00;
			count++;
		}
		respond_ok_with_ssid("Path1", PATH1, PATH1_SSID);


	} else if (buffer[0] == '2' && length > 1) {
		buffer++; length--;
		int count = 0;
		while (length-- && count < 6) {
			char c = buffer[count];
			if (c != 0 && c != 10 && c != 13) {
				PATH2[count] = c;
			} else {
				PATH2[count] = 0;
			}
			count++;
		}
		while (count < 6) {
			PATH2[count] = 0x00;
			count++;
		}
		respond_ok_with_ssid("Path2",PATH2,PATH2_SSID);


	} else if (buffer[0] == 's' && length > 2) {
		buffer++; length--;
		if (buffer[0] == 'c') {
			if (length > 2 && buffer[2] > 48 && buffer[2] < 58) {
				CALL_SSID = 10+buffer[2]-48;
			} else {
				CALL_SSID = buffer[1]-48;
			}
			respond_ok_with_ssid("Callsign", CALL, CALL_SSID);
		}
		if (buffer[0] == 'd') {
			if (length > 2 && buffer[2] > 48 && buffer[2] < 58) {
				DST_SSID = 10+buffer[2]-48;
			} else {
				DST_SSID = buffer[1]-48;
			}
			respond_ok_with_ssid("Destination", DST, DST_SSID);
		}
		if (buffer[0] == '1' && buffer[2] > 48 && buffer[2] < 58) {
			if (length > 2) {
				PATH1_SSID = 10+buffer[2]-48;
			} else {
				PATH1_SSID = buffer[1]-48;
			}
			respond_ok_with_ssid("Path1", PATH1, PATH1_SSID);
		}
		if (buffer[0] == '2' && buffer[2] > 48 && buffer[2] < 58) {
			if (length > 2) {
				PATH2_SSID = 10+buffer[2]-48;
			} else {
				PATH2_SSID = buffer[1]-48;
			}
			respond_ok_with_ssid("Path2", PATH2, PATH2_SSID);
		}

	} else if (buffer[0] == 'p' && length > 2) {
		buffer++; length--;
		if (buffer[0] == 's') {
			enable_print_flag(&PRINT_SRC,(buffer[1] == 49) , "SRC");
		}
		if (buffer[0] == 'd') {
			enable_print_flag(&PRINT_DST, (buffer[1] == 49), "DST");
		}
		if (buffer[0] == 'p') {
			enable_print_flag(&PRINT_PATH, (buffer[1] == 49), "PATH");
		}
		if (buffer[0] == 'm') {
			enable_print_flag(&PRINT_DATA, (buffer[1] == 49), "DATA");
		}
		if (buffer[0] == 'i') {
			enable_print_flag(&PRINT_INFO, (buffer[1] == 49), "INFO");
		}
	} else if (buffer[0] == 'v') {
		if (buffer[1] == 49) {
			VERBOSE = true;
			kfile_printf(&ser->fd, "Verbose mode enabled\n");
		} else {
			VERBOSE = false;
			kfile_printf(&ser->fd, "Verbose mode disabled\n");
		}
	} else if (buffer[0] == 'V') {
		if (buffer[1] == 49) {
			SILENT = true;
			VERBOSE = false;
			kfile_printf(&ser->fd, "Silent mode enabled\n");
		} else {
			SILENT = false;
			kfile_printf(&ser->fd, "Silent mode disabled\n");
		}
	} else if (buffer[0] == 'l' && length > 2) {
		buffer++; length--;
		if (buffer[0] == 'l' && buffer[1] == 'a' && length >= 10) {
			buffer += 2;
			memcpy(latitude, (void *)buffer, 8);
			if (!SILENT && VERBOSE) kprintf("Latitude set to %.8s\n", latitude);
			if (!SILENT && !VERBOSE ) kprintf("1\n");
		} else if (buffer[0] == 'l' && buffer[1] == 'o' && length >= 11) {
			buffer += 2;
			memcpy(longtitude, (void *)buffer, 9);
			if (!SILENT && VERBOSE) kprintf("Longtitude set to %.9s\n", longtitude);
			if (!SILENT && !VERBOSE) kprintf("1\n");
		} else if (buffer[0] == 'p' && length >= 2 && buffer[1] >= 48 && buffer[1] <= 57) {
			power = buffer[1] - 48;
			if (!SILENT && VERBOSE) kprintf("Power set to %dw\n", power*power);
			if (!SILENT && !VERBOSE) kprintf("1\n");
		} else if (buffer[0] == 'h' && length >= 2 && buffer[1] >= 48 && buffer[1] <= 57) {
			height = buffer[1] - 48;
			if (!SILENT && VERBOSE) kprintf("Antenna height set to %ldm AAT\n", (long)(BV(height)*1000L)/328L);
			if (!SILENT && !VERBOSE) kprintf("1\n");
		} else if (buffer[0] == 'g' && length >= 2 && buffer[1] >= 48 && buffer[1] <= 57) {
			gain = buffer[1] - 48;
			if (!SILENT && VERBOSE) kprintf("Gain set to %ddB\n", gain);
			if (!SILENT && !VERBOSE) kprintf("1\n");
		} else if (buffer[0] == 'd' && length >= 2 && buffer[1] >= 48 && buffer[1] <= 57) {
			directivity = buffer[1] - 48;
			if (directivity == 9) directivity = 8;
			if (!SILENT && !VERBOSE) kprintf("1\n");
			if (!SILENT && VERBOSE) {
				if (directivity == 0) kprintf("Dir set to omni\n");
				if (directivity != 0) kprintf("Dir set to %ddeg\n", directivity*45);
			}
		} else if (buffer[0] == 's' && length >= 2) {
			symbol = buffer[1];
			if (!SILENT && VERBOSE) kprintf("Symbol set to %c\n", symbol);
		} else if (buffer[0] == 't' && length >= 2) {
			if (buffer[1] == 'a') {
				symbolTable = '\\';
				if (!SILENT && VERBOSE) kprintf("Selected alt symbol table\n");
			} else {
				symbolTable = '/';
				if (!SILENT && VERBOSE) kprintf("Selected std symbol table\n");
			}
			if (!VERBOSE && !SILENT) kprintf("1\n");
		}


	} else if (buffer[0] == 'm' && length > 1) {
		buffer++; length--;
		if (buffer[0] == 'c' && length > 1) {
			buffer++; length--;
			int count = 0;
			while (length-- && count < 6) {
				char c = buffer[count];
				if (c != 0 && c != 10 && c != 13) {
					message_recip[count] = c;
				} else {
					message_recip[count] = 0x00;
				}
				count++;
			}
			while (count < 6) {
				message_recip[count] = 0x00;
				count++;
			}
			if (!SILENT && VERBOSE) {
				kprintf("Msg rcpt: %.6s", message_recip);
				if (message_recip_ssid != -1) {
					kprintf("-%d\n", message_recip_ssid);
				} else {
					kprintf("\n");
				}
			}
			if (!VERBOSE && !SILENT) kprintf("1\n");
		} else if (buffer[0] == 's' && length > 1) {
			if (length > 2) {
				message_recip_ssid = 10+buffer[2]-48;
			} else {
				message_recip_ssid = buffer[1]-48;
			}
			if (message_recip_ssid < 0 || message_recip_ssid > 15) message_recip_ssid = -1;
			if (!SILENT && VERBOSE) {
				kprintf("Msg rcpt: %.6s", message_recip);
				if (message_recip_ssid != -1) {
					kprintf("-%d\n", message_recip_ssid);
				} else {
					kprintf("\n");
				}
			}
			if (!VERBOSE && !SILENT) kprintf("1\n");
		} else if (buffer[0] == 'r') {
			ss_msgRetry(ctx);
			if (!SILENT && VERBOSE) kprintf("Retried last msg\n");
			if (!VERBOSE && !SILENT) kprintf("1\n");
		} else if (buffer[0] == 'a') {
			if (buffer[1] == 49) {
				message_autoAck = true;
				if (!SILENT && VERBOSE) kprintf("Msg auto-ack enabled\n");
				if (!VERBOSE && !SILENT) kprintf("1\n");
			} else {
				message_autoAck = false;
				if (!SILENT && VERBOSE) kprintf("Msg auto-ack disabled\n");
				if (!VERBOSE && !SILENT) kprintf("1\n");
			}
		}

	} else {
		respond_error("Invalid command\n");
	}


}

void ss_sendPkt(void *_buffer, size_t length, AX25Ctx *ax25) {

    uint8_t *buffer = (uint8_t *)_buffer;

    memcpy(dst.call, DST, 6);
    dst.ssid = DST_SSID;

    memcpy(src.call, CALL, 6);
    src.ssid = CALL_SSID;

    memcpy(path1.call, PATH1, 6);
    path1.ssid = PATH1_SSID;

    memcpy(path2.call, PATH2, 6);
    path2.ssid = PATH2_SSID;

    path[0] = dst;
    path[1] = src;
    path[2] = path1;
    path[3] = path2;

    ax25_sendVia(ax25, path, countof(path), buffer, length);
}

void ss_sendLoc(void *_buffer, size_t length, AX25Ctx *ax25) {
    size_t payloadLength = 20+length;
    bool usePHG = false;
    if (power < 10 && height < 10 && gain < 10 && directivity < 9) {
        usePHG = true;
        payloadLength += 7;
    }
    uint8_t *packet = malloc(payloadLength);
    uint8_t *ptr = packet;
    packet[0] = '=';
    packet[9] = symbolTable;
    packet[19] = symbol;
    ptr++;
    memcpy(ptr, latitude, 8);
    ptr += 9;
    memcpy(ptr, longtitude, 9);
    ptr += 10;
    if (usePHG) {
        packet[20] = 'P';
        packet[21] = 'H';
        packet[22] = 'G';
        packet[23] = power+48;
        packet[24] = height+48;
        packet[25] = gain+48;
        packet[26] = directivity+48;
        ptr+=7;
    }
    if (length > 0) {
        uint8_t *buffer = (uint8_t *)_buffer;
        memcpy(ptr, buffer, length);
    }

    //kprintf("Assembled packet:\n%.*s\n", payloadLength, packet);
    ss_sendPkt(packet, payloadLength, ax25);
    free(packet);
}

void ss_sendMsg(void *_buffer, size_t length, AX25Ctx *ax25) {
    if (length > 67) length = 67;
    size_t payloadLength = 11+length+4;

    uint8_t *packet = malloc(payloadLength);
    uint8_t *ptr = packet;
    packet[0] = ':';
    int callSize = 6;
    int count = 0;
    while (callSize--) {
        if (message_recip[count] != 0) {
            packet[1+count] = message_recip[count];
            count++;
        }
    }
    if (message_recip_ssid != -1) {
        packet[1+count] = '-'; count++;
        if (message_recip_ssid < 10) {
            packet[1+count] = message_recip_ssid+48; count++;
        } else {
            packet[1+count] = 49; count++;
            packet[1+count] = message_recip_ssid-10+48; count++;
        }
    }
    while (count < 9) {
        packet[1+count] = ' '; count++;
    }
    packet[1+count] = ':';
    ptr += 11;
    if (length > 0) {
        uint8_t *buffer = (uint8_t *)_buffer;
        memcpy(ptr, buffer, length);
        memcpy(lastMessage, buffer, length);
        lastMessageLen = length;
    }

    message_seq++;
    if (message_seq > 999) message_seq = 0;

    packet[11+length] = '{';
    int n = message_seq % 10;
    int d = ((message_seq % 100) - n)/10;
    int h = (message_seq - d - n) / 100;

    packet[12+length] = h+48;
    packet[13+length] = d+48;
    packet[14+length] = n+48;
    
    //kprintf("Assembled packet:\n%.*s\n", payloadLength, packet);
    ss_sendPkt(packet, payloadLength, ax25);

    free(packet);
}

void ss_msgRetry(AX25Ctx *ax25) {
    message_seq--;
    ss_sendMsg(lastMessage, lastMessageLen, ax25);
}

#if 0
#define out(fmt,arg...)  kfile_printf(&g_ser->fd, fmt, ##arg)
#else
#define out(fmt,arg...) kprintf(fmt,##arg)
#endif

void ss_printSettings(void) {
	out("TinyAPRS 1.0-r%d/f%d/BG5HHP\n",VERS_BUILD,CONFIG_AFSK_FILTER);
	out("----------------------------------\n");
    out("Configuration:\n");
    out("Callsign: %.6s-%d\n", CALL, CALL_SSID);
    out("Destination: %.6s-%d\n", DST, DST_SSID);
    out("Path1: %.6s-%d\n", PATH1, PATH1_SSID);
    out("Path2: %.6s-%d\n", PATH2, PATH2_SSID);
    if (message_autoAck) {
        out("Auto-ack messages: On\n");
    } else {
        out("Auto-ack messages: Off\n");
    }
    if (power != 10) {out("Power: %d\n", power);}
    if (height != 10) {out("Height: %d\n", height);}
    if (gain != 10) {out("Gain: %d\n", gain);}
    if (directivity != 10) {out("Directivity: %d\n", directivity);}
    if (symbolTable == '\\') {out("Symbol table: alternate\n");}
    if (symbolTable == '/') {out("Symbol table: standard\n");}
    out("Symbol: %c\n", symbol);
}



#if ENABLE_HELP
    void ss_printHelp(void) {
            out("----------------------------------\n");
            out("Serial commands:\n");
            out("!<data>   Send raw packet\n");
            out("@<cmt>    Send location update (cmt = optional comment)\n");
            out("#<msg>    Send APRS message\n\n");

            out("c<call>   Set your callsign\n");
            out("d<call>   Set destination callsign\n");
            out("1<call>   Set PATH1 callsign\n");
            out("2<call>   Set PATH2 callsign\n\n");

            out("sc<ssid>  Set your SSID\n");
            out("sd<ssid>  Set destination SSID\n");
            out("s1<ssid>  Set PATH1 SSID\n");
            out("s2<ssid>  Set PATH2 SSID\n\n");

            out("lla<LAT>  Set latitude (NMEA-format, eg 4903.50N)\n");
            out("llo<LON>  Set latitude (NMEA-format, eg 07201.75W)\n");
            out("lp<0-9>   Set TX power info\n");
            out("lh<0-9>   Set antenna height info\n");
            out("lg<0-9>   Set antenna gain info\n");
            out("ld<0-9>   Set antenna directivity info\n");
            out("ls<sym>   Select symbol\n");
            out("lt<s/a>   Select symbol table (standard/alternate)\n\n");

            out("mc<call>  Set message recipient callsign\n");
            out("ms<ssid>  Set message recipient SSID\n");
            out("mr<ssid>  Retry last message\n");
            out("ma<1/0>   Automatic message ACK on/off\n\n");

            out("ps<1/0>   Print SRC on/off\n");
            out("pd<1/0>   Print DST on/off\n");
            out("pp<1/0>   Print PATH on/off\n");
            out("pm<1/0>   Print DATA on/off\n");
            out("pi<1/0>   Print INFO on/off\n\n");
			
            out("v<1/0>    Verbose mode on/off\n");
            out("V<1/0>    Silent mode on/off\n\n");

            out("S         Save configuration\n");
            out("L         Load configuration\n");
            out("C         Clear configuration\n");
            out("H         Print configuration\n");
            out("----------------------------------\n");
    }
#endif
