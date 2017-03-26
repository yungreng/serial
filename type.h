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


#define CRC_BYTES 5
#define BOOL unsigned char
#define TRUE  1
#define FALSE  0

/***************************************************************************/
#define VALUE_SZ 1024*4
#define SEND_BUFSZ    VALUE_SZ*2
#define RECV_BUFSZ    VALUE_SZ*2
#define READ_BUFSZ    4096
#define READ_SZ    READ_BUFSZ/4
#define FLAG_BYTES 4
#define HEAD_FLAG '\x55'
#define TAIL_FLAG '\xaa'

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
    int HasFrame;
    int HasDevName;
    int HasNewline;
    int HasId;
    char *Pattern;
    int PatternSZ;
    int crc_bytes;
    int flag_bytes;
    int count;
    char head_flag;
    char tail_flag;
    FILE *logfile;
    FILE *patternFile;
    char *patternFileName;
    char *DevShortName;
    unsigned char packet_buf[READ_BUFSZ];
    int (*calculateCRC)();
    BOOL (*checkCRC)();
    int (*stuffPacket)();
    int (*parsePacket)();
    void (*parsePattern)();
    int(*openPatternFile)();
}Packer;
/***************************************************************************/
typedef struct{
    int sending;
    char *baudStr;
    char *portName;
    char DevFullName[40];
    int (*run)();
    int (*open)();
    void (*parseOption)();
    int (*readDataBlock)();
    int (*openLogFile)();
    unsigned char send_buf[SEND_BUFSZ];
    unsigned char recv_buf[RECV_BUFSZ];
    HANDLE_TYPE hSerial;
    Packer *packer;
}Serial;
/***************************************************************************/
inline void out_head(unsigned char *DevName);
void serial_parseOption(Serial *serial, int argc, char **argv);
int serial_openLogFile(Serial *serial);
int serial_run(Serial*);
int serial_open(Serial*);
int serial_stuffPacket(Serial *serial,int id);

int packer_calculateCRC(unsigned char *pData, int dataLen);
BOOL packer_checkCRC(Packer *pPacker, int dataLen);
int  packer_stuffPacket(Packer *packer,unsigned char *send_buf, int id);
int packer_parsePacket(Packer *packer, unsigned char *recv_buf, int bytesRead);
void packer_parsePattern(Packer *packer, char *optarg);
int packer_openPatternFile(Packer * packer,char *filename);

#ifdef APPLET /* run in multi-call utility */
#define MAIN serial_main
#else/* run in stand-alone utility */
#define MAIN main
#endif /* APPLET */
