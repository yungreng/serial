#include "type.h"

int serial_run(Serial *serial)
{
    HANDLE rHandle = CreateThread(NULL, 0, ReadThread, serial, 0, NULL);
    if (rHandle == INVALID_HANDLE_VALUE){
        fprintf(stdout,"create read thread failed!\nexit...\n");
        exit(0);
    }
    HANDLE wHandle = CreateThread(NULL, 0, WriteThread, serial, 0, NULL);
    if (wHandle == INVALID_HANDLE_VALUE){
        fprintf(stdout,"create write thread failed!\nexit...\n");
        exit(0);
    }
    HANDLE kHandle = CreateThread(NULL, 0, KeyThread, serial, 0, NULL);
    if (wHandle == INVALID_HANDLE_VALUE){
        fprintf(stdout,"create key  thread failed!\nexit...\n");
        exit(0);
    }
    WaitForSingleObject(rHandle, INFINITE);
    WaitForSingleObject(wHandle, INFINITE);
    fclose( serial->hSerial );

    return 1;
}


int serial_open(Serial *serial)
{

    DCB dcb ;
    COMMTIMEOUTS CommTimeOuts;
    serial->hSerial = CreateFile(serial->DevFullName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING, //
            FILE_FLAG_OVERLAPPED, //non block mode
            NULL
            );

    if (INVALID_HANDLE_VALUE == serial->hSerial){
        fprintf(stdout,"error:unable to open %d",serial->packer->DevShortName);
        exit(1);
        return 0;
    }

    SetupComm(serial->hSerial,1024*64,1024*64); //setup in&out buffer size
    CommTimeOuts.ReadIntervalTimeout = 500;
    CommTimeOuts.ReadTotalTimeoutMultiplier = 10;
    CommTimeOuts.ReadTotalTimeoutConstant = 500;
    CommTimeOuts.WriteTotalTimeoutMultiplier = 1000;
    CommTimeOuts.WriteTotalTimeoutConstant = 100;
    SetCommTimeouts(serial->hSerial, &CommTimeOuts);

    GetCommState(serial->hSerial, &dcb);
    dcb.BaudRate = atoi(serial->baudStr);
    dcb.StopBits =ONESTOPBIT;
    dcb.Parity = NOPARITY;
    dcb.ByteSize = 8;
    dcb.fBinary = TRUE;

    if(!SetCommState(serial->hSerial, &dcb)){
        return 0;
    }
    //clear device buffer before using
    PurgeComm( serial->hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
    return 0;
}


#define rTIMEOUT 1000
/***************************************************************************/
DWORD WINAPI ReadThread( void *param )
{
    Serial *serial =  param;
    Packer *packer = serial->packer;
    DWORD bytesRead = 0;
    out_head(packer->DevShortName);
    while(1){
        OVERLAPPED osRead = {0};
        osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if ( !ReadFile(serial->hSerial, serial->recv_buf, READ_SZ, &bytesRead, &osRead) ){
            //loop until success
            while(1){
                Sleep(0);
                if (WaitForSingleObject(osRead.hEvent, rTIMEOUT) == WAIT_OBJECT_0) {
                    if (GetOverlappedResult(serial->hSerial, &osRead, &bytesRead, FALSE))
                        break;
                }
            }
        }
        packer->parsePacket(packer, serial->recv_buf, bytesRead);
        CloseHandle(osRead.hEvent);
    }
    return 0;

}

DWORD WINAPI WriteThread( void *param )
{
    Serial *serial = param;
    Packer *packer = serial->packer;
    int packetId = 0;
    int packet_size;
    if (!serial->sending)
	EscapeCommFunction(serial->hSerial,CLRRTS);
    else
	EscapeCommFunction(serial->hSerial,SETRTS);
    Sleep(1500);
    while ( 1 ) {
        if (!serial->sending)
            continue;
        /* sending  packet.... */
        packet_size = packer->stuffPacket(packer, serial->send_buf, ++packetId );
        DWORD byteWriten;
        OVERLAPPED osWrite = {0};
        osWrite.hEvent= CreateEvent( NULL, TRUE, FALSE, NULL);
        if (!WriteFile( serial->hSerial, serial->send_buf, packet_size,&byteWriten, &osWrite)){
            while(1){ //waiting for success writing
                Sleep(0);
                if (WaitForSingleObject(osWrite.hEvent, INFINITE) == WAIT_OBJECT_0) {
                    if (GetOverlappedResult(serial->hSerial, &osWrite, &byteWriten, FALSE)){
                        CloseHandle(osWrite.hEvent);
                        break;
                    }
                }
            }
        }
        if (packetId == packer->count ){
            printf("stop sending....\n");
            serial->sending= FALSE;
            break;
        }
        Sleep(1);

    }
    return 0;
}

DWORD WINAPI  KeyThread(void* param)
{

    Serial * serial = param;
    while (1){
        switch (getch()){
            case '\x1b':
                exit(0);
                break;
            case 't':
            case 'T':
                serial->sending = TRUE;
		EscapeCommFunction(serial->hSerial,SETRTS);
                break;
            case 'q':
            case 'Q':
                serial->sending = FALSE;
		EscapeCommFunction(serial->hSerial,CLRRTS);
                break;
        }
    }
    return 0;

}

