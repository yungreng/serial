# serial
Usage:	 ./serial [option]

	 -b baudrate, Set baudrate(9600,115200 etc.) 
	 -p ttySx, Set port name to  ttySx
	 -P string, Set pattern from file if string happened being a file, 
		or as "x:nn:nn:..." to start with"x:" to input hex,
		otherwise ascii text 
	 -t, no sending at start
	 -n, no new_line
	 -i, no id number
	 -c count, send times before stop 
	 -d, no device name
	 -v, no CRC&packet 
	 -?h, this usage
