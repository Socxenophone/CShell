#include "simple_shell.h"

// Custom command callback
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
