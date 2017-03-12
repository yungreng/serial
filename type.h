/* ---- Include Files ---------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>

#ifdef WINDOWS
#include <windows.h>
#include <sys/timeb.h>
#endif /*WINDOWS*/

#ifdef LINUX
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <pthread.h>
#include <signal.h>
#include <termios.h>
#include <linux/input.h>
#endif /*LINUX*/


#define CRC_BYTES 4
#define BOOL unsigned char
#define TRUE  1
#define FALSE  0

/***************************************************************************/
#define SEND_BUFSZ    4096
#define RECV_BUFSZ    4096
#define READ_BUFSZ    4096
#define WANTED  64
#define HEAD_FLAG '\x55'
#define TAIL_FLAG '\x5a'
#define FLAG_BYTES 4
#define CRC_BYTES 4

#ifdef LINUX
void* ReadThread( void* param );
void* KeyThread(void* param);
void* WriteThread( void* param );
#define DIR_SYM '/'
#define DEV_FORMAT_STR "/dev/%s"
#define DEFAULT_PORT "ttyUSB0"
#define DEFAULT_PORTX "ttySx"
#define HANDLE_TYPE int
#endif

#ifdef WINDOWS
DWORD WINAPI ReadThread( void* param );
DWORD WINAPI WriteThread( void* param );
DWORD WINAPI KeyThread(void* param);
#define DIR_SYM '\\'
#define DEV_FORMAT_STR "\\\\.\\%s"
#define DEFAULT_PORT "COM3"
#define DEFAULT_PORTX "COMx"
#define HANDLE_TYPE HANDLE
#endif


/***************************************************************************/
typedef struct{
    int crc_bytes;
    int (*calculateCRC)();
    BOOL (*checkCRC)();
}Crc;
/***************************************************************************/
typedef struct{
    int sending;
    int HasFrame;
    int HasSenderName;
    int HasNewline;
    int HasId;
    char *Pattern;
    char *baudStr;
    char *portName;
    char DevFullName[40];
    char *DevShortName;
    int (*run)();
    int (*open)();
    void (*parseOption)();
    int (*readDataBlock)();
    int (*openLogFile)();
    int (*stuffPacket)();
    unsigned char send_buf[SEND_BUFSZ];
    unsigned char recv_buf[RECV_BUFSZ];
    HANDLE_TYPE hSerial;
    FILE *logfile;
    Crc *pVerifier;
}Serial;
/***************************************************************************/
inline void OUT_HEAD(Serial *serial);
void serial_parseOption(Serial *serial, int argc, char **argv);
int serial_openLogFile(Serial *serial);
int serial_run(Serial*);
int serial_open(Serial*);
int serial_readDataBlock(Serial *serial, int bytesRead);
int serial_stuffPacket(Serial *serial,int id);

int Crc_calculateCRC(unsigned char *pData, int dataLen);
BOOL Crc_checkCRC(Crc *crc, unsigned char *pData, int dataLen);

#ifdef APPLET /* run in multi-call utility */
#define MAIN serial_main
#else/* run in stand-alone utility */
#define MAIN main
#endif /* APPLET */
