/* =========================================================
* Pebble Bugs Game - Filename: buggame.c
* Created by: Oleksandr Chernenko
* Created on: 20.02.2016
* License: GNU Public License 3.0
===========================================================*/
#include <buggame.h>

static GColor s_colors[3];
static Layer *s_playfield_layer;
static TextLayer *s_score_layer;

static Bug *s_play_field[8][8];
static int player_score = 0, bot_score = 0;

// game mechanics

static bool try_move_bug(int cx, int cy, enum Direction dir) {
	 Bug *ori_bug = s_play_field[cx][cy];
	 if (ori_bug == NULL) return false;
	
	 Bug **tgt_cell = NULL;	
	 switch (dir) {
		 case Up:
		 		printf("Moving UP");
		    if (cy==0) return false; else tgt_cell = &s_play_field[cx][cy-1];
		 		break;
		 case Down:
				 printf("Moving Down");
		    if (cy==7) return false; else tgt_cell = &s_play_field[cx][cy+1];
		 	  break;
		 case Left:
		 printf("Moving Left");
		  	if (cx==0) return false; else tgt_cell = &s_play_field[cx-1][cy];
		   	break;
		 case Right:
		 printf("Moving Right");
		  	if (cx==7) return false; else tgt_cell = &s_play_field[cx+1][cy];
		 	break;
	 }
	
	Bug *tgt_bug = ((Bug*)(*tgt_cell));
	if (tgt_bug!=NULL && tgt_bug->isPlayer == ori_bug->isPlayer) { printf("stuck into own bug"); return false; }
	if (tgt_bug!=NULL && ((ori_bug->type == Red && tgt_bug->type != Blue) ||
											  (ori_bug->type == Blue && tgt_bug->type != Green) ||
												(ori_bug->type == Green && tgt_bug->type != Red))  )  {
		printf("Spotted Enemy whom I cannot beat");
		return false;
	}
	ori_bug->isProcessed = true;
	*tgt_cell = ori_bug;
	s_play_field[cx][cy]=NULL; 
	if (tgt_bug!=NULL) {
		printf("Beaten the enemy!!!");
		if (ori_bug->isPlayer) player_score++; else bot_score++;
		static char s_buffer[10];
		snprintf(s_buffer, sizeof(s_buffer), "%d : %d", bot_score, player_score);
    text_layer_set_text(s_score_layer, s_buffer);
		free(tgt_bug);
	}
	return true;
	
}

static enum Direction choose_direction(int cx, int cy) {
	bool isPlayer = s_play_field[cx][cy]->isPlayer;
	if (isPlayer) {		
		if (cx!=0 && s_play_field[cx-1][cy]!= NULL && s_play_field[cx-1][cy]->isPlayer != isPlayer) { printf("Player(%d,%d): Enemy Ahead, Attacking Left", cx,cy); return Left; }	 
		if (cy!=0 && s_play_field[cx][cy-1]!= NULL && s_play_field[cx][cy-1]->isPlayer != isPlayer) { printf("Player(%d,%d): Enemy Above, Attacking Up", cx,cy); return Up; }
	  if (cy!=7 && s_play_field[cx][cy+1]!= NULL && s_play_field[cx][cy+1]->isPlayer != isPlayer) { printf("Player(%d,%d): Enemy Below, Attacking Down", cx,cy);return Down; }
		if (cx!=7 && s_play_field[cx+1][cy]!= NULL && s_play_field[cx+1][cy]->isPlayer != isPlayer) { printf("Player(%d,%d): Enemy Behind, Attacking Right", cx,cy);return Right; }
		printf("Player(%d,%d): No bots around - go ahead!", cx, cy);
		return Left;
	} else 
		{
		if (cx!=7 && s_play_field[cx+1][cy]!= NULL && s_play_field[cx+1][cy]->isPlayer != isPlayer) { printf("Bot(%d,%d): Enemy Behind, Attacking Right", cx,cy);return Right; } 
		if (cy!=0 && s_play_field[cx][cy-1]!= NULL && s_play_field[cx][cy-1]->isPlayer != isPlayer) { printf("Bot(%d,%d): Enemy Above, Attacking Up", cx,cy); return Up; }
	  if (cy!=7 && s_play_field[cx][cy+1]!= NULL && s_play_field[cx][cy+1]->isPlayer != isPlayer) { printf("Bot(%d,%d): Enemy Below, Attacking Down", cx,cy);return Down; }		
		if (cx!=0 && s_play_field[cx-1][cy]!= NULL && s_play_field[cx-1][cy]->isPlayer != isPlayer) { printf("Bot(%d,%d): Enemy Ahead, Attacking Left", cx,cy); return Left; }
		printf("Bot(%d,%d): No creatures around - go ahead!", cx, cy);
		return Right;
	}	  
}

static bool spawn_bug(bool isForPlayer, enum BugType type) {
		int cx = isForPlayer?7:0, cy = rand() % 8;
		if (s_play_field[cx][cy]!=NULL) return false;
	  Bug *bug  = (Bug*)malloc(sizeof(Bug));
	  bug->type = type;
		bug->isPlayer = isForPlayer;	
	  bug->isProcessed = false;
	  s_play_field[cx][cy] = 	bug;
		return true;	
}

