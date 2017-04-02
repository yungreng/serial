/****************************************************************************
 *
 * A debug program for serial port on windows & Linux.
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
    /* show bytes in  buffer  */
    for (i=0; i < bufLen; i++){
        fputc(buf[i],logfile);
    }
    fprintf(logfile, "\n");
    fflush(logfile);
    return 0;
}
Packer gPacker = {
    .HasFrame = TRUE,
    .HasDevName = TRUE,
    .HasNewline = TRUE,
    .HasId = TRUE,
    .Pattern = "0123456789abcdef",
    .PatternSZ = sizeof("0123456789abcdef"),
    .crc_bytes = CRC_BYTES,
    .flag_bytes = FLAG_BYTES,
    .count = 0,
    .head_flag = HEAD_FLAG,
    .tail_flag = TAIL_FLAG,
    .calculateCRC = packer_calculateCRC,
    .checkCRC = packer_checkCRC,
    .stuffPacket = packer_stuffPacket,
    .parsePacket = packer_parsePacket,
    .parsePattern = packer_parsePattern,
};
Serial gSerial  = {
    .packer = &gPacker,
    .sending = TRUE,
    .baudStr = "9600",
    .portName = DEFAULT_PORT,
    .open = serial_open,
    .openLogFile = serial_openLogFile,
    .run = serial_run,
    .parseOption = serial_parseOption,
};
/***************************************************************************/
int  serial_usage(char *bin_name)
{
    printf( "Usage: %s [option]\n",bin_name );
    printf( "\n" );
    printf( "\t -b baudrate, Set baudrate(9600,115200 etc.) \n" );
    printf( "\t -p %s, Set port name to  %s\n", DEFAULT_PORTX, DEFAULT_PORTX  );
    printf( "\t -P string, Set pattern from file if string happened being a file, \n\t\tor as \"x:nn:nn:...\" to start with\"x:\" to input hex,\n\t\totherwise ascii text \n" );
    printf( "\t -t, no sending at start\n" );
    printf( "\t -n, no new_line\n" );
    printf( "\t -i, no id number\n" );
    printf( "\t -c count, send times before stop \n" );
    printf( "\t -d, no device name\n" );
    printf( "\t -v, no CRC&packet \n" );
    printf( "\t -?h, this usage\n");
    return 0;
}
/***************************************************************************/
int serial_openLogFile(Serial *serial)
{
    int result = 0;
    char err_file_name[64];
    sprintf(err_file_name, "_%s.log", serial->packer->DevShortName);
    serial->packer->logfile = fopen(err_file_name, "w+");
    if (serial->packer->logfile == NULL){
        fprintf(stdout, "Err:open log file %s failed!", err_file_name);
        exit(1);
    }
    result = 1;
    return result;
}

void serial_parseOption(Serial *serial, int argc, char **argv)
{
    int opt;
    Packer *packer = serial->packer;
    while (( opt = getopt( argc, argv, "b:p:P:c:vdnith?")) > 0 ){
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
                    //printf("parse pattern: %s\n\n",optarg);
                    packer->parsePattern(packer, optarg);
                    break;
                }
            case 'c':
                {
                    packer->count = atoi(optarg);
                    break;
                }
            case 'n':
                {
                    packer->HasNewline = FALSE;
                    break;
                }
            case 'd':
                {
                    packer->HasDevName = FALSE;
                    break;
                }
            case 'i':
                {
                    packer->HasId = FALSE;
                    break;
                }
            case 'v':
                {
                    packer->HasFrame = FALSE;
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
    packer->DevShortName = strrchr(serial->DevFullName,DIR_SYM) + 1;

}


/***************************************************************************/
//int serial_readDataBlock(Serial *serial, int bytesRead)
//{
//    int i;
//    static packet_cnt=0;
//    static unsigned char packet_buf[READ_BUFSZ];
//    static unsigned char last_char;
//    static flag_cnt = 0;
//    static BOOL start = FALSE;
//    static int recv_cnt = 0;
//    static int err_cnt = 0;
//    Packer *packer  = serial->packer;
//    char *pRead = serial->recv_buf; /* receive buffer head */
//    for (i=0;i<bytesRead;i++){
//        if (packer->HasFrame == FALSE){ /* no packet & CRC */
//            fputc(*pRead,stdout);
//            pRead++;
//            continue;
//        }
//        /* check out packet head*/
//        if ( start == FALSE ){
//            if ( *pRead == packer->head_flag){
//                flag_cnt ++;
//            }else{
//                if (--flag_cnt <= 0)
//                    flag_cnt = 0;
//                if (flag_cnt == FLAG_BYTES-1){/* end of FLAG_BYTES 'head' */
//                    flag_cnt = 0;
//                    start = TRUE;
//                    packet_buf[recv_cnt]= *pRead;
//                    recv_cnt++;
//                }
//            }
//            last_char = *pRead;
//            pRead++;
//            continue;
//        }
//
//        if ( *pRead == packer->tail_flag){ /* check out packet tail */
//            flag_cnt++;
//        }else{
//            if (--flag_cnt < 0){
//                flag_cnt = 0;
//            }
//            if  (flag_cnt == FLAG_BYTES - 1) { /* end of FLAG_BYTES  'tail' */
//                start = FALSE;
//                flag_cnt = 0;
//                fprintf(stdout,"Rx%d____",++packet_cnt);
//                OUT_HEAD(serial->DevShortName);
//                if(!packer->checkCRC(packer, packet_buf, recv_cnt-FLAG_BYTES)){
//                    log_error(serial->logfile,++err_cnt,packet_buf, recv_cnt-FLAG_BYTES);
//                }
//                /*this maybe start of 'head'*/
//                if ( *pRead == packer->head_flag)
//                    flag_cnt ++;
//                recv_cnt = 0;
//                last_char = *pRead;
//                pRead++;
//                continue;
//            }
//        }
//        /* get content of packet */
//        packet_buf[recv_cnt]= *pRead;
//        recv_cnt++;
//        last_char = *pRead;
//        pRead++;
//    }
//    return 0;
//}

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


