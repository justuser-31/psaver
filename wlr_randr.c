#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include "wlr-output-management-unstable-v1-client-protocol.h"

struct randr_state;
struct randr_head;

struct randr_mode {
	struct randr_head *head;
	struct zwlr_output_mode_v1 *wlr_mode;
	struct wl_list link;

	int32_t width, height;
	int32_t refresh; // mHz
	bool preferred;
};

struct randr_head {
	struct randr_state *state;
	struct zwlr_output_head_v1 *wlr_head;
	struct wl_list link;

	char *name, *description;
	int32_t phys_width, phys_height; // mm
	struct wl_list modes;

	bool enabled;
	struct randr_mode *mode;
	struct {
		int32_t width, height;
		int32_t refresh;
	} custom_mode;
	int32_t x, y;
	enum wl_output_transform transform;
	double scale;
};

struct randr_state {
	struct zwlr_output_manager_v1 *output_manager;

	struct wl_list heads;
	uint32_t serial;
	bool running;
	bool failed;
};

int screen_state = 0;

static void get_s_state(struct randr_state *state) {
	struct randr_head *head;
	wl_list_for_each(head, &state->heads, link) {
		// HERE WE ARE
		// GET STATE OF SCREEN
		//printf("  Enabled: %s\n", head->enabled ? "yes" : "no");
		//return head->enabled;
		screen_state = head->enabled;
	}

	state->running = false;
}

static void config_handle_succeeded(void *data,
		struct zwlr_output_configuration_v1 *config) {
	struct randr_state *state = data;
	zwlr_output_configuration_v1_destroy(config);
	state->running = false;
}

static void config_handle_failed(void *data,
		struct zwlr_output_configuration_v1 *config) {
	struct randr_state *state = data;
	zwlr_output_configuration_v1_destroy(config);
	state->running = false;
	state->failed = true;

	fprintf(stderr, "failed to apply configuration\n");
}

static void config_handle_cancelled(void *data,
		struct zwlr_output_configuration_v1 *config) {
	struct randr_state *state = data;
	zwlr_output_configuration_v1_destroy(config);
	state->running = false;
	state->failed = true;

	fprintf(stderr, "configuration cancelled, please try again\n");
}

static const struct zwlr_output_configuration_v1_listener config_listener = {
	.succeeded = config_handle_succeeded,
	.failed = config_handle_failed,
	.cancelled = config_handle_cancelled,
};

static void mode_handle_size(void *data, struct zwlr_output_mode_v1 *wlr_mode,
		int32_t width, int32_t height) {
	struct randr_mode *mode = data;
	mode->width = width;
	mode->height = height;
}

static void mode_handle_refresh(void *data,
		struct zwlr_output_mode_v1 *wlr_mode, int32_t refresh) {
	struct randr_mode *mode = data;
	mode->refresh = refresh;
}

static void mode_handle_preferred(void *data,
		struct zwlr_output_mode_v1 *wlr_mode) {
	struct randr_mode *mode = data;
	mode->preferred = true;
}

static void mode_handle_finished(void *data,
		struct zwlr_output_mode_v1 *wlr_mode) {
	struct randr_mode *mode = data;
	wl_list_remove(&mode->link);
	zwlr_output_mode_v1_destroy(mode->wlr_mode);
	free(mode);
}

static const struct zwlr_output_mode_v1_listener mode_listener = {
	.size = mode_handle_size,
	.refresh = mode_handle_refresh,
	.preferred = mode_handle_preferred,
	.finished = mode_handle_finished,
};

static void head_handle_name(void *data,
		struct zwlr_output_head_v1 *wlr_head, const char *name) {
	struct randr_head *head = data;
	head->name = strdup(name);
}

static void head_handle_description(void *data,
		struct zwlr_output_head_v1 *wlr_head, const char *description) {
	struct randr_head *head = data;
	head->description = strdup(description);
}

