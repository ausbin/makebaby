#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "defs.h"

void scanner_new(scanner_t *scanner, FILE *fp) {
    scanner->fp = fp;
    scanner->i = 0;
}

static token_type_t symbol_finder(scanner_t *scanner, int c, token_type_t last) {
    if ((!scanner->i || last == TOKEN_SPACE) && c == ' ') {
        return TOKEN_SPACE;
    }

    if (!scanner->i) {
        switch (c) {
            case '\t':
            return TOKEN_TAB;

            case ':':
            return TOKEN_COLON;

            case '\n':
            return TOKEN_NEWLINE;

            default:
            return TOKEN_UNKNOWN;
        }
    } else {
        return TOKEN_UNKNOWN;
    }
}

static token_type_t word_finder(scanner_t *scanner, int c, token_type_t last) {
    (void)scanner;
    (void)last;
    return (c == EOF || c == ':' || isspace(c))? TOKEN_UNKNOWN : TOKEN_WORD;
}

typedef token_type_t (*finder_t)(scanner_t *scanner, int c, token_type_t last);

token_t *scanner_next(scanner_t *scanner, token_t *token) {
    static const finder_t finders[] = {symbol_finder, word_finder};
    static const size_t nfinders = sizeof finders / sizeof (finder_t);

    token_type_t last_type[nfinders];
    token_type_t type[nfinders];
    memset(last_type, 0, sizeof last_type);

    scanner->i = 0;
    int c;

    while (1) {
        if ((c = fgetc(scanner->fp)) == EOF) {
            if (!scanner->i) {
                token->type = TOKEN_EOF;
                goto found;
            } else {
                for (size_t i = 0; i < nfinders; i++) {
                    if (last_type[i] != TOKEN_UNKNOWN) {
                        token->type = last_type[i];
                        goto found;
                    }
                }

                goto notfound;
            }
        } else {
            token->token[scanner->i] = c;
        }

        for (size_t i = 0; i < nfinders; i++) {
            type[i] = finders[i](scanner, c, last_type[i]);
        }

        for (size_t i = 0; i < nfinders; i++) {
            if (type[i] == TOKEN_UNKNOWN && last_type[i] != TOKEN_UNKNOWN) {
                token->type = last_type[i];
                goto found;
            }
        }

        int ok = 0;
        for (size_t i = 0; i < nfinders; i++) {
            last_type[i] = type[i];
            if (type[i] != TOKEN_UNKNOWN) {
                ok = 1;
            }
        }

        if (!ok) {
            goto notfound;
        }

        if (++scanner->i == MAX_BUF) {
            fprintf(stderr, "token too long");
            return NULL;
        }
    }

    notfound:
    token->type = TOKEN_UNKNOWN;
    // We want to see the mystery token
    scanner->i++;
    // fallthrough

    found:
    token->token[scanner->i] = '\0';
    ungetc(c, scanner->fp);
    return token;
}