void game_tick() {
	Bug *bug = NULL;
	for (int cx = 0; cx < 8; cx++ ) {
			for (int cy = 0; cy < 8; cy++) {
				bug = s_play_field[cx][cy];
				if (bug==NULL) continue;
				 if (!bug->isProcessed) {
					 	// if there is an enemy around attack in following priority (Back, Up, Down, Forward) otherwise by default try move forward.
						if (try_move_bug(cx,cy,choose_direction(cx,cy))) continue;
					  // if forward is blocked by own bug - move up/down to evade
            int up = rand() % 2;
					 	 if (up==1) {
							  if (try_move_bug(cx,cy,Up)) continue;
							  if (try_move_bug(cx,cy,Down)) continue;
						   } else		{
							  if (try_move_bug(cx,cy,Down)) continue;
							  if (try_move_bug(cx,cy,Up)) continue;
						 }
					 // finally try move backwards
					 if (bug->isPlayer) { if (try_move_bug(cx,cy,Right)) continue; }
						else if (try_move_bug(cx,cy,Left)) continue;				 
				 } 
			}
	}
	// clean isProcessed flag
	for (int cx = 0; cx < 8; cx++ ) {
			for (int cy = 0; cy < 8; cy++) {
				bug = s_play_field[cx][cy];
				if (bug==NULL) continue;
				bug->isProcessed = false;
			}
	}
	layer_mark_dirty(s_playfield_layer);
	
}

// scene rendering

static void redraw_playfield_layer(Layer *layer, GContext *ctx) {
	time_t t_utc_start, t_utc_end;
	uint16_t out_ms;
	uint16_t start  = time_ms(&t_utc_start, &out_ms);

	GRect rect;
	Bug *bug = NULL;
		for (int cx = 0; cx < 8; cx ++ ) {
			for (int cy = 0; cy < 8; cy++) {
				bug = s_play_field[cx][cy];
				if (bug==NULL) continue;
				  rect = GRect(cx*16 + 8, cy*16 + 24 , 16,16);
				  if (bug->isPlayer) {
					graphics_context_set_fill_color(ctx, s_colors[bug->type]);
				  graphics_fill_rect(ctx, rect, 0, GCornerNone);
					}
					else { graphics_context_set_stroke_color(ctx, s_colors[bug->type]);
					graphics_draw_rect(ctx, rect);
				}
				  
						
			}
		}
	// draw color bases in 2 passes for bot and player
		  rect = (GRect){ .origin = { 0 , 24 }, .size = { 8 , 16 } };
			while (rect.origin.x<144) {
			graphics_context_set_fill_color(ctx, GColorRed);
			graphics_fill_rect(ctx, rect, 0, GCornerNone);
			rect.origin.y += 48;
			graphics_context_set_fill_color(ctx, GColorBlue);
			graphics_fill_rect(ctx, rect, 0, GCornerNone);
			rect.origin.y += 48;
			graphics_context_set_fill_color(ctx, GColorGreen);
			graphics_fill_rect(ctx, rect, 0, GCornerNone);
			rect.origin.x += 8 + 8*16;
			rect.origin.y = 24;
			}	
uint16_t end_ms = time_ms(&t_utc_end, &out_ms);
 printf("Miliseconds redraw took: %d", (int)((t_utc_end - t_utc_start) + (end_ms - start)/1000));
}

// init, interactions and shutdown

static void create_layers(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	s_playfield_layer = layer_create(bounds);
	layer_set_update_proc(s_playfield_layer, redraw_playfield_layer);
	layer_add_child(window_layer, s_playfield_layer);
	s_score_layer = text_layer_create((GRect) { .origin = { 0, 8 }, .size = { bounds.size.w, 16 } });
  text_layer_set_text(s_score_layer, "0 : 0");
	text_layer_set_font(s_score_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_score_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_score_layer));
}

static void clear_playfield(){
	for (int cx = 0; cx < 8; cx ++ ) {
			for (int cy = 0; cy < 8; cy++) {
				if (s_play_field[cx][cy]!= NULL) {
					free(s_play_field[cx][cy]);
					s_play_field[cx][cy] = NULL;
				}
			}
	}
}

void blue_click_handler(ClickRecognizerRef recognizer, void *context) {  
	 spawn_bug(true, Blue);
	 spawn_bug(false, rand() % 3);
}

void red_click_handler(ClickRecognizerRef recognizer, void *context) { 
	 spawn_bug(true, Red);
	 spawn_bug(false, rand() % 3);
}

void green_click_handler(ClickRecognizerRef recognizer, void *context) {   
	 spawn_bug(true, Green);
	 spawn_bug(false, rand() % 3);
}

void warm_up(Window *window) {
	s_colors[0]=GColorRed;
	s_colors[1]=GColorBlue;
	s_colors[2]=GColorGreen;
  create_layers(window);
}

void clean_up() {
	clear_playfield();
  text_layer_destroy(s_score_layer);
	layer_destroy(s_playfield_layer);
}

