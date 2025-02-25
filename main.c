#include <unistd.h> 

#include "wlr_randr.c"
#include "config.h"
#include "run_command.h"

// Wait in milliseconds
void wait(int time) {
	usleep(time * 1000);
}

// Blob from wlr-randr to get state of screen
// 0 - off, 1 - on
void get_state(
	void
) {
	struct randr_state state = { .running = true };
	wl_list_init(&state.heads);
	struct wl_display *display = wl_display_connect(NULL);

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, &state);
	wl_display_dispatch(display);
	wl_display_roundtrip(display);

	get_s_state(&state);

	while (state.running && wl_display_dispatch(display) != -1) {
		// This space intentionally left blank
	}

	// TODO: destroy heads
	zwlr_output_manager_v1_destroy(state.output_manager);
	wl_registry_destroy(registry);
	wl_display_disconnect(display);
}

//#include <stdlib.h>

#define FILE_PATH "/etc/sudoers"
#define BUFFER_SIZE 1024

int main(
	int argc, char *argv[]
) {
	//
	// Check if installed aready
	//
	char *home = getenv("HOME");
	char installed_dir[512];
	snprintf(installed_dir, sizeof(installed_dir), "%s%s", home, "/.config/psaver/installed");
	if (access(installed_dir, F_OK) == 0)
	{
		// INSTALLED
	}
	else {
		// NOT INSTALLED
		char *user = getenv("USER");

		runCommand("whereis chcpu | awk -F \" \" '{print $2}'");
		char chcpu_loc[50] = "";
		strcpy(chcpu_loc, com_out[0]);
		chcpu_loc[strlen(chcpu_loc) - 1] = '\0'; // Remove '\n'
		
		runCommand("whereis cpupower | awk -F \" \" '{print $2}'");
		char cpupower_loc[50] = "";
		strcpy(cpupower_loc, com_out[0]);
		cpupower_loc[strlen(cpupower_loc) - 1] = '\0'; // Remove '\n'

		// Generate string like "user ALL = (root) NOPASSWD: /sbin/chcpu"
		char noroot_chcpu[100] = ""; 
		char noroot_cpupower[100] = "";
		sprintf(noroot_chcpu, "%s ALL = (root) NOPASSWD: %s", user, chcpu_loc);
		sprintf(noroot_cpupower, "%s ALL = (root) NOPASSWD: %s", user, cpupower_loc);

		// Generate command to write: "echo 'user ALL = (root) NOPASSWD: /sbin/chcpu' | sudo tee -a /etc/sudoers"
		char write_com_chcpu[200] = "";
		char write_com_cpupower[200] = "";
		sprintf(write_com_chcpu, "echo '%s' | sudo tee -a /etc/sudoers", noroot_chcpu);
		sprintf(write_com_cpupower, "echo '%s' | sudo tee -a /etc/sudoers", noroot_cpupower);

		printf("Allow cpupower and chcpu without password... Please, enter password.\n");
		runCommand(write_com_chcpu);
		runCommand(write_com_cpupower);

		// Create file to point what we make all
		FILE *fptr;
		fptr = fopen(installed_dir, "w");
		fclose(fptr); 
		
		return 0;
	}

	// Load timeout from config
	load_config();

	// How much device has screen off
	// Turn on screen - reset timer
	int sleep_time = 0;
	bool optimized = false;
	char cpupower_com[200] = "";
	while (1) {
		get_state();
		if (screen_state == 1) {
			printf("Screen on\n");
			sleep_time = 0;
			if (optimized) {
				// UNOPTIMIZE SECTION
				runCommand("sudo chcpu -e 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16");
				sprintf(cpupower_com, "sudo cpupower frequency-set --max %s", max_freq);
				runCommand(cpupower_com);
				optimized = false;
				printf("UNOPTIMIZE\n");
			}
		}
		else {
			printf("Screen off\n");
			if (!optimized) {
				if (sleep_time >= timeout) {
					// OPTIMIZE SECTION
					runCommand("sudo chcpu -d 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16");
					sprintf(cpupower_com, "sudo cpupower frequency-set --max %s", min_freq);
					runCommand(cpupower_com);
					printf("OPTIMIZE\n");
					sleep_time = 0;
					optimized = true;
				}
				else {
					sleep_time += 1;
					printf("sleep_time++: %d\n", sleep_time);
				}
			}
		}
		wait(1000);
	}

	return 0;
}
