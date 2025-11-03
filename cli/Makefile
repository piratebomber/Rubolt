CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

TARGET = rbcli
SOURCE = rbcli.c

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe

install: $(TARGET)
	@echo "Installing rbcli..."
	@cp $(TARGET) ../$(TARGET)
	@echo "✓ rbcli installed successfully!"
	@echo "Run: ./rbcli help"
