
#include "type.h"


/***************************************************************************/
struct termios stdin_tio_org;
void setup_terminal(void)
{
	struct termios stdin_tio;
	if ( tcgetattr( fileno( stdin ), &stdin_tio_org ) < 0 )
	{
		fprintf( stderr, "Unable to get terminal settings: %s\n", strerror( errno ));
		exit( 12 );
	}
	stdin_tio = stdin_tio_org;
	stdin_tio.c_lflag &= ~( ICANON | ECHO );
	stdin_tio.c_cc[VTIME] = 0;
	stdin_tio.c_cc[VMIN] = 1;

	// Put stdin in raw mode, avoid waiting for RETURN in  Canonical mode
	if ( tcsetattr( fileno( stdin ), TCSANOW, &stdin_tio ) < 0 )
	{
		fprintf( stderr, "Unable to setup terminal settings: %s\n", strerror( errno ));
		exit( 11 );
	}
}

/***************************************************************************/
void restore_terminal(void)
{
	if ( tcsetattr( fileno( stdin ), TCSANOW, &stdin_tio_org ) < 0 )
	{
		fprintf( stdout, "Unable to restore terminal settings: %s\n", strerror( errno ));
		exit( 10 );
	}
}

/***************************************************************************/
int serial_run(Serial *serial)
{
	int r;
	int sig;
	sigset_t termSig;
	pthread_t readThreadId;
	pthread_t keyThreadId;
	pthread_t writeThreadId;

	setbuf( stdin, NULL );
	setbuf( stdout, NULL );


	setup_terminal();
	r = pthread_create( &keyThreadId, NULL, KeyThread, serial);
	if ( r != 0 )
	{
		fprintf( stdout, "Error creating KeyThread: %s\n", strerror( r ));
		exit( 7 );
	}
	r = pthread_create( &readThreadId, NULL, ReadThread, serial);
	if ( r != 0 )
	{
		fprintf( stdout, "Error creating ReadThread: %s\n", strerror( r ));
		exit( 8 );
	}

	r = pthread_create( &writeThreadId, NULL, WriteThread, serial);
	if ( r != 0 )
	{
		fprintf( stdout, "Error creating WriteThread: %s\n", strerror( r ));
		exit( 9 );
	}


	sigemptyset( &termSig );
	sigaddset( &termSig, SIGINT );
	sigaddset( &termSig, SIGTERM );
	pthread_sigmask( SIG_BLOCK, &termSig, NULL );

	if (( r = sigwait( &termSig, &sig )) != 0 )
	{
		fprintf( stdout, "sigwait failed\n" );
	}
	else
	{
		fprintf( stdout, "Exiting...\n" );
	}

	pthread_cancel( readThreadId );
	pthread_cancel( writeThreadId);
	pthread_cancel( keyThreadId);



	pthread_sigmask( SIG_UNBLOCK, &termSig, NULL );

	pthread_join( readThreadId, NULL );
	pthread_join( writeThreadId, NULL );
	pthread_join( keyThreadId, NULL );

	close( serial->hSerial);
	restore_terminal();
	return 1;
}



/***************************************************************************/
struct{
	speed_t speed;
	unsigned int baudRate;
} gBaudTable[] =
{
	{ B50, 50 },
	{ B75, 75 },
	{ B110, 110 },
	{ B134, 134 },
	{ B150, 150 },
	{ B200, 200 },
	{ B300, 300 },
	{ B600, 600 },
	{ B1200, 1200 },
	{ B1800, 1800 },
	{ B2400, 2400 },
	{ B4800, 4800 },
	{ B9600, 9600 },
	{ B19200, 19200 },
	{ B38400, 38400 },
	{ B57600, 57600 },
	{ B115200, 115200 },
	{ B230400, 230400 },
};

