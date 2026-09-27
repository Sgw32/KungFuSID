TARGET = updater_demo

CC = cl65

CFLAGS = -t c64 -O -Cl -Os
LDFLAGS = -t c64

all: $(TARGET).prg

$(TARGET).prg: $(TARGET).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET).prg $(TARGET).o