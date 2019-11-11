#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "defs.h"

static size_t cmdbufi;
static char cmdbuf[MAX_CMDBUF];
static size_t ntargets;
static target_t targets[MAX_TARGETS];
static char adjmatrix[MAX_TARGETS][MAX_TARGETS];

static int parse(void);
static int topo(void);
static int run(void);

int main(void) {
    if (parse())
        goto failure;

    if (topo())
        goto failure;

    if (run())
        goto failure;

    return 0;

    failure:
    return 1;
}

static target_t *get_target(const char *name) {
    target_t *target = NULL;
    for (size_t i = 0; i < ntargets; i++) {
        if (!strcmp(name, targets[i].name)) {
            target = &targets[i];
        }
    }

    if (!target) {
        if (strlen(name) > MAX_TARGET_NAME) {
            fprintf(stderr, "target name exceeds max of %d chars\n",
                            MAX_TARGET_NAME);
            return NULL;
        }
        if (ntargets == MAX_TARGETS) {
            fprintf(stderr, "exceed max targets %d\n", MAX_TARGETS);
            return NULL;
        }

        target = &targets[ntargets];
        strcpy(target->name, name);
        target->idx = ntargets++;
        target->ncmds = 0;
    }

    return target;
}

static int parse(void) {
    FILE *fp;

    if (!(fp = fopen("Makebabyfile", "r"))) {
        perror("Could not open Makebabyfile");
        return 1;
    }

    scanner_t scanner;
    scanner_new(&scanner, fp);

    token_t token;
    parse_state_t state = STATE_NEWLINE;
    target_t *target = NULL;

    do {
        if (!scanner_next(&scanner, &token))
            goto failure;

        if (token.type == TOKEN_UNKNOWN) {
            fprintf(stderr, "unknown token: `%s'\n", token.token);
            goto failure;
        }

        if (state == STATE_NEWLINE) {
            if (token.type == TOKEN_NEWLINE || token.type == TOKEN_EOF) {
                // empty line, that's fine
            } else if (token.type == TOKEN_WORD) {
                // cool, a new(?) target!
                if (!(target = get_target(token.token)))
                    goto failure;

                state = STATE_WANT_COLON;
            } else if (token.type == TOKEN_TAB) {
                // sweet, some commands for the target
                if (!target) {
                    fprintf(stderr, "stray tab\n");
                    goto failure;
                } else {
                    // Allocate a new command
                    if (target->ncmds == MAX_CMDS) {
                        fprintf(stderr, "too many commands for target %s\n", target->name);
                        goto failure;
                    }

                    target->ncmds++;
                    target->cmds[target->ncmds - 1].cmd = NULL;
                    target->cmds[target->ncmds - 1].args = 0;

                    state = STATE_COMMAND;
                }
            } else {
                fprintf(stderr, "unexpected token at beginning of line: `%s'\n", token.token);
                goto failure;
            }
        } else if (state == STATE_WANT_COLON) {
            if (token.type == TOKEN_COLON) {
                state = STATE_DEPS;
            } else if (token.type == TOKEN_SPACE) {
                // ignore
            } else {
                fprintf(stderr, "expected `:', got `%s'\n", token.token);
                goto failure;
            }
        } else if (state == STATE_DEPS) {
            if (token.type == TOKEN_WORD) {
                target_t *dep;
                if (!(dep = get_target(token.token)))
                    goto failure;

                // Mark edge from target -> dep
                adjmatrix[target->idx][dep->idx] = 1;
            } else if (token.type == TOKEN_SPACE) {
                // ignore
            } else if (token.type == TOKEN_NEWLINE || token.type == TOKEN_EOF) {
                state = STATE_NEWLINE;
            } else {
                fprintf(stderr, "expected dependency, got `%s'\n", token.token);
                goto failure;
            }
        } else if (state == STATE_COMMAND) {
            if (token.type == TOKEN_WORD) {
                size_t token_len = strlen(token.token) + 1;
                if (token_len + cmdbufi >= MAX_CMDBUF) {
                    fprintf(stderr, "ran out of cmd memory\n");
                    goto failure;
                }

                // another arg, epic!
                if (++target->cmds[target->ncmds - 1].args > MAX_ARGS) {
                    fprintf(stderr, "too many args\n");
                    goto failure;
                }

                if (!target->cmds[target->ncmds - 1].cmd)
                    target->cmds[target->ncmds - 1].cmd = cmdbuf + cmdbufi;

                strcpy(cmdbuf + cmdbufi, token.token);
                cmdbufi += token_len;
            } else if (token.type == TOKEN_SPACE) {
                // ignore
            } else if (token.type == TOKEN_NEWLINE || token.type == TOKEN_EOF) {
                // If this was just a tab followed by a newline,
                // dont consider it a command
                if (!target->cmds[target->ncmds - 1].cmd)
                    target->ncmds--;

                state = STATE_NEWLINE;
            } else {
                fprintf(stderr, "expected command, got `%s'\n", token.token);
                goto failure;
            }
        } else {
            fprintf(stderr, "unknown parse state `%u'\n", state);
            goto failure;
        }
    } while (token.type != TOKEN_EOF);

    fclose(fp);
    return 0;

    failure:
    fclose(fp);
    return 1;
}

static size_t topo_order;

static int topo_visit(target_t *target) {
    if (target->marks & MARK_PERM)
        return 0;
    if (target->marks & MARK_TEMP) {
        fprintf(stderr, "cycle detected: %s depends on itself\n", target->name);
        return 1;
    }
    target->marks |= MARK_TEMP;

    for (size_t i = 0; i < ntargets; i++)
        if (adjmatrix[target->idx][i])
            topo_visit(&targets[i]);

    target->marks &= ~MARK_TEMP;
    target->marks |= MARK_PERM;
    target->order = topo_order++;

    return 0;
}

static int topo_target_compare(const void *leftp, const void *rightp) {
    const target_t *left = (const target_t *) leftp;
    const target_t *right = (const target_t *) rightp;
    return (int)left->order - (int)right->order;
}

// https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search
static int topo(void) {
    while (1) {
        target_t *target = NULL;
        for (size_t i = 0; i < ntargets; i++) {
            if (!(targets[i].marks & MARK_PERM)) {
                target = &targets[i];
                break;
            }
        }

        if (!target)
            break;

        if (topo_visit(target))
            goto failure;
    }

    qsort(targets, ntargets, sizeof (target_t), topo_target_compare);

    return 0;

    failure:
    return 1;
}

static char *args[MAX_ARGS + 1];

static int run(void) {
    for (size_t i = 0; i < ntargets; i++) {
        target_t *target = &targets[i];

        for (size_t j = 0; j < target->ncmds; j++) {
            command_t *cmd = &target->cmds[j];
            char *str = cmd->cmd;

            for (size_t k = 0; k < cmd->args; k++) {
                printf("%s%s", str, (k + 1 < cmd->args)? " " : "");
                args[k] = str;
                str += strlen(str) + 1;
            }

            printf("\n");
            args[cmd->args] = NULL;

            pid_t pid;
            if ((pid = fork()) == -1) {
                perror("fork");
                goto failure;
            }

            if (!pid) {
                if (execvp(args[0], args) == -1) {
                    perror("exec");
                    goto failure;
                }
            } else {
                int wstatus;
                if (waitpid(pid, &wstatus, 0) == -1) {
                    perror("waitpid");
                    goto failure;
                }

                int exit_code = WEXITSTATUS(wstatus);
                if (exit_code) {
                    fprintf(stderr, "command failed with exit code %d\n", exit_code);
                    goto failure;
                }
            }
        }
    }

    return 0;

    failure:
    return 1;
}
