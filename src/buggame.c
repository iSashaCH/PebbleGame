/* =========================================================
* Pebble Bugs Game - Filename: buggame.c
* Created by: Oleksandr Chernenko
* Created on: 20.02.2016
* License: GNU Public License 3.0
===========================================================*/
#include <buggame.h>
//#define DEBUG 0

const uint8_t dir_chances[4] = { 60, 37, 14 };

static GColor s_colors[3];
static Layer *s_playfield_layer;
static TextLayer *s_score_layer;
static TextLayer *s_status_layer;

static Bug *s_play_field[8][8];
static int player_score = 0, bot_score = 0;
static uint8_t total_bugs = 0, player_bugs = 0, bot_bugs = 0;
static bool isRunning = false;

// game mechanics

static void find_closest_enemy(Bug *bug) {

}

static bool try_move_bug(uint8_t cx, uint8_t cy, enum Direction dir) {
	Bug *ori_bug = s_play_field[cx][cy];
	if (ori_bug == NULL) return false;

	Bug **tgt_cell = NULL;
	switch (dir) {
	case Up:

#ifdef DEBUG
		printf("Moving UP");
#endif
		if (cy == 0) return false; else tgt_cell = &s_play_field[cx][cy - 1];
		break;
	case Down:

#ifdef DEBUG
		printf("Moving Down");
#endif
		if (cy == 7) return false; else tgt_cell = &s_play_field[cx][cy + 1];
		break;
	case Left:

#ifdef DEBUG
		printf("Moving Left");
#endif
		if (cx == 0) return false; else tgt_cell = &s_play_field[cx - 1][cy];
		break;
	case Right:

#ifdef DEBUG
		printf("Moving Right");
#endif
		if (cx == 7) return false; else tgt_cell = &s_play_field[cx + 1][cy];
		break;
	}

	Bug *tgt_bug = ((Bug*)(*tgt_cell));
	if (tgt_bug != NULL && tgt_bug->isPlayer == ori_bug->isPlayer) {
#ifdef DEBUG
		printf("stuck into own bug");
#endif
		return false;
	}
	if (tgt_bug != NULL && ((ori_bug->type == Red && tgt_bug->type != Blue) ||
	                        (ori_bug->type == Blue && tgt_bug->type != Green) ||
	                        (ori_bug->type == Green && tgt_bug->type != Red))  )  {
#ifdef DEBUG
		printf("Spotted Enemy whom I cannot beat");
#endif
		return false;
	}
	ori_bug->isProcessed = true;
	*tgt_cell = ori_bug;
	s_play_field[cx][cy] = NULL;
	if (tgt_bug != NULL) {
		total_bugs--;
#ifdef DEBUG
		printf("Beaten the enemy!!!");
#endif
		if (ori_bug->isPlayer)  { player_score++; bot_bugs--; } else { bot_score++; player_bugs--; }
		update_status();	
		free(tgt_bug);
		if (player_bugs==0 || bot_bugs==0) stop_game();
	}
	return true;

}

static enum Direction rand_dir(bool isPlayer) {
	uint8_t dir = rand() % 100;
	if (isPlayer) {
		if (dir>dir_chances[0]) return Left;
		if (dir>dir_chances[1]) return Up;
		if (dir>dir_chances[2]) return Down;
		return Right;
	} else {
		if (dir>dir_chances[0]) return Right;
		if (dir>dir_chances[1]) return Up;
		if (dir>dir_chances[2]) return Down;
		return Left;		
	}
}

