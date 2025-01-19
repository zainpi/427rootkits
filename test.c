#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // Check if an argument is provided
    if (argc != 2) {
        printf("Usage: %s <trigger_word>\n", argv[0]);
        //hint: check rootkit.c
        return 1;
    }

    const char *trigger = argv[1];


    // Use write() to send the trigger word
    write(STDOUT_FILENO, trigger, strlen(trigger));
    write(STDOUT_FILENO, "\n", 1); // Add a newline for better output formatting

    return 0;
}
