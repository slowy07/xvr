/**
MIT License

Copyright (c) 2025 arfy slowy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "xvr_lexer.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_keyword_types.h"
#include "xvr_token_types.h"

static void cleanLexer(Xvr_Lexer* lexer) {
    lexer->source = NULL;
    lexer->start = 0;
    lexer->current = 0;
    lexer->line = 1;
}

static bool isAtEnd(Xvr_Lexer* lexer) {
    return lexer->source[lexer->current] == '\0';
}

static char peek(Xvr_Lexer* lexer) { return lexer->source[lexer->current]; }

static char peekNext(Xvr_Lexer* lexer) {
    if (isAtEnd(lexer)) return '\0';
    return lexer->source[lexer->current + 1];
}

static char advance(Xvr_Lexer* lexer) {
    if (isAtEnd(lexer)) {
        return '\0';
    }

    // new line
    if (lexer->source[lexer->current] == '\n') {
        lexer->line++;
    }

    lexer->current++;
    return lexer->source[lexer->current - 1];
}

static void eatWhitespace(Xvr_Lexer* lexer) {
    const char c = peek(lexer);

    switch (c) {
    case ' ':
    case '\r':
    case '\n':
    case '\t':
        advance(lexer);
        break;

    // INFO: add shebang feature
    case '#':
        if (lexer->start == 0 && peekNext(lexer) == '!') {
            // eat the entire shebang line
            while (peek(lexer) != '\n' && !isAtEnd(lexer)) {
                advance(lexer);
            }
            // eat the newline character as well
            if (peek(lexer) == '\n') {
                advance(lexer);
            }
            break;
        }
        return;

    // comments
    case '/':
        // eat the line
        if (peekNext(lexer) == '/') {
            while (advance(lexer) != '\n' && !isAtEnd(lexer));
            break;
        }

        // eat the block
        if (peekNext(lexer) == '*') {
            advance(lexer);
            advance(lexer);
            while (!(peek(lexer) == '*' && peekNext(lexer) == '/'))
                advance(lexer);
            advance(lexer);
            advance(lexer);
            break;
        }
        return;

    default:
        return;
    }

    // tail recursion
    eatWhitespace(lexer);
}

static bool isDigit(Xvr_Lexer* lexer) {
    return peek(lexer) >= '0' && peek(lexer) <= '9';
}

static bool isAlpha(Xvr_Lexer* lexer) {
    return (peek(lexer) >= 'A' && peek(lexer) <= 'Z') ||
           (peek(lexer) >= 'a' && peek(lexer) <= 'z') || peek(lexer) == '_';
}

static bool match(Xvr_Lexer* lexer, const char* expected, size_t length) {
    if (isAtEnd(lexer)) {
        return false;
    }

    if (strncmp(&lexer->source[lexer->current], expected, length) == 0) {
        for (size_t i = 0; i < length; i++) {
            if (isAtEnd(lexer)) return false;
            advance(lexer);
        }
        return true;
    }

    return false;
}

// token generators
static Xvr_Token makeErrorToken(Xvr_Lexer* lexer, char* msg) {
    Xvr_Token token;

    token.type = XVR_TOKEN_ERROR;
    token.lexeme = msg;
    token.length = strlen(msg);
    token.line = lexer->line;

#ifndef XVR_EXPORT
    if (Xvr_commandLine.verbose) {
        printf("err:");
        Xvr_private_printToken(&token);
    }
#endif

    return token;
}

static Xvr_Token makeToken(Xvr_Lexer* lexer, Xvr_TokenType type) {
    Xvr_Token token;

    token.type = type;
    token.length = lexer->current - lexer->start;
    token.lexeme = &lexer->source[lexer->current - token.length];
    token.line = lexer->line;

#ifndef XVR_EXPORT
    if (Xvr_commandLine.verbose) {
        printf("tok:");
        Xvr_private_printToken(&token);
    }
#endif

    return token;
}

static Xvr_Token makeIntegerOrFloat(Xvr_Lexer* lexer) {
    Xvr_TokenType type = XVR_TOKEN_LITERAL_INTEGER;  // what am I making?

    while (isDigit(lexer) || peek(lexer) == '_') advance(lexer);

    if (peek(lexer) == '.' &&
        (peekNext(lexer) >= '0' && peekNext(lexer) <= '9')) {
        type = XVR_TOKEN_LITERAL_FLOAT;
        advance(lexer);
        while (isDigit(lexer) || peek(lexer) == '_') advance(lexer);
    }

    Xvr_Token token;

    token.type = type;
    token.lexeme = &lexer->source[lexer->start];
    token.length = lexer->current - lexer->start;
    token.line = lexer->line;

#ifndef XVR_EXPORT
    if (Xvr_commandLine.verbose) {
        if (type == XVR_TOKEN_LITERAL_INTEGER) {
            printf("int:");
        } else {
            printf("flt:");
        }
        Xvr_private_printToken(&token);
    }
#endif

    return token;
}

static bool isEscapeableCharacter(char c) {
    switch (c) {
    case 'n':
    case 't':
    case '\\':
    case '"':
        return true;
    default:
        return false;
    }
}

static Xvr_Token makeString(Xvr_Lexer* lexer, char terminator) {
    while (!isAtEnd(lexer)) {
        // actually escape if you've hit the terminator
        if (peek(lexer) == terminator) {
            advance(lexer);  // eat terminator
            break;
        }

        // skip escaped terminators
        if (peek(lexer) == '\\' && isEscapeableCharacter(peekNext(lexer))) {
            advance(lexer);
            advance(lexer);
            continue;
        }

        // otherwise
        advance(lexer);
    }

    if (isAtEnd(lexer)) {
        return makeErrorToken(lexer, "Unterminated string");
    }

    Xvr_Token token;

    token.type = XVR_TOKEN_LITERAL_STRING;
    token.lexeme = &lexer->source[lexer->start + 1];
    token.length = lexer->current - lexer->start - 2;
    token.line = lexer->line;

#ifndef XVR_EXPORT
    if (Xvr_commandLine.verbose) {
        printf("str:");
        Xvr_private_printToken(&token);
    }
#endif

    return token;
}

static Xvr_Token makeKeywordOrIdentifier(Xvr_Lexer* lexer) {
    advance(lexer);  // first letter can only be alpha

    while (isDigit(lexer) || isAlpha(lexer)) {
        advance(lexer);
    }

    // scan for a keyword
    for (int i = 0; Xvr_keywordTypes[i].keyword; i++) {
        if (strlen(Xvr_keywordTypes[i].keyword) ==
                (long unsigned int)(lexer->current - lexer->start) &&
            !strncmp(Xvr_keywordTypes[i].keyword, &lexer->source[lexer->start],
                     lexer->current - lexer->start)) {
            Xvr_Token token;

            token.type = Xvr_keywordTypes[i].type;
            token.lexeme = &lexer->source[lexer->start];
            token.length = lexer->current - lexer->start;
            token.line = lexer->line;

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("kwd:");
                Xvr_private_printToken(&token);
            }
#endif

            return token;
        }
    }

    // return an identifier
    Xvr_Token token;

    token.type = XVR_TOKEN_IDENTIFIER;
    token.lexeme = &lexer->source[lexer->start];
    token.length = lexer->current - lexer->start;
    token.line = lexer->line;

#ifndef XVR_EXPORT
    if (Xvr_commandLine.verbose) {
        printf("idf:");
        Xvr_private_printToken(&token);
    }
#endif

    return token;
}

// exposed functions
void Xvr_initLexer(Xvr_Lexer* lexer, const char* source) {
    cleanLexer(lexer);

    lexer->source = source;
}

Xvr_Token Xvr_private_scanLexer(Xvr_Lexer* lexer) {
    eatWhitespace(lexer);

    lexer->start = lexer->current;

    if (isAtEnd(lexer)) return makeToken(lexer, XVR_TOKEN_EOF);

    if (isDigit(lexer)) return makeIntegerOrFloat(lexer);
    if (isAlpha(lexer)) return makeKeywordOrIdentifier(lexer);

    char c = advance(lexer);

    switch (c) {
    case '(':
        return makeToken(lexer, XVR_TOKEN_PAREN_LEFT);
    case ')':
        return makeToken(lexer, XVR_TOKEN_PAREN_RIGHT);
    case '{':
        return makeToken(lexer, XVR_TOKEN_BRACE_LEFT);
    case '}':
        return makeToken(lexer, XVR_TOKEN_BRACE_RIGHT);
    case '[':
        return makeToken(lexer, XVR_TOKEN_BRACKET_LEFT);
    case ']':
        return makeToken(lexer, XVR_TOKEN_BRACKET_RIGHT);

    case '+':
        return makeToken(lexer, match(lexer, "=", 1)   ? XVR_TOKEN_PLUS_ASSIGN
                                : match(lexer, "+", 1) ? XVR_TOKEN_PLUS_PLUS
                                                       : XVR_TOKEN_PLUS);
    case '-':
        return makeToken(lexer, match(lexer, "=", 1)   ? XVR_TOKEN_MINUS_ASSIGN
                                : match(lexer, "-", 1) ? XVR_TOKEN_MINUS_MINUS
                                                       : XVR_TOKEN_MINUS);
    case '*':
        return makeToken(lexer, match(lexer, "=", 1) ? XVR_TOKEN_MULTIPLY_ASSIGN
                                                     : XVR_TOKEN_MULTIPLY);
    case '/':
        return makeToken(lexer, match(lexer, "=", 1) ? XVR_TOKEN_DIVIDE_ASSIGN
                                                     : XVR_TOKEN_DIVIDE);
    case '%':
        return makeToken(lexer, match(lexer, "=", 1) ? XVR_TOKEN_MODULO_ASSIGN
                                                     : XVR_TOKEN_MODULO);

    case '!':
        return makeToken(
            lexer, match(lexer, "=", 1) ? XVR_TOKEN_NOT_EQUAL : XVR_TOKEN_NOT);
    case '=':
        return makeToken(
            lexer, match(lexer, "=", 1) ? XVR_TOKEN_EQUAL : XVR_TOKEN_ASSIGN);

    case '<':
        return makeToken(lexer, match(lexer, "=", 1) ? XVR_TOKEN_LESS_EQUAL
                                                     : XVR_TOKEN_LESS);
    case '>':
        return makeToken(lexer, match(lexer, "=", 1) ? XVR_TOKEN_GREATER_EQUAL
                                                     : XVR_TOKEN_GREATER);

    case '&':
        if (advance(lexer) != '&') {
            return makeErrorToken(lexer, "Unexpected '&'");
        } else {
            return makeToken(lexer, XVR_TOKEN_AND);
        }

    case '|':
        return makeToken(lexer,
                         match(lexer, "|", 1) ? XVR_TOKEN_OR : XVR_TOKEN_PIPE);

    case '?':
        return makeToken(lexer, XVR_TOKEN_QUESTION);
    case ':':
        return makeToken(lexer, XVR_TOKEN_COLON);
    case ';':
        return makeToken(lexer, XVR_TOKEN_SEMICOLON);
    case ',':
        return makeToken(lexer, XVR_TOKEN_COMMA);
    case '.':
        if (peek(lexer) == '.' && peekNext(lexer) == '.') {
            advance(lexer);
            advance(lexer);
            return makeToken(lexer, XVR_TOKEN_REST);
        }
        return makeToken(lexer, XVR_TOKEN_DOT);

    case '"':
        return makeString(lexer, c);
        // TODO: possibly support interpolated strings

    default: {
        char buffer[128];
        snprintf(buffer, 128, "Unexpected token: %c", c);
        return makeErrorToken(lexer, buffer);
    }
    }
}

static void trim(char** s, int* l) {  // all this to remove a newline?
    while (isspace(((*((unsigned char**)(s)))[(*l) - 1]))) (*l)--;
    while (**s && isspace(**(unsigned char**)(s))) {
        (*s)++;
        (*l)--;
    }
}

void Xvr_private_printToken(Xvr_Token* token) {
    if (token->type == XVR_TOKEN_ERROR) {
        printf(XVR_CC_ERROR "Error\t%d\t%.*s\n" XVR_CC_RESET, token->line,
               token->length, token->lexeme);
        return;
    }

    printf("\t%d\t%d\t", token->type, token->line);

    if (token->type == XVR_TOKEN_IDENTIFIER ||
        token->type == XVR_TOKEN_LITERAL_INTEGER ||
        token->type == XVR_TOKEN_LITERAL_FLOAT ||
        token->type == XVR_TOKEN_LITERAL_STRING) {
        printf("%.*s\t", token->length, token->lexeme);
    } else {
        char* keyword = Xvr_findKeywordByType(token->type);

        if (keyword != NULL) {
            printf("%s", keyword);
        } else {
            char* str = (char*)token->lexeme;
            int length = token->length;
            trim(&str, &length);
            printf("%.*s", length, str);
        }
    }

    printf("\n");
}