static enum Direction choose_direction(uint8_t cx, uint8_t cy) {
	bool isPlayer = s_play_field[cx][cy]->isPlayer;
	if (isPlayer) {
		if (cx != 0 && s_play_field[cx - 1][cy] != NULL && s_play_field[cx - 1][cy]->isPlayer != isPlayer) {
#ifdef DEBUG
			printf("Player(%d,%d): Enemy Ahead, Attacking Left", cx, cy);
#endif
			return Left;
		}
		if (cy != 0 && s_play_field[cx][cy - 1] != NULL && s_play_field[cx][cy - 1]->isPlayer != isPlayer) {
#ifdef DEBUG
			printf("Player(%d,%d): Enemy Above, Attacking Up", cx, cy);
#endif
			return Up;
		}
		if (cy != 7 && s_play_field[cx][cy + 1] != NULL && s_play_field[cx][cy + 1]->isPlayer != isPlayer) {
#ifdef DEBUG
			printf("Player(%d,%d): Enemy Below, Attacking Down", cx, cy);
#endif
			return Down;
		}
		if (cx != 7 && s_play_field[cx + 1][cy] != NULL && s_play_field[cx + 1][cy]->isPlayer != isPlayer) {

#ifdef DEBUG
			printf("Player(%d,%d): Enemy Behind, Attacking Right", cx, cy);
#endif
			return Right;
		}
#ifdef DEBUG
		printf("Player(%d,%d): No bots around - go random!", cx, cy);
#endif
		return rand_dir(true);
	} else
	{
		if (cx != 7 && s_play_field[cx + 1][cy] != NULL && s_play_field[cx + 1][cy]->isPlayer != isPlayer) {
#ifdef DEBUG
			printf("Bot(%d,%d): Enemy Behind, Attacking Right", cx, cy);
#endif
			return Right;
		}
		if (cy != 0 && s_play_field[cx][cy - 1] != NULL && s_play_field[cx][cy - 1]->isPlayer != isPlayer) {
#ifdef DEBUG
			printf("Bot(%d,%d): Enemy Above, Attacking Up", cx, cy);
#endif
			return Up;
		}
		if (cy != 7 && s_play_field[cx][cy + 1] != NULL && s_play_field[cx][cy + 1]->isPlayer != isPlayer) {
#ifdef DEBUG
			printf("Bot(%d,%d): Enemy Below, Attacking Down", cx, cy);
#endif
			return Down;
		}
		if (cx != 0 && s_play_field[cx - 1][cy] != NULL && s_play_field[cx - 1][cy]->isPlayer != isPlayer) {
#ifdef DEBUG
			printf("Bot(%d,%d): Enemy Ahead, Attacking Left", cx, cy);
#endif
			return Left;
		}

#ifdef DEBUG
		printf("Bot(%d,%d): No creatures around - go ahead!", cx, cy);
#endif
		return rand_dir(false);
	}
}

static bool spawn_bug(bool isPlayer, enum BugType type) {
	uint8_t cx = isPlayer ? 7 : 0, cy = rand() % 8;
	if (s_play_field[cx][cy] != NULL) return false;
	Bug *bug  = (Bug*)malloc(sizeof(Bug));
	bug->type = type;
	bug->isPlayer = isPlayer;
	bug->isProcessed = false;
	s_play_field[cx][cy] = 	bug;
	total_bugs++;
	if (isPlayer) player_bugs++; else bot_bugs++;
	update_status();
	return true;
}

void game_tick() {
	if (!isRunning) return;
	spawn_bug(false, rand() % 3);
	Bug *bug = NULL;
	for (uint8_t cx = 0; cx < 8; cx++ ) {
		for (uint8_t cy = 0; cy < 8; cy++) {
			bug = s_play_field[cx][cy];
			if (bug == NULL) continue;
			if (!bug->isProcessed) {
				// if there is an enemy around attack in following priority (Back, Up, Down, Forward) otherwise by default try move forward.
				if (try_move_bug(cx, cy, choose_direction(cx, cy))) continue;
				// if forward is blocked by own bug - move up/down to evade
				uint8_t up = rand() % 2;
				if (up == 1) {
					if (try_move_bug(cx, cy, Up)) continue;
					if (try_move_bug(cx, cy, Down)) continue;
				} else		{
					if (try_move_bug(cx, cy, Down)) continue;
					if (try_move_bug(cx, cy, Up)) continue;
				}
				// finally try move backwards
				if (bug->isPlayer) { if (try_move_bug(cx, cy, Right)) continue; }
				else if (try_move_bug(cx, cy, Left)) continue;
			}
			
		}
		
	}
	// clean isProcessed flag
	for (uint8_t cx = 0; cx < 8; cx++ ) {
		for (uint8_t cy = 0; cy < 8; cy++) {
			bug = s_play_field[cx][cy];
			if (bug == NULL) continue;
			bug->isProcessed = false;
		}
	}
		layer_mark_dirty(s_playfield_layer);
}

// scene rendering

