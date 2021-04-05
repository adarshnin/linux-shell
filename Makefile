C = gcc
CFLAGS = -lreadline -o
INPUT = shell
OUT = $(INPUT)

all: $(INPUT)

$(OUT): $(INPUT).c
		$(C) $(INPUT).c $(CFLAGS) $(OUT)