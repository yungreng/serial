/****************************************************************************
 *
 * This implements a sample program for accessing the serial port.
 *
 *****************************************************************************/


#include "type.h"

/***************************************************************************/
int stamp(FILE* logfile)
{
    time_t tt;
    char str[100];
    tt = time(NULL);
    strftime(str, sizeof(str),"%Y-%m-%d %H:%M:%S",localtime(&tt));
#ifdef LINUX
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv,&tz);
    fprintf(logfile,"%s.%03d", str, tv.tv_usec/1000);
#endif /*LINUX*/
#ifdef WINDOWS
    struct  timeb   tp;
    ftime(&tp);
    fprintf(logfile,"%s.%03d", str, tp.millitm);
#endif /*WINDOWS*/
    return 0;

}
int log_error(FILE *logfile,int id,unsigned char * buf, int bufLen)
{
    int i;
    fprintf(logfile, "err#%d @",id);
    stamp(logfile);
    fprintf(logfile, ":\t\t");
    /* out buffer */
    for (i=0; i < bufLen; i++){
        fputc(buf[i],logfile);
    }
    fprintf(logfile, "\n");
    fflush(logfile);
    return 0;
}
Crc gVerifier = {
    .crc_bytes = CRC_BYTES,
    .calculateCRC = Crc_calculateCRC,
    .checkCRC = Crc_checkCRC,
};
Serial gSerial  = {
    .pVerifier = &gVerifier,
    .sending = TRUE,
    .HasFrame = TRUE,
    .HasSenderName = TRUE,
    .HasNewline = TRUE,
    .HasId = TRUE,
    .Pattern = "0123456789abcdef",
    .baudStr = "9600",
    .portName = DEFAULT_PORT,
    .open = serial_open,
    .openLogFile = serial_openLogFile,
    .run = serial_run,
    .parseOption = serial_parseOption,
    .stuffPacket = serial_stuffPacket,
    .readDataBlock = serial_readDataBlock,
};
/***************************************************************************/
int  serial_usage(char *bin_name)
{
    fprintf( stdout, "Usage: %s[option(s)]\n",bin_name );
    fprintf( stdout, "\n" );
    fprintf( stdout, "\t -b baudrate, Set baudrate(9600,115200 etc.) \n" );
    fprintf( stdout, "\t -p %s, Set port name of  %s\n", DEFAULT_PORTX, DEFAULT_PORTX  );
    fprintf( stdout, "\t -P string, Set pattern string \n" );
    fprintf( stdout, "\t -t, no sending at start\n" );
    fprintf( stdout, "\t -n, no new_line\n" );
    fprintf( stdout, "\t -i, no id number\n" );
    fprintf( stdout, "\t -s, no sender name\n" );
    fprintf( stdout, "\t -c, no CRC&packet \n" );
    fprintf( stdout, "\t -?h, this usage\n");
    return 0;
}

/***************************************************************************/
int serial_openLogFile(Serial *serial)
{
    int result = 0;
    char err_file_name[64];
    sprintf(err_file_name, "err_%s.log", serial->DevShortName);
    //stderr = fopen(err_file_name, "w+");
    serial->logfile = fopen(err_file_name, "w+");
    if (serial->logfile == NULL){
        fprintf(stdout, "Err:open log file %s failed!", err_file_name);
        exit(2);
    }
    result = 1;
    return result;
}

void serial_parseOption(Serial *serial, int argc, char **argv)
{
    int opt;
    while (( opt = getopt( argc, argv, "b:p:P:csnith?")) > 0 ){
        switch ( opt ){
            case 't':
                {
                    serial->sending= FALSE;
                    break;
                }
            case 'b':
                {
                    serial->baudStr = optarg;
                    break;
                }
            case 'p':
                {
                    serial->portName = optarg;
                    break;
                }
            case 'P':
                {
                    serial->Pattern = optarg;
                    break;
                }
            case 'n':
                {
                    serial->HasNewline = FALSE;
                    break;
                }
            case 's':
                {
                    serial->HasSenderName = FALSE;
                    break;
                }
            case 'i':
                {
                    serial->HasId = FALSE;
                    break;
                }
            case 'c':
                {
                    serial->HasFrame = FALSE;
                    break;
                }
            case '?':
            case 'h':
                {
                    serial_usage(argv[0]);
                    exit(0);
                    break;
                }
            default:
                break;
        }
    }

    if ( serial->portName[ 0 ] != DIR_SYM)
    {
        sprintf(serial->DevFullName, DEV_FORMAT_STR, serial->portName);
    }
    serial->DevShortName = strrchr(serial->DevFullName,DIR_SYM) + 1;

}