static void redraw_playfield_layer(Layer *layer, GContext *ctx) {
	if (!isRunning) return;
#ifdef DEBUG
	time_t t_utc_start, t_utc_end;
	uint16_t out_ms;
	uint16_t start  = time_ms(&t_utc_start, &out_ms);
#endif
	
	GRect rect;
	Bug *bug = NULL;
	for (uint8_t cx = 0; cx < 8; cx++ ) {
		for (uint8_t cy = 0; cy < 8; cy++) {
			bug = s_play_field[cx][cy];
			if (bug == NULL) continue;
			rect = GRect(cx * 16 + 8 + 2, cy * 16 + 2 , 14, 14);
			if (bug->isPlayer) {
				graphics_context_set_fill_color(ctx, s_colors[bug->type]);
				graphics_fill_rect(ctx, rect, 0, GCornerNone);
			}
			else {
				graphics_context_set_stroke_color(ctx, s_colors[bug->type]);
				graphics_draw_rect(ctx, rect);
			}
		}
	}
	// draw color bases in 2 passes for bot and player
	rect = (GRect) { .origin = { 0 , 1 }, .size = { 6 , 14 } };
	while (rect.origin.x < 144) {
		graphics_context_set_fill_color(ctx, GColorRed);
		graphics_fill_rect(ctx, rect, 0, GCornerNone);
		rect.origin.y += 50;
		graphics_context_set_fill_color(ctx, GColorBlue);
		graphics_fill_rect(ctx, rect, 0, GCornerNone);
		rect.origin.y += 50;
		graphics_context_set_fill_color(ctx, GColorGreen);
		graphics_fill_rect(ctx, rect, 0, GCornerNone);
		rect.origin.x += 10 + 8 * 16;
		rect.origin.y = 0;
	}
	
#ifdef DEBUG
	uint16_t end_ms = time_ms(&t_utc_end, &out_ms);
	printf("Miliseconds redraw took: %d", (int)((t_utc_end - t_utc_start) + (end_ms - start) / 1000));
#endif
}

void update_status() {
	  static char s_status_buffer[27];
	  static char s_score_buffer[10];
	  if (isRunning)
			snprintf(s_status_buffer, sizeof(s_status_buffer), "Bot:%d Total:%d Play:%d", bot_bugs, total_bugs, player_bugs);
		else 
			snprintf(s_status_buffer, sizeof(s_status_buffer), "GAME OVER");
		text_layer_set_text(s_status_layer, s_status_buffer); 
		snprintf(s_score_buffer, sizeof(s_score_buffer), "%d : %d", bot_score, player_score);
		text_layer_set_text(s_score_layer, s_score_buffer);
}

// init, interactions and shutdown

static void create_layers(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	s_playfield_layer = layer_create(GRect(0, 24, bounds.size.w, bounds.size.h - 40));
	layer_set_update_proc(s_playfield_layer, redraw_playfield_layer);
	layer_add_child(window_layer, s_playfield_layer);
	s_score_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 24 } });
	text_layer_set_text(s_score_layer, "0 : 0");
	text_layer_set_font(s_score_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	text_layer_set_text_alignment(s_score_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(s_score_layer));
	s_status_layer = text_layer_create((GRect) { .origin = { 0, 152 }, .size = { bounds.size.w, 16 } });
	text_layer_set_text(s_status_layer, "Bot:0 Total:0 Play:0");
	text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(s_status_layer));
}

void clear_playfield() {
	for (uint8_t cx = 0; cx < 8; cx++ ) {
		for (uint8_t cy = 0; cy < 8; cy++) {
			if (s_play_field[cx][cy] != NULL) {
				free(s_play_field[cx][cy]);
				s_play_field[cx][cy] = NULL;
				total_bugs--;
			}
		}
	}
	player_bugs = bot_bugs = 0;
}

void blue_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (isRunning) spawn_bug(true, Blue);
/*	else {
			player_score = bot_score = 0;
			clear_playfield();
			update_status();
			isRunning = true;
	} */
}

void red_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (isRunning) spawn_bug(true, Red);
}

void green_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (isRunning) spawn_bug(true, Green);
}

void warm_up(Window *window) {
	s_colors[0] = GColorRed;
	s_colors[1] = GColorBlue;
	s_colors[2] = GColorGreen;
	create_layers(window);
	isRunning = true;
}

void stop_game() {	
	if (player_bugs==0) bot_score+=bot_bugs;
	 else if (bot_bugs==0) player_score+=player_bugs;
		 layer_mark_dirty(s_playfield_layer);
	isRunning = false;
	update_status();
}

void clean_up() {
	isRunning = false;
	clear_playfield();
	text_layer_destroy(s_status_layer);
	text_layer_destroy(s_score_layer);
	layer_destroy(s_playfield_layer);
}


