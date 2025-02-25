#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define CONFIG_DIR "/.config/psaver"
#define CONFIG_FILE "/.config/psaver/settings"

#define DEFAULT_TIMEOUT 60
#define DEFAULT_MIN_FREQ "800MHz"
#define DEFAULT_MAX_FREQ "1GHz"

int timeout = DEFAULT_TIMEOUT;
char min_freq[16] = DEFAULT_MIN_FREQ;
char max_freq[16] = DEFAULT_MAX_FREQ;

void create_default_config(const char *filepath) {
    FILE *file = fopen(filepath, "w");
    if (file) {
        fprintf(file, "timeout=%d\n", DEFAULT_TIMEOUT);
        fprintf(file, "min_freq=%s\n", DEFAULT_MIN_FREQ);
        fprintf(file, "max_freq=%s\n", DEFAULT_MAX_FREQ);
        fclose(file);
    } else {
        perror("Failed to create config file");
    }
}

void load_config(
	void
) {
    char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Error: Cannot determine HOME directory.\n");
        return;
    }

    char config_dir[512], config_path[512];
    snprintf(config_dir, sizeof(config_dir), "%s%s", home, CONFIG_DIR);
    snprintf(config_path, sizeof(config_path), "%s%s", home, CONFIG_FILE);

    struct stat st;
    if (stat(config_path, &st) != 0) {
        // Config file does not exist, create it
        mkdir(config_dir, 0755);  // Ensure the directory exists
        create_default_config(config_path);
        return;
    }

    FILE *file = fopen(config_path, "r");
    if (!file) {
        perror("Failed to open config file");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "timeout=", 8) == 0) {
            timeout = atoi(line + 8);
        } else if (strncmp(line, "min_freq=", 9) == 0) {
            sscanf(line + 9, "%15s", min_freq);
        } else if (strncmp(line, "max_freq=", 9) == 0) {
            sscanf(line + 9, "%15s", max_freq);
        }
    }
    fclose(file);
}
