SHELL := /usr/bin/sh

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic

FLEX := flex
LEX_SRC := src/lexer.l
LEX_GEN := src/lexer.yy.c

BISON := bison
YACC_SRC := src/parser.y
YACC_GEN_C := src/parser.tab.c
YACC_GEN_H := src/parser.tab.h

C_SRC := src/cpq.c src/compiler.c src/utils.c src/ast.c src/debug.c src/symbols.c src/semantics.c src/quad.c src/quad_utils.c
SRC := $(C_SRC) $(YACC_GEN_C) $(LEX_GEN)

OBJDIR := build/obj
OBJ := $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRC))

.PHONY: all clean

all: $(YACC_GEN_C) $(YACC_GEN_H) $(LEX_GEN) cpq

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(YACC_GEN_C) $(YACC_GEN_H): $(YACC_SRC)
	$(BISON) -d -o $(YACC_GEN_C) $(YACC_SRC)

$(LEX_GEN): $(LEX_SRC) $(YACC_GEN_H)
	$(FLEX) -o $(LEX_GEN) $(LEX_SRC)

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

cpq: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	rm -rf build
	rm -f src/lexer.yy.c src/parser.tab.c src/parser.tab.h cpq