TARGET = updater
SRC = main.c firmware_blob.c

all: $(TARGET).prg

$(TARGET).prg: $(SRC)
	cl65 -t c64 -O -Cl -Os -o $@ $(SRC)

clean:
	rm -f $(TARGET).prg $(TARGET).o main.o firmware_blob.o firmware_blob.c firmware_blob.h	