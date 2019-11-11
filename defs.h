#ifndef DEFS_H
#define DEFS_H

// makebaby.c
#define MAX_CMDBUF 0x1000
#define MAX_TARGETS 64
#define MAX_TARGET_NAME 64
#define MAX_CMDS 16
#define MAX_ARGS 0x100

typedef struct {
    char *cmd;
    size_t args;
} command_t;

enum {
    MARK_PERM = 1 << 0,
    MARK_TEMP = 1 << 1,
};

typedef struct {
    size_t idx;
    size_t order;
    unsigned char marks;
    char name[MAX_TARGET_NAME + 1];
    size_t ncmds;
    command_t cmds[MAX_CMDS];
} target_t;

typedef enum {
    STATE_NEWLINE,
    STATE_WANT_COLON,
    STATE_DEPS,
    STATE_COMMAND,
} parse_state_t;

// scanner.c
#define MAX_BUF 128

typedef struct {
    FILE *fp;
    unsigned int i;
} scanner_t;

typedef enum {
    TOKEN_UNKNOWN,
    TOKEN_EOF,
    TOKEN_TAB,
    TOKEN_COLON,
    TOKEN_NEWLINE,
    TOKEN_WORD,
    TOKEN_SPACE,
} token_type_t;

typedef struct {
    token_type_t type;
    char token[MAX_BUF];
} token_t;

void scanner_new(scanner_t *scanner, FILE *fp);
token_t *scanner_next(scanner_t *scanner, token_t *token);

#endif
