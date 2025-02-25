#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define OUT_LEN 1000 // 1000 items with 100 symbol len
int com_out_len = OUT_LEN;
// Output of command execution
char com_out[OUT_LEN][100];
int com_out_i = 0;

bool runCommand(char* command) {
	FILE *fp;
	char path[1024];
	int status;
	
	// Catch stdout and stderr
	char fullCommand[1024];
	snprintf(fullCommand, sizeof(fullCommand), "%s 2>&1", command);

	// Open the command for reading
	fp = popen(fullCommand, "r");
	if (fp == NULL) {
		printf("Failed to run command\n");
		return false;
	}

	// Clean
	com_out_i = 0;
	memset(com_out, 0, sizeof(com_out));
	
	// Read the output a line at a time and store it in the out array
	while (fgets(path, sizeof(path), fp) != NULL) {
		strncpy(com_out[com_out_i], path, 100);
		com_out[com_out_i][99] = '\0';  // Ensure null termination
		com_out_i++;
		if (com_out_i >= com_out_len) {
			break;  // Prevent overflow
		}
		//printf("%s", path);
	}

	/* Close */
	status = pclose(fp);

	/* Check the return status of the command */
	if (WIFEXITED(status)) {
		int exit_status = WEXITSTATUS(status);
		if (exit_status != 0) {
			return false;  // Command failed
		}
	} else {
		return false;  // Command did not exit normally
	}

	return true;
}
