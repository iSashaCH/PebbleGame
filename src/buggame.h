/* =========================================================
* Pebble Bugs Game - Filename: buggame.h
* Created by: Oleksandr Chernenko
* Created on: 20.02.2016
* License: GNU Public License 3.0
===========================================================*/
#pragma once
#include <pebble.h>

enum BugType { Red = 0, Blue = 1, Green = 2 };

enum Direction { Up = 0 , Down = 1, Left = 2, Right = 3 };

typedef struct {
	 bool isPlayer;
	 bool isAlive;
	 enum BugType type;
	 uint8_t cx;
	 uint8_t cy;
	 Layer *layer;
	
} Bug;

// button handlers
void blue_click_handler(ClickRecognizerRef recognizer, void *context);
void red_click_handler(ClickRecognizerRef recognizer, void *context);
void green_click_handler(ClickRecognizerRef recognizer, void *context);

// start, tick and stop
void warm_up(Window *window);
void game_tick();
void clean_up();

void clear_playfield();
void update_status();
void stop_game();

void animate_spawn(Bug *bug);
void animate_move(Bug *bug, const uint8_t x, const uint8_t y);