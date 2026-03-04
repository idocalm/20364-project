SHELL := /bin/sh

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2

FLEX := flex
LEX_SRC := src/lexer.l
LEX_GEN := src/lexer.yy.c

BISON := bison
YACC_SRC := src/parser.y
YACC_GEN_C := src/parser.tab.c
YACC_GEN_H := src/parser.tab.h

C_SRC := src/cpq.c src/compiler.c src/utils.c src/ast.c src/debug.c
SRC := $(C_SRC) $(YACC_GEN_C) $(LEX_GEN)

OBJDIR := build/obj
OBJ := $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRC))

.PHONY: all clean

all: $(YACC_GEN_C) $(YACC_GEN_H) $(LEX_GEN) cpq.exe

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(YACC_GEN_C) $(YACC_GEN_H): $(YACC_SRC)
	$(BISON) -d -o $(YACC_GEN_C) $(YACC_SRC)

$(LEX_GEN): $(LEX_SRC) $(YACC_GEN_H)
	$(FLEX) -o $@ $<

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

cpq.exe: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	rm -rf build
	rm -f src/lexer.yy.c src/parser.tab.c src/parser.tab.h cpq.exe
