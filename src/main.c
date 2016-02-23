#include <buggame.h>

static Window *window;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	game_tick();
}

static void click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_SELECT, blue_click_handler);
	window_single_click_subscribe(BUTTON_ID_UP, red_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, green_click_handler);
}

static void window_load(Window *window) {
	warm_up(window);
	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void window_unload(Window *window) {
	clean_up();
}

static void init(void) {
	window = window_create();
	window_set_click_config_provider(window, click_config_provider);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
			.unload = window_unload,
	});
	const bool animated = true;
	window_stack_push(window, animated);
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}