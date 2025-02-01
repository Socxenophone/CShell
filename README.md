
# **Simple Shell Library API Documentation**

The **Simple Shell Library** is a single header, lightweight, extensible, and easy-to-use library for integrating a shell into your C applications. It supports features like command execution, input/output redirection, job control, custom commands, and more.


## **Table of Contents**
1. [Initialization and Cleanup](#initialization-and-cleanup)
2. [Command Execution](#command-execution)
3. [Custom Commands](#custom-commands)
4. [Job Control](#job-control)
5. [Environment Variables](#environment-variables)
6. [Error Handling](#error-handling)
7. [Interactive Mode](#interactive-mode)
8. [Prompt Customization](#prompt-customization)
9. [Example Usage](#example-usage)


### **Initialization and Cleanup**

#### `shell_init`
Initializes the shell context.

```c
ShellError shell_init(ExtendedShellContext *ctx, const char *prompt, bool interactive);
```

##### Parameters:
- `ctx`: Pointer to the `ExtendedShellContext` structure.
- `prompt`: Custom shell prompt (e.g., `"> "`). If `NULL`, defaults to `"> "`.
- `interactive`: Set to `true` for interactive mode (e.g., displaying a prompt).

##### Returns:
- `SHELL_OK` on success.
- Error code on failure (e.g., `SHELL_ERROR_NULL_POINTER`).

---

#### `shell_cleanup`
Cleans up the shell context and frees allocated resources.

```c
ShellError shell_cleanup(ExtendedShellContext *ctx);
```

##### Parameters:
- `ctx`: Pointer to the `ExtendedShellContext` structure.

##### Returns:
- `SHELL_OK` on success.
- Error code on failure.

---

### **Command Execution**

#### `shell_execute_command`
Parses and executes a command.

```c
ShellError shell_execute_command(ExtendedShellContext *ctx, const char *command);
```

##### Parameters:
- `ctx`: Pointer to the `ExtendedShellContext` structure.
- `command`: The command to execute (e.g., `"ls -l"`).

##### Returns:
- `SHELL_OK` on success.
- Error code on failure.

---

#### `shell_execute_external`
Executes an external command using `posix_spawnp`.

```c
ShellError shell_execute_external(ExtendedShellContext *ctx, char *const argv[], const char *input_file, const char *output_file, bool append_output);
```

##### Parameters:
- `ctx`: Pointer to the `ExtendedShellContext` structure.
- `argv`: Array of command arguments (e.g., `{"ls", "-l", NULL}`).
- `input_file`: File to redirect `stdin` from (e.g., `"input.txt"`).
- `output_file`: File to redirect `stdout` to (e.g., `"output.txt"`).
- `append_output`: If `true`, append to `output_file` instead of overwriting.

##### Returns:
- `SHELL_OK` on success.
- Error code on failure.

---

### **Custom Commands**

#### `shell_register_command`
Registers a custom command.

```c
ShellError shell_register_command(ExtendedShellContext *ctx, const char *name, CommandCallback callback);
```

##### Parameters:
- `ctx`: Pointer to the `ExtendedShellContext` structure.
- `name`: Name of the custom command (e.g., `"hello"`).
- `callback`: Function to execute when the command is called.

##### Returns:
- `SHELL_OK` on success.
- Error code on failure.

---

#### `shell_execute_custom`
Executes a registered custom command.

```c
ShellError shell_execute_custom(ExtendedShellContext *ctx, int argc, char **argv);
```

##### Parameters:
- `ctx`: Pointer to the `ExtendedShellContext` structure.
- `argc`: Number of arguments.
- `argv`: Array of arguments.

##### Returns:
- `SHELL_OK` on success.
- Error code on failure.

---

### **Job Control**

#### `shell_add_job`
Adds a job to the job list.

```c
ShellError shell_add_job(ExtendedShellContext *ctx, pid_t pid, const char *command);
```

##### Parameters:
- `ctx`: Pointer to the `ExtendedShellContext` structure.
- `pid`: Process ID of the job.
- `command`: Command associated with the job.

##### Returns:
- `SHELL_OK` on success.
- Error code on failure.

---

#### `shell_update_jobs`
Updates the status of all jobs.

```c
ShellError shell_update_jobs(ExtendedShellContext *ctx);
```

##### Parameters:
- `ctx`: Pointer to the `ExtendedShellContext` structure.

##### Returns:
- `SHELL_OK` on success.
- Error code on failure.

---

### **Environment Variables**

#### `shell_set_env`
Sets an environment variable.

```c
ShellError shell_set_env(ExtendedShellContext *ctx, const char *key, const char *value);
```

##### Parameters:
- `ctx`: Pointer to the `ExtendedShellContext` structure.
- `key`: Environment variable name.
- `value`: Environment variable value.

##### Returns:
- `SHELL_OK` on success.
- Error code on failure.

---

#### `shell_get_env`
Retrieves the value of an environment variable.

```c
const char *shell_get_env(ExtendedShellContext *ctx, const char *key);
```

##### Parameters:
- `ctx`: Pointer to the `ExtendedShellContext` structure.
- `key`: Environment variable name.

##### Returns:
- Value of the environment variable, or `NULL` if not found.

---

### **Error Handling**

#### Error Codes
The library uses the `ShellError` enum to represent errors. Common error codes include:
- `SHELL_OK`: No error.
- `SHELL_ERROR_NULL_POINTER`: Null pointer encountered.
- `SHELL_ERROR_MEMORY_ALLOCATION`: Memory allocation failed.
- `SHELL_ERROR_COMMAND_NOT_FOUND`: Command not found.
- `SHELL_ERROR_EXECUTION_FAILED`: Command execution failed.

---

### **Interactive Mode**

#### Interactive Mode
If `interactive` is set to `true` during initialization, the shell will display a prompt and wait for user input.

---

### **Prompt Customization**

#### Custom Prompt
You can set a custom prompt during initialization using the `prompt` parameter in `shell_init`.

---

### **Example Usage**

#### Example 1: Basic Shell
```c
#include "simple_shell.h"

ShellError custom_hello(ShellContext *ctx, int argc, char **argv) {
    if (argc > 1) {
        printf("Hello, %s!\n", argv[1]);
    } else {
        printf("Hello, world!\n");
    }
    return SHELL_OK;
}

int main() {
    ExtendedShellContext ctx;
    shell_init(&ctx, "my_shell> ", true);

    // Register a custom command
    shell_register_command(&ctx, "hello", custom_hello);

    // Run the shell
    shell_run(&ctx);

    // Clean up
    shell_cleanup(&ctx);
    return 0;
}
```

#### Example 2: Redirecting Output
```c
#include "simple_shell.h"

int main() {
    ExtendedShellContext ctx;
    shell_init(&ctx, NULL, false);

    // Execute a command with output redirection
    char *args[] = {"ls", "-l", NULL};
    shell_execute_external(&ctx, args, NULL, "output.txt", false);

    // Clean up
    shell_cleanup(&ctx);
    return 0;
}
```

---

### **License**
This library is released under the **MIT License**. Feel free to use, modify, and distribute it as needed.

---

