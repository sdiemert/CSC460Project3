#UPLOAD TO ATMEGA WITH: sudo avrdude -p m2560 -c stk500 -P /dev/tty.usbmodem1451 -U flash:w:main.hex
#SEE SERIAL OUTPUT WITH: sudo cu -l /dev/tty.usbmodem1411 -s 9600 OR screen /dev/tty.usbmodem1411 9600

echo "Cleaning..."
rm -f *.o
rm -f *.elf
rm -f *.hex

echo "Copying..."
#cp ../shared/*.* .
#cp ../shared/radio/*.* .
#cp ../shared/roomba/*.* .
#cp ../shared/rtos/*.* .
#cp ../shared/trace_uart/*.* .


echo "Compiling..."

avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega2560 -c -o trace.o trace_uart.c

avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega2560 -c -o radio.o radio.c

avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega2560 -c -o roomba.o roomba.c

avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega2560 -c -o spi.o spi.c

avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega2560 -c -o uart.o uart.c

avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega2560 -c -o ir.o ir.c

avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega2560 -c -o game.o game.c

avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega2560 -c -o mainProg.o $1

avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega2560 -c -o os.o os.c

echo "Linking..."
avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega2560 -g -o out.elf ir.o uart.o roomba.o trace.o spi.o radio.o game.o os.o mainProg.o

echo "Making ELF..."
#avr-gcc -Wall -Os -mmcu=atmega2560 -g -o out.elf out.o


echo "Making HEX..."
avr-objcopy -j .text -j .data -O ihex out.elf main.hex

sudo avrdude -p m2560 -c stk500 -P /dev/tty.usbmodem1451 -U flash:w:main.hex -D