#define ARRAY_SZ(x) ( sizeof( x ) / sizeof( x[ 0 ]))
/***************************************************************************/
int serial_open(Serial *serial)
{
	speed_t baudRate;
	sigset_t termSig;

	struct termios attr;


	baudRate = B0;
	if ( serial->baudStr == NULL )
	{
		baudRate = B9600;
	}
	else
	{
		int i;
		int reqBaudRate = atoi( serial->baudStr );

		for ( i = 0; i < ARRAY_SZ( gBaudTable ); i++ )
		{
			if ( gBaudTable[ i ].baudRate == reqBaudRate )
			{
				baudRate = gBaudTable[ i ].speed;
				break;
			}
		}

		if ( baudRate == B0 )
		{
			fprintf( stderr, "Unrecognized baud rate: '%s'\n", serial->baudStr );
			exit( 1 );
		}
	}


	/*!!!!!!!!!!!!!!!!!!! Important: change into non-block mode !!!!!!!!!!!!!!!!!!!!*/
	if (( serial->hSerial = open( serial->DevFullName, O_RDWR | O_EXCL | O_NONBLOCK )) < 0 )
	{
		fprintf( stderr, "Unable to open serial port '%s': %s\n", serial->DevFullName, strerror( errno ));
		exit( 2 );
	}

	/*!!!!!!!!!!!!!!!!!!! Important: change into non-block mode !!!!!!!!!!!!!!!!!!!!*/
	fcntl( serial->hSerial, F_SETFL, fcntl( serial->hSerial, F_GETFL ) & ~O_NONBLOCK );

	if ( tcgetattr( serial->hSerial, &attr ) < 0 )
	{
		fprintf( stderr, "Call to tcgetattr failed: %s\n", strerror( errno ));
		exit( 3 );
	}


	/*!!!!!!!!!!!!!!!!!!! Important: change into rawmode !!!!!!!!!!!!!!!!!!!!*/
	cfmakeraw(&attr);
	attr.c_cflag |= ( CLOCAL | CREAD );

	cfsetispeed( &attr, baudRate );
	cfsetospeed( &attr, baudRate );

	if ( tcsetattr( serial->hSerial, TCSAFLUSH, &attr ) < 0 )
	{
		fprintf( stderr, "Call to tcsetattr failed: %s\n", strerror( errno ));
		exit( 4 );
	}

	return 0;
}

/***************************************************************************/
int serial_setRTS(Serial *serial)
{
	int ctrlbits;
	ioctl(serial->hSerial,TIOCMGET,&ctrlbits);
	ctrlbits |= TIOCM_RTS;
	//ctrlbits &= ~TIOCM_RTS;
	ioctl(serial->hSerial, TIOCMSET,&ctrlbits);
	return 0;
}
/***************************************************************************/
int serial_clearRTS(Serial *serial)
{
	int ctrlbits;
	ioctl(serial->hSerial,TIOCMGET,&ctrlbits);
	ctrlbits &= ~TIOCM_RTS;
	ioctl(serial->hSerial, TIOCMSET,&ctrlbits);
	return 0;
}
/***************************************************************************/
void *ReadThread( void *param )
{
	Serial *serial =  param;
	Packer *packer = serial->packer;
	int bytesRead = 0;
	out_head(packer->DevShortName);
	while(1){
		if (( bytesRead = read( serial->hSerial, serial->recv_buf, READ_SZ)) < 0 ){
			fprintf( stdout, "Serial read failed: %s\n", strerror( errno ));
			continue;
		}
		packer->parsePacket(packer, serial->recv_buf, bytesRead);
	}
	return NULL;

}
/***************************************************************************/
void *WriteThread( void *param )
{
	Serial *serial = param;
	Packer *packer = serial->packer;
	int packetId = 0,packet_size;
	usleep(1500000);//wait for friend port ready
	while ( 1 ) {
		if (!serial->sending)
			continue;
		packet_size = packer->stuffPacket(packer, serial->send_buf, ++packetId );
		/* send packet and flush out.... */
		if (write( serial->hSerial, serial->send_buf, packet_size ) < 0 ){
			fprintf( stderr, "write to serial port failed: %s\n", strerror( errno ));
		}else{
			fsync(serial->hSerial);
		}
		if (packetId == packer->count ){
			printf("stop sending....\n");
			serial->sending= FALSE;
			break;
		}
		usleep(1000);/* pause for 1ms */
	}
	return NULL;
}

/***************************************************************************/
void * KeyThread(void * param)
{
	Serial * serial = param;
	int keys_fd;
	struct input_event t;
	keys_fd = open ("/dev/input/event0", O_RDONLY);
	if (keys_fd <= 0){
		printf ("open /dev/input/event0 device error!\n");
		return 0;
	}
	if ( serial->sending == TRUE)
		serial->setRTS(serial);
	else
		serial->clearRTS(serial);
	while(1){
		if (read (keys_fd, &t, sizeof (t)) == sizeof (t)){
			if (t.type != EV_KEY  && t.value != 1)
				continue;
			switch (t.code){
				case KEY_ESC:
					{
						restore_terminal();
						exit(0);
						break;
					}
				case KEY_T:
					{
						serial->setRTS(serial);
						serial->sending = TRUE;
						break;
					}
				case KEY_Q:
					{
						serial->clearRTS(serial);
						serial->sending = FALSE;
						break;
					}
			}
		}
	}
	close(keys_fd);
	return NULL;
}