/***************************************************************************/
int  serial_stuffPacket(Serial *serial,int id)
{
    int i;
    unsigned short crc_value;
    unsigned char *pSend = serial->send_buf;
    unsigned char *pHead = &serial->send_buf[FLAG_BYTES];
    if (serial->HasFrame == TRUE){
        for (i=0;i<FLAG_BYTES;i++){ /* stuff packet head */
            *pSend  = HEAD_FLAG;
            pSend ++;
        }
    }
    if (serial->HasSenderName){ /* stuff sender name */
        sprintf(pSend,"%s :",serial->DevShortName);
        pSend += strlen(pSend);
    }
    if (serial->HasId){ /* stuff id */
        sprintf(pSend," %d\t",id);
    }
    pSend += strlen(pSend);
    if (strlen(serial->Pattern)>0){ /* stuff pattern string */
        sprintf(pSend,"%s",serial->Pattern);
        pSend += strlen(pSend);
    }
    if (serial->HasNewline){ /* stuff char of new_line */
        sprintf(pSend,"%s","\n");
        pSend += strlen(pSend);
    }
    *pSend = '\0';/* add end_flag */
    pSend++;
    if (serial->HasFrame == TRUE){ /* stuff CRC & packet tail */
        /* stuff CRC value of HEX text format */
        crc_value= serial->pVerifier->calculateCRC(pHead,(int)(pSend - pHead));
        sprintf(pSend,"%04x",crc_value);
        pSend += strlen(pSend);
        /* stuff packet tail */
        for (i=0;i<FLAG_BYTES;i++){
            *pSend = TAIL_FLAG;
            pSend++;
        }
    }
    return (int)(pSend - serial->send_buf);
}


/***************************************************************************/
inline void OUT_HEAD(Serial *serial)
{
    fprintf(stdout,"%s<<<<<",serial->DevShortName);
}
/***************************************************************************/
int serial_readDataBlock(Serial *serial, int bytesRead)
{
    int i;
    static unsigned char packet_buf[READ_BUFSZ];
    static unsigned char last_char;
    static flag_cnt = 0;
    static BOOL start = FALSE;
    static int recv_cnt = 0;
    static int err_cnt = 0;
    char *pRead = serial->recv_buf; /* receive buffer head */
    for (i=0;i<bytesRead;i++){
        if (serial->HasFrame == FALSE){ /* no packet & CRC */
            fputc(*pRead,stdout);
            pRead++;
            continue;
        }
        /* check out packet head*/
        if ( start == FALSE ){
            if ( *pRead == HEAD_FLAG ){
                flag_cnt ++;
            }else{
                if (--flag_cnt <= 0)
                    flag_cnt = 0;
                if (flag_cnt == FLAG_BYTES-1){/* end of FLAG_BYTES 'head' */
                    flag_cnt = 0;
                    start = TRUE;
                    packet_buf[recv_cnt]= *pRead;
                    recv_cnt++;
                }
            }
            last_char = *pRead;
            pRead++;
            continue;
        }

        if ( *pRead == TAIL_FLAG){ /* check out packet tail */
            flag_cnt++;
        }else{
            if (--flag_cnt < 0){
                flag_cnt = 0;
            }
            if  (flag_cnt == FLAG_BYTES - 1) { /* end of FLAG_BYTES  'tail' */
                start = FALSE;
                flag_cnt = 0;
                OUT_HEAD(serial);
                if(!serial->pVerifier->checkCRC(serial->pVerifier, packet_buf, recv_cnt-FLAG_BYTES)){
                    log_error(serial->logfile,++err_cnt,packet_buf, recv_cnt-FLAG_BYTES);
                }
                /*this maybe start of 'head'*/
                if ( *pRead == HEAD_FLAG )
                    flag_cnt ++;
                recv_cnt = 0;
                last_char = *pRead;
                pRead++;
                continue;
            }
        }
        /* get content of packet */
        packet_buf[recv_cnt]= *pRead;
        recv_cnt++;
        last_char = *pRead;
        pRead++;
    }
    return 0;
}

/***************************************************************************/
//int main( int argc, char **argv )
int MAIN( int argc, char **argv )
{

    Serial *serial = &gSerial;
    serial->parseOption(serial, argc, argv);
    serial->open(serial);
    serial->openLogFile(serial);
    serial->run(serial);
    return 1;
}


