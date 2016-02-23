#include <pebble.h>

static Layer *s_grid_layer;

static void redraw_grid_layer(Layer *layer, GContext *ctx) {
	GPoint p0 = GPoint(0, 24);
	GPoint p1 = GPoint(144, 24);
	graphics_context_set_stroke_color(ctx, GColorBlack);
	while (p0.y < 153) {
		graphics_draw_line(ctx, p0, p1);
		p0.y += 16;
		p1.y += 16;
	}
	p0 = GPoint(8, 24);
	p1 = GPoint(8, 152);
	while (p0.x < 144) {
		graphics_draw_line(ctx, p0, p1);
		p0.x += 16;
		p1.x += 16;
	}

}