# cpq
The compiler is written in C, with bison and flex. 

## Implementation
A few major decisions I made in designing the compiler:

1. The Bison code in `src/parsery.y` builds the AST tree using translation schemes. Obviously this means that a major pile of code is needed to recursively build, free and maintain the AST tree. All that cod eis in `ast.c/.h`

2. I made some tweaks to Bison (And also Flex) so they won't stop after reporting the first error, and will report everything they find. Of course, this does mean that there may be some unwanted artifacts to a simple lexer or parser error, becuase they could make the parser / lexer to also see other parts later as errors as a result... 

3. We will never get to the IR part if a parser / lexer error was found. That also means that error that can only be found at IR stage (e.g. a `break` outside of `while`/`switch`) won't be detected. 

4. The IR works in 2 stages: first we create a `.qud.tmp` file that contains almost the full Quad code, but we use labels. Labels in that file would appear like `LABEL L{i}`. This is needed because can't know yet the final line number where a jump instructions needs to go to, so we jump to a label. Then, we finalize: we go over the `.qud.tmp` and remove `LABEL` definitions, replace jump instructions with the correct lines, and keep everything else. Then we delete the `.qud.tmp` file and remain with `.qud`, the final output.

5. I took some extra effort to make the parser / lexer print messages as close to as gcc does it. So whenever a lexical or parsing error occurres, you will see in the `stderr` the line with highlighting of the exact error. For example:

```
[2026-03-05 02:22:14] [ERROR] line 1:6: syntax error, unexpected TOKEN_INT, expecting TOKEN_COMMA or TOKEN_COLON near 'int'
    a, b int
         ^^^
[2026-03-05 02:22:14] [ERROR] line 3:9: syntax error, unexpected TOKEN_SEMICOLON near ';'
        x = ;
            ^
[2026-03-05 02:22:14] [ERROR] line 5:9: syntax error, unexpected TOKEN_OUTPUT, expecting TOKEN_RPAREN or TOKEN_OR near 'output'
            output
            ^^^^^^
[2026-03-05 02:22:14] [ERROR] found 3 parsing/lexical error(s) in 'tests/test_parser_errors.ou'
```

## How to build and run

I added a `Makefile`, so you can run:

```
make clean
make
```

You do need to have bison and flex installed. I used **MSYS2** on my windows MinGW64 environment. 
After installing MSYS2, open the **MSYS2 MinGW64** terminal and install the required tools:

`pacman -S mingw-w64-x86_64-toolchain flex bison make`

Then you can run `make` (and `make clean`).


### debug mode
While developing the project I found it very useful to be able to print, for example, the AST tree that was built during the parsing stage. So I made the executable run like this:

`cpq [--debug] <filename>.ou`

If given `--debug`, you will see a log that says you enabled debug mode. The only 2 differences when working in debug mode is:

1. The AST tree will be printed to you after the parser finished (if there are no errors, ofc)
2. The `.qud.tmp` file mentioned before will not be deleted, so you could see before and after removing the labels from the qud code
