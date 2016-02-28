/* =========================================================
* Pebble Bugs Game - Filename: buggame.c
* Created by: Oleksandr Chernenko
* Created on: 20.02.2016
* License: GNU Public License 3.0
===========================================================*/
#include <buggame.h>
//#define DEBUG 0
#define SIZE 14
#define MARGIN 1
#define STEP 16


const uint8_t dir_chances[4] = { 60, 37, 14 };

static GColor s_colors[3];
static Layer *s_window_layer;
static Layer *s_bases_layer;
static TextLayer *s_score_layer;
static TextLayer *s_status_layer;

static Bug s_bugs_pool[64];
static int player_score = 0, bot_score = 0;
static uint8_t total_bugs = 0, player_bugs = 0, bot_bugs = 0;
static bool isRunning = false;

// game mechanics

static Bug* detect_collision(const uint8_t cx, const uint8_t cy) {
	for (uint8_t i = 0; i < 64; i++) {
		if (!s_bugs_pool[i].isAlive) continue;
		if (cx <= (s_bugs_pool[i].cx + 15) && (cx + 15) >= s_bugs_pool[i].cx &&
		        cy <= (s_bugs_pool[i].cy + 15) && (cy + 15) >= s_bugs_pool[i].cy) return &s_bugs_pool[i];
	}
	return NULL;
}

static bool try_kill(Bug *ori_bug, Bug *tgt_bug) {
	if (ori_bug == NULL || !ori_bug->isAlive) {
#ifdef DEBUG
		printf("Attacker is undefined or already dead");
#endif
		return false;
	}
	if (tgt_bug == NULL || !tgt_bug->isAlive) {
#ifdef DEBUG
		printf("Target is undefined or already dead");
#endif
		return false;
	}
	if (tgt_bug->isPlayer == ori_bug->isPlayer) {
#ifdef DEBUG
		printf("Friendly Fire! Stop It!");
#endif
		return false;
	}
	if (tgt_bug != NULL && ((ori_bug->type == Red && tgt_bug->type != Blue) ||
	                        (ori_bug->type == Blue && tgt_bug->type != Green) ||
	                        (ori_bug->type == Green && tgt_bug->type != Red))  )  {
#ifdef DEBUG
		printf("Spotted Enemy whom I cannot beat. Retreat!");
#endif
		return false;
	}
//kill him!
	tgt_bug->isAlive = false;
	layer_remove_from_parent(tgt_bug->layer);
	total_bugs--;
#ifdef DEBUG
	printf("Victory! I have beaten the enemy!!!");
#endif
	if (ori_bug->isPlayer)  { player_score++; bot_bugs--; } else { bot_score++; player_bugs--; }
	update_status();
	if (player_bugs == 0 || bot_bugs == 0) stop_game();
	return true;
}

static bool try_move_bug(Bug* ori_bug, enum Direction dir) {
	if (ori_bug == NULL) return false;
	uint8_t tcx = ori_bug->cx, tcy = ori_bug->cy;
	//printf(" %d %d ", tcx, tcy);
	switch (dir) {
	case Up:

#ifdef DEBUG
		printf("Moving UP");
#endif
		if (ori_bug->cy <= 24) return false; else tcy = ori_bug->cy - STEP;
		break;
	case Down:

#ifdef DEBUG
		printf("Moving Down");
#endif
		if (ori_bug->cy >= 136) return false; else tcy = ori_bug->cy + STEP;
		break;
	case Left:

#ifdef DEBUG
		printf("Moving Left");
#endif
		if (ori_bug->cx <= 8 ) return false; else tcx = ori_bug->cx - STEP;
		break;
	case Right:

#ifdef DEBUG
		printf("Moving Right");
#endif
		if (ori_bug->cx >= 120) return false; else tcx = ori_bug->cx + STEP;
		break;
	}

//	printf ("Moving from %d %d to %d %d", ori_bug->cx, ori_bug->cy, tcx, tcy);
//detect if there is any collision with phantom
	Bug *tgt = detect_collision(tcx, tcy);
	while (tgt != NULL && tgt->isPlayer != ori_bug->isPlayer && try_kill(ori_bug, tgt)) detect_collision(tcx, tcy); // iterate until kill all enemies
	if (tgt != NULL) return false; // could not kill colliding enemy or that is same side bug => fail
//move there - it's finally empty

	animate_move(ori_bug, tcx, tcy);
	ori_bug->cx = tcx;
	ori_bug->cy = tcy;
	return true;
}

static enum Direction rand_dir(bool isPlayer) {
	uint8_t dir = rand() % 100;
	if (isPlayer) {
		if (dir > dir_chances[0]) return Left;
		if (dir > dir_chances[1]) return Up;
		if (dir > dir_chances[2]) return Down;
		return Right;
	} else {
		if (dir > dir_chances[0]) return Right;
		if (dir > dir_chances[1]) return Up;
		if (dir > dir_chances[2]) return Down;
		return Left;
	}
}

