
linux:
	gcc -o serial main.c packet.c serial_linux.c   -lpthread -DLINUX
	sudo ./serial -p ttyUSB0 -b 115200

CROSS_COMPILE=i686-w64-mingw32-
windows:
	$(CROSS_COMPILE)gcc -o serial.exe -m32 main.c packet.c serial_windows.c -DWINDOWS
	$(CROSS_COMPILE)strip  serial.exe
	serial.exe -p COM3 -b 115200

