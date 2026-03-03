SHELL := /bin/sh

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2

FLEX := flex
LEX_SRC := src/lexer.l
LEX_GEN := src/lexer.yy.c

C_SRC := src/cpq.c src/compiler.c src/utils.c
SRC := $(C_SRC) $(LEX_GEN)

OBJDIR := build/obj
OBJ := $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRC))

.PHONY: all clean

all: $(LEX_GEN) cpq.exe

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(LEX_GEN): $(LEX_SRC)
	$(FLEX) -o $@ $<

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

cpq.exe: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	rm -rf build
	rm -f src/lexer.yy.c cpq.exe