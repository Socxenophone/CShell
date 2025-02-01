#ifndef SIMPLE_SHELL_H
#define SIMPLE_SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define MAX_INPUT_SIZE 1024
#define MAX_HISTORY_SIZE 100
#define MAX_ENV_VARS 100
#define MAX_CUSTOM_COMMANDS 50
#define MAX_JOBS 100
#define MAX_ALIASES 50
#define MAX_TAB_COMPLETIONS 100

// Error codes
typedef enum {
    SHELL_OK = 0,
    SHELL_ERROR_NULL_POINTER,
    SHELL_ERROR_MEMORY_ALLOCATION,
    SHELL_ERROR_INVALID_INPUT,
    SHELL_ERROR_COMMAND_NOT_FOUND,
    SHELL_ERROR_EXECUTION_FAILED,
    SHELL_ERROR_SIGNAL_HANDLING_FAILED,
    SHELL_ERROR_HISTORY_FULL,
    SHELL_ERROR_ENV_VAR_NOT_FOUND,
    SHELL_ERROR_ENV_VAR_FULL,
    SHELL_ERROR_CUSTOM_COMMAND_FULL,
    SHELL_ERROR_JOB_CONTROL_FULL,
    SHELL_ERROR_REDIRECTION_FAILED,
    SHELL_ERROR_PIPELINE_FAILED,
    SHELL_ERROR_ALIAS_FULL,
    SHELL_ERROR_TAB_COMPLETION_FAILED
} ShellError;

// Shell context structure
typedef struct {
    char *input;
    char *output;
    char *error;
    char *history[MAX_HISTORY_SIZE];
    int history_count;
    char *env_vars[MAX_ENV_VARS];
    int env_vars_count;
    pid_t child_pid;
    sigset_t signal_mask;
    ShellError last_error;
    char *prompt;
    bool interactive;
} ShellContext;

// Job structure for job control
typedef struct {
    pid_t pid;
    char *command;
    bool running;
} Job;

// Custom command callback type
typedef ShellError (*CommandCallback)(ShellContext *ctx, int argc, char **argv);

// Custom command structure
typedef struct {
    char *name;
    CommandCallback callback;
} CustomCommand;

// Alias structure
typedef struct {
    char *name;
    char *value;
} Alias;

// Shell context extension for custom commands, job control, aliases, etc.
typedef struct {
    ShellContext base;
    CustomCommand custom_commands[MAX_CUSTOM_COMMANDS];
    int custom_command_count;
    Job jobs[MAX_JOBS];
    int job_count;
    Alias aliases[MAX_ALIASES];
    int alias_count;
} ExtendedShellContext;

// Initialize the shell context
ShellError shell_init(ExtendedShellContext *ctx, const char *prompt, bool interactive) {
    if (!ctx) return SHELL_ERROR_NULL_POINTER;

    ctx->base.input = NULL;
    ctx->base.output = NULL;
    ctx->base.error = NULL;
    ctx->base.history_count = 0;
    ctx->base.env_vars_count = 0;
    ctx->base.child_pid = -1;
    ctx->base.last_error = SHELL_OK;
    ctx->base.prompt = prompt ? strdup(prompt) : strdup("> ");
    ctx->base.interactive = interactive;
    ctx->custom_command_count = 0;
    ctx->job_count = 0;
    ctx->alias_count = 0;

    // Initialize signal mask
    sigemptyset(&ctx->base.signal_mask);
    sigaddset(&ctx->base.signal_mask, SIGINT);
    sigaddset(&ctx->base.signal_mask, SIGTERM);

    // Block signals during critical sections
    if (sigprocmask(SIG_BLOCK, &ctx->base.signal_mask, NULL) == -1) {
        ctx->base.last_error = SHELL_ERROR_SIGNAL_HANDLING_FAILED;
        return SHELL_ERROR_SIGNAL_HANDLING_FAILED;
    }

    return SHELL_OK;
}

// Clean up the shell context
ShellError shell_cleanup(ExtendedShellContext *ctx) {
    if (!ctx) return SHELL_ERROR_NULL_POINTER;

    if (ctx->base.input) free(ctx->base.input);
    if (ctx->base.output) free(ctx->base.output);
    if (ctx->base.error) free(ctx->base.error);
    if (ctx->base.prompt) free(ctx->base.prompt);

    for (int i = 0; i < ctx->base.history_count; i++) {
        if (ctx->base.history[i]) free(ctx->base.history[i]);
    }

    for (int i = 0; i < ctx->base.env_vars_count; i++) {
        if (ctx->base.env_vars[i]) free(ctx->base.env_vars[i]);
    }

    for (int i = 0; i < ctx->custom_command_count; i++) {
        if (ctx->custom_commands[i].name) free(ctx->custom_commands[i].name);
    }

    for (int i = 0; i < ctx->job_count; i++) {
        if (ctx->jobs[i].command) free(ctx->jobs[i].command);
    }

    for (int i = 0; i < ctx->alias_count; i++) {
        if (ctx->aliases[i].name) free(ctx->aliases[i].name);
        if (ctx->aliases[i].value) free(ctx->aliases[i].value);
    }

    return SHELL_OK;
}

// Register a custom command
ShellError shell_register_command(ExtendedShellContext *ctx, const char *name, CommandCallback callback) {
    if (!ctx || !name || !callback) return SHELL_ERROR_NULL_POINTER;

    if (ctx->custom_command_count >= MAX_CUSTOM_COMMANDS) {
        ctx->base.last_error = SHELL_ERROR_CUSTOM_COMMAND_FULL;
        return SHELL_ERROR_CUSTOM_COMMAND_FULL;
    }

    ctx->custom_commands[ctx->custom_command_count].name = strdup(name);
    if (!ctx->custom_commands[ctx->custom_command_count].name) {
        ctx->base.last_error = SHELL_ERROR_MEMORY_ALLOCATION;
        return SHELL_ERROR_MEMORY_ALLOCATION;
    }

    ctx->custom_commands[ctx->custom_command_count].callback = callback;
    ctx->custom_command_count++;

    return SHELL_OK;
}

// Add a job to the job list
ShellError shell_add_job(ExtendedShellContext *ctx, pid_t pid, const char *command) {
    if (!ctx || !command) return SHELL_ERROR_NULL_POINTER;

    if (ctx->job_count >= MAX_JOBS) {
        ctx->base.last_error = SHELL_ERROR_JOB_CONTROL_FULL;
        return SHELL_ERROR_JOB_CONTROL_FULL;
    }

    ctx->jobs[ctx->job_count].pid = pid;
    ctx->jobs[ctx->job_count].command = strdup(command);
    ctx->jobs[ctx->job_count].running = true;
    ctx->job_count++;

    return SHELL_OK;
}

// Check and update job status
ShellError shell_update_jobs(ExtendedShellContext *ctx) {
    if (!ctx) return SHELL_ERROR_NULL_POINTER;

    for (int i = 0; i < ctx->job_count; i++) {
        if (ctx->jobs[i].running) {
            int status;
            pid_t result = waitpid(ctx->jobs[i].pid, &status, WNOHANG);
            if (result > 0) {
                ctx->jobs[i].running = false;
                printf("[%d] Done: %s\n", i + 1, ctx->jobs[i].command);
            }
        }
    }

    return SHELL_OK;
}

// Execute a custom command
ShellError shell_execute_custom(ExtendedShellContext *ctx, int argc, char **argv) {
    if (!ctx || !argv) return SHELL_ERROR_NULL_POINTER;

    for (int i = 0; i < ctx->custom_command_count; i++) {
        if (strcmp(argv[0], ctx->custom_commands[i].name) == 0) {
            return ctx->custom_commands[i].callback(&ctx->base, argc, argv);
        }
    }

    ctx->base.last_error = SHELL_ERROR_COMMAND_NOT_FOUND;
    return SHELL_ERROR_COMMAND_NOT_FOUND;
}

// Execute a built-in command
ShellError shell_execute_builtin(ExtendedShellContext *ctx, const char *command) {
    if (!ctx || !command) return SHELL_ERROR_NULL_POINTER;

    if (strcmp(command, "exit") == 0) {
        exit(0);
    } else if (strcmp(command, "history") == 0) {
        for (int i = 0; i < ctx->base.history_count; i++) {
            printf("%d: %s\n", i + 1, ctx->base.history[i]);
        }
    } else if (strcmp(command, "jobs") == 0) {
        shell_update_jobs(ctx);
        for (int i = 0; i < ctx->job_count; i++) {
            printf("[%d] %s: %s\n", i + 1, ctx->jobs[i].running ? "Running" : "Done", ctx->jobs[i].command);
        }
    } else {
        ctx->base.last_error = SHELL_ERROR_COMMAND_NOT_FOUND;
        return SHELL_ERROR_COMMAND_NOT_FOUND;
    }

    return SHELL_OK;
}

// Execute an external command using posix_spawnp
ShellError shell_execute_external(ExtendedShellContext *ctx, char *const argv[], const char *input_file, const char *output_file, bool append_output) {
    if (!ctx || !argv) return SHELL_ERROR_NULL_POINTER;

    pid_t pid;
    posix_spawnattr_t attr;
    posix_spawn_file_actions_t file_actions;

    // Initialize spawn attributes and file actions
    posix_spawnattr_init(&attr);
    posix_spawn_file_actions_init(&file_actions);

    // Set flags for posix_spawn
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF);
    posix_spawnattr_setsigmask(&attr, &ctx->base.signal_mask);

    // Handle input redirection
    if (input_file) {
        posix_spawn_file_actions_addopen(&file_actions, STDIN_FILENO, input_file, O_RDONLY, 0);
    }

    // Handle output redirection
    if (output_file) {
        int flags = O_WRONLY | O_CREAT | (append_output ? O_APPEND : O_TRUNC);
        posix_spawn_file_actions_addopen(&file_actions, STDOUT_FILENO, output_file, flags, 0644);
    }

    // Execute the command
    int status = posix_spawnp(&pid, argv[0], &file_actions, &attr, argv, environ);
    if (status != 0) {
        ctx->base.last_error = SHELL_ERROR_EXECUTION_FAILED;
        return SHELL_ERROR_EXECUTION_FAILED;
    }

    // Add the job to the job list
    shell_add_job(ctx, pid, argv[0]);

    // Clean up
    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&file_actions);

    return SHELL_OK;
}

// Parse and execute a command with redirection and piping
ShellError shell_execute_command(ExtendedShellContext *ctx, const char *command) {
    if (!ctx || !command) return SHELL_ERROR_NULL_POINTER;

    // Tokenize the command
    char *tokens[MAX_INPUT_SIZE];
    int token_count = 0;
    char *token = strtok((char *)command, " ");
    while (token) {
        tokens[token_count++] = token;
        token = strtok(NULL, " ");
    }
    tokens[token_count] = NULL;

    // Handle redirection and piping
    const char *input_file = NULL;
    const char *output_file = NULL;
    bool append_output = false;

    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            input_file = tokens[i + 1];
            tokens[i] = NULL;
        } else if (strcmp(tokens[i], ">") == 0) {
            output_file = tokens[i + 1];
            tokens[i] = NULL;
        } else if (strcmp(tokens[i], ">>") == 0) {
            output_file = tokens[i + 1];
            append_output = true;
            tokens[i] = NULL;
        }
    }

    // Execute custom commands
    if (shell_execute_custom(ctx, token_count, tokens) == SHELL_OK) {
        return SHELL_OK;
    }

    // Execute built-in commands
    if (shell_execute_builtin(ctx, tokens[0]) == SHELL_OK) {
        return SHELL_OK;
    }

    // Execute external commands
    return shell_execute_external(ctx, tokens, input_file, output_file, append_output);
}

// Main shell loop
ShellError shell_run(ExtendedShellContext *ctx) {
    if (!ctx) return SHELL_ERROR_NULL_POINTER;

    char input[MAX_INPUT_SIZE];
    while (true) {
        if (ctx->base.interactive) {
            printf("%s", ctx->base.prompt);
        }

        if (!fgets(input, MAX_INPUT_SIZE, stdin)) {
            ctx->base.last_error = SHELL_ERROR_INVALID_INPUT;
            return SHELL_ERROR_INVALID_INPUT;
        }

        // Remove newline character
        input[strcspn(input, "\n")] = 0;

        // Add to history
        shell_add_history(&ctx->base, input);

        // Execute the command
        shell_execute_command(ctx, input);
    }

    return SHELL_OK;
}

#endif // SIMPLE_SHELL_H 