static enum Direction choose_direction(Bug *bug) {
	bool isPlayer = bug->isPlayer;
	uint8_t cx = bug->cx;
	uint8_t cy = bug->cy;
		if (isPlayer) {
			if (cx >= 16 && detect_collision(cx - STEP, cy) != NULL &&  detect_collision(cx - STEP, cy)->isPlayer!= isPlayer) {
	#ifdef DEBUG
				printf("Player(%d,%d): Enemy Ahead, Attacking Left", cx, cy);
	#endif
				return Left;
			}
			if (cy >= 40 && detect_collision(cx, cy - STEP ) != NULL &&  detect_collision(cx, cy - STEP )->isPlayer!= isPlayer) {
	#ifdef DEBUG
				printf("Player(%d,%d): Enemy Above, Attacking Up", cx, cy);
	#endif
				return Up;
			}
			if (cy <= 120 && detect_collision(cx, cy + STEP ) != NULL &&  detect_collision(cx, cy + STEP )->isPlayer!= isPlayer) {
	#ifdef DEBUG
				printf("Player(%d,%d): Enemy Below, Attacking Down", cx, cy);
	#endif
				return Down;
			}
			if (cx <= 104 && detect_collision(cx + STEP, cy) != NULL &&  detect_collision(cx + STEP, cy)->isPlayer!= isPlayer) {
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
		if (cx <= 104 && detect_collision(cx + STEP, cy) != NULL &&  detect_collision(cx + STEP, cy)->isPlayer!= isPlayer) {
	#ifdef DEBUG
				printf("Bot(%d,%d): Enemy Behind, Attacking Right", cx, cy);
	#endif
				return Right;
			}
		if (cy >= 40 && detect_collision(cx, cy - STEP ) != NULL &&  detect_collision(cx, cy - STEP )->isPlayer!= isPlayer) {
	#ifdef DEBUG
				printf("Bot(%d,%d): Enemy Above, Attacking Up", cx, cy);
	#endif
				return Up;
			}
			if (cy <= 120 && detect_collision(cx, cy + STEP ) != NULL &&  detect_collision(cx, cy + STEP )->isPlayer!= isPlayer) {
	#ifdef DEBUG
				printf("Bot(%d,%d): Enemy Below, Attacking Down", cx, cy);
	#endif
				return Down;
			}
		if (cx >= 16 && detect_collision(cx - STEP, cy) != NULL &&  detect_collision(cx - STEP, cy)->isPlayer!= isPlayer) {
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

static Bug* find_dead_bug() {
	for (uint8_t i = 0; i < 64; i++) {
		if (!s_bugs_pool[i].isAlive) return &s_bugs_pool[i];
	}
	return NULL;
}

static bool spawn_bug(bool isPlayer, enum BugType type) {
	if (total_bugs >= 64) return false;
	uint8_t cx = isPlayer ? 120 : 8 , cy = 16 * (rand() % 8) + 24;
	Bug *bug  = find_dead_bug();
	if (bug == NULL) return false;
	bug->cx = cx;
	bug->cy = cy;
	bug->isPlayer = isPlayer;
	bug->type = type;
	Bug *tgt = detect_collision(bug->cx, bug->cy);
	while (tgt != NULL && tgt->isPlayer != isPlayer && try_kill(bug, tgt)) detect_collision(bug->cx, bug->cy); // iterate until kill all enemies
	if (tgt != NULL) return false; // could not kill colliding enemy or that is same side bug => fail
	bug->isAlive = true;
	total_bugs++;
	if (isPlayer) player_bugs++; else bot_bugs++;
	update_status();
	layer_set_frame(bug->layer, GRect(cx + MARGIN , cy + MARGIN, SIZE, SIZE));
	layer_add_child(s_window_layer, bug->layer);
	animate_spawn(bug);
	return true;
}

void game_tick() {
	if (!isRunning) return;
	spawn_bug(false, rand() % 3);
	Bug *bug = NULL;
	for (uint8_t i = 0; i < 64; i++ ) {
		bug = &s_bugs_pool[i];
		if (!bug->isAlive) continue;
		if (bug == NULL) continue;
		// if there is an enemy around attack in following priority (Back, Up, Down, Forward) otherwise by default try move forward.
		if (try_move_bug(bug, choose_direction(bug))) continue;
		// if forward is blocked by own bug - move up/down to evade
		uint8_t up = rand() % 2;
		if (up == 1) {
			if (try_move_bug(bug, Up)) continue;
			if (try_move_bug(bug, Down)) continue;
		} else		{
			if (try_move_bug(bug, Down)) continue;
			if (try_move_bug(bug, Up)) continue;
		}
		// finally try move backwards
		if (bug->isPlayer) { if (try_move_bug(bug, Right)) continue; }
		else if (try_move_bug(bug, Left)) continue;
	}
}

// scene rendering

static void redraw_bases_layer(Layer *layer, GContext *ctx) {
	if (!isRunning) return;
	// draw color bases in 2 passes for bot and player
	GRect rect = (GRect) { .origin = { 0 , MARGIN }, .size = { 8 - MARGIN , SIZE } };
	while (rect.origin.x < 144) {
		graphics_context_set_fill_color(ctx, GColorRed);
		graphics_fill_rect(ctx, rect, 0, GCornerNone);
		rect.origin.y += SIZE + 2 * 16 + 2 * MARGIN;
		graphics_context_set_fill_color(ctx, GColorBlue);
		graphics_fill_rect(ctx, rect, 0, GCornerNone);
		rect.origin.y += SIZE + 2 * 16 + 2 * MARGIN;
		graphics_context_set_fill_color(ctx, GColorGreen);
		graphics_fill_rect(ctx, rect, 0, GCornerNone);
		rect.origin.x += 8 + MARGIN + 8 * 16;
		rect.origin.y = MARGIN;
	}
}

static void render_bug(Layer *layer, GContext *ctx) {
	Bug *bug = *(Bug**)layer_get_data(layer);
	graphics_context_set_fill_color(ctx, s_colors[bug->type]);
	graphics_context_set_stroke_color(ctx, s_colors[bug->type]);
	if (bug->isPlayer) graphics_fill_rect(ctx, layer_get_bounds(layer) , 4, GCornersAll);
	else graphics_draw_round_rect(ctx, layer_get_bounds(layer) , 4);
}

void animate_spawn(Bug *bug) {
	GRect start = GRect(bug->cx + MARGIN + SIZE / 2, bug->cy + MARGIN + SIZE / 2, 0, 0);
	GRect finish = GRect(bug->cx + MARGIN, bug->cy + MARGIN, SIZE, SIZE);
	PropertyAnimation *prop_anim_move_1_a = property_animation_create_layer_frame(bug->layer, &start, &finish);
	Animation *anim_move_1_a = property_animation_get_animation(prop_anim_move_1_a);
	animation_set_duration(anim_move_1_a, 500);
	animation_schedule(anim_move_1_a);
}

void animate_move(Bug *bug, const uint8_t x, const uint8_t y) {
	const uint8_t sx = bug->cx;
	const uint8_t sy = bug->cy;
	GRect start = GRect(sx + MARGIN, sy + MARGIN, SIZE, SIZE);
	GRect finish = GRect(x + MARGIN, y + MARGIN, SIZE, SIZE);
	PropertyAnimation *prop_anim_move_1_a = property_animation_create_layer_frame(bug->layer, &start, &finish);
	Animation *anim_move_1_a = property_animation_get_animation(prop_anim_move_1_a);
	animation_set_duration(anim_move_1_a, 500);
	animation_schedule(anim_move_1_a);
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
	s_window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(s_window_layer);
	s_score_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 24 } });
	text_layer_set_text(s_score_layer, "0 : 0");
	text_layer_set_font(s_score_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	text_layer_set_text_alignment(s_score_layer, GTextAlignmentCenter);
	layer_add_child(s_window_layer, text_layer_get_layer(s_score_layer));
	s_status_layer = text_layer_create((GRect) { .origin = { 0, 152 }, .size = { bounds.size.w, 16 } });
	text_layer_set_text(s_status_layer, "Bot:0 Total:0 Play:0");
	text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
	layer_add_child(s_window_layer, text_layer_get_layer(s_status_layer));
	s_bases_layer = layer_create((GRect) { .origin = { 0, 24 }, .size = { bounds.size.w, 16 * 8 } });
	layer_set_update_proc(s_bases_layer, redraw_bases_layer);
	layer_add_child(s_window_layer, s_bases_layer);
}

void clear_playfield() {
	total_bugs = player_bugs = bot_bugs = 0;
	for (uint8_t i = 0; i < 64; i++) {
		layer_destroy(s_bugs_pool[i].layer);
		s_bugs_pool[i].isAlive = false;
	}
}

void blue_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (isRunning) //game_tick();
		spawn_bug(true, Blue);
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

void init_bugs() {
	for (uint8_t i = 0; i < 64; i++) {
		s_bugs_pool[i].isAlive = false;
		s_bugs_pool[i].layer = layer_create_with_data(GRect(0, 0 , 16, 16), sizeof(Bug*));
		*((Bug**)layer_get_data(s_bugs_pool[i].layer)) = &s_bugs_pool[i];
		layer_set_update_proc(s_bugs_pool[i].layer, render_bug);
	}
}

void warm_up(Window *window) {
	s_colors[0] = GColorRed;
	s_colors[1] = GColorBlue;
	s_colors[2] = GColorGreen;
	create_layers(window);
	init_bugs();
	isRunning = true;
	light_enable(true);
}

void stop_game() {
	if (player_bugs == 0) bot_score += bot_bugs;
	else if (bot_bugs == 0) player_score += player_bugs;
	isRunning = false;
	light_enable(false);
	update_status();
}

void clean_up() {
	isRunning = false;
	clear_playfield();
	text_layer_destroy(s_status_layer);
	text_layer_destroy(s_score_layer);
}