static void head_handle_physical_size(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t width, int32_t height) {
	struct randr_head *head = data;
	head->phys_width = width;
	head->phys_height = height;
}

static void head_handle_mode(void *data,
		struct zwlr_output_head_v1 *wlr_head,
		struct zwlr_output_mode_v1 *wlr_mode) {
	struct randr_head *head = data;

	struct randr_mode *mode = calloc(1, sizeof(*mode));
	mode->head = head;
	mode->wlr_mode = wlr_mode;
	wl_list_insert(&head->modes, &mode->link);

	zwlr_output_mode_v1_add_listener(wlr_mode, &mode_listener, mode);
}

static void head_handle_enabled(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t enabled) {
	struct randr_head *head = data;
	head->enabled = !!enabled;
	if (!enabled) {
		head->mode = NULL;
	}
}

static void head_handle_current_mode(void *data,
		struct zwlr_output_head_v1 *wlr_head,
		struct zwlr_output_mode_v1 *wlr_mode) {
	struct randr_head *head = data;
	struct randr_mode *mode;
	wl_list_for_each(mode, &head->modes, link) {
		if (mode->wlr_mode == wlr_mode) {
			head->mode = mode;
			return;
		}
	}
	fprintf(stderr, "received unknown current_mode\n");
	head->mode = NULL;
}

static void head_handle_position(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t x, int32_t y) {
	struct randr_head *head = data;
	head->x = x;
	head->y = y;
}

static void head_handle_transform(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t transform) {
	struct randr_head *head = data;
	head->transform = transform;
}

static void head_handle_scale(void *data,
		struct zwlr_output_head_v1 *wlr_head, wl_fixed_t scale) {
	struct randr_head *head = data;
	head->scale = wl_fixed_to_double(scale);
}

static void head_handle_finished(void *data,
		struct zwlr_output_head_v1 *wlr_head) {
	struct randr_head *head = data;
	wl_list_remove(&head->link);
	zwlr_output_head_v1_destroy(head->wlr_head);
	free(head->name);
	free(head->description);
	free(head);
}

static const struct zwlr_output_head_v1_listener head_listener = {
	.name = head_handle_name,
	.description = head_handle_description,
	.physical_size = head_handle_physical_size,
	.mode = head_handle_mode,
	.enabled = head_handle_enabled,
	.current_mode = head_handle_current_mode,
	.position = head_handle_position,
	.transform = head_handle_transform,
	.scale = head_handle_scale,
	.finished = head_handle_finished,
};

static void output_manager_handle_head(void *data,
		struct zwlr_output_manager_v1 *manager,
		struct zwlr_output_head_v1 *wlr_head) {
	struct randr_state *state = data;

	struct randr_head *head = calloc(1, sizeof(*head));
	head->state = state;
	head->wlr_head = wlr_head;
	head->scale = 1.0;
	wl_list_init(&head->modes);
	wl_list_insert(&state->heads, &head->link);

	zwlr_output_head_v1_add_listener(wlr_head, &head_listener, head);
}

static void output_manager_handle_done(void *data,
		struct zwlr_output_manager_v1 *manager, uint32_t serial) {
	struct randr_state *state = data;
	state->serial = serial;
}

static void output_manager_handle_finished(void *data,
		struct zwlr_output_manager_v1 *manager) {
	// This space is intentionally left blank
}

static const struct zwlr_output_manager_v1_listener output_manager_listener = {
	.head = output_manager_handle_head,
	.done = output_manager_handle_done,
	.finished = output_manager_handle_finished,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	struct randr_state *state = data;

	if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
		state->output_manager = wl_registry_bind(registry, name,
			&zwlr_output_manager_v1_interface, 1);
		zwlr_output_manager_v1_add_listener(state->output_manager,
			&output_manager_listener, state);
	}
}

static void registry_handle_global_remove(void *data,
		struct wl_registry *registry, uint32_t name) {
	// This space is intentionally left blank
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};
