TARGET = a.out
SRC = job_control.c shell.c
CC = gcc
CFLAGS = -Wall
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
