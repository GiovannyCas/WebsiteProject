########################################################################
####################### Makefile Template ##############################
########################################################################


MKDIR   := mkdir -p
RMDIR   := rm -rf
CC      := clang
BUILD	:= ./build
BIN     := $(BUILD)/bin
OBJ     := $(BUILD)/obj
SRC     := ./src
INCLUDE := $(SRC)/include
SRCS    := $(wildcard $(SRC)/*.c)
OBJS    := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
EXE     := $(BIN)/main.exe
CFLAGS  := -I$(INCLUDE)
LDLIBS  := -lm

.PHONY: all run clean

all: $(EXE)

$(EXE): $(OBJS) | $(BIN)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS): $(INCLUDE)/global.h

$(BIN) $(OBJ):
	$(MKDIR) $@

run: $(EXE)
	$< $(arg)

clean:
	$(RMDIR) $(OBJ) $(BIN)
