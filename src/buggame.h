/* =========================================================
* Pebble Bugs Game - Filename: buggame.h
* Created by: Oleksandr Chernenko
* Created on: 20.02.2016
* License: GNU Public License 3.0
===========================================================*/
#pragma once
#include <pebble.h>

enum BugType { Red = 0, Blue = 1, Green = 2 };

enum Direction { Up, Down, Left, Right };

typedef struct {
	 bool isPlayer;
	 bool isProcessed;
	 enum BugType type;
	 uint8_t cx;
	 uint8_t cy;
} Bug;

// button handlers
void blue_click_handler(ClickRecognizerRef recognizer, void *context);
void red_click_handler(ClickRecognizerRef recognizer, void *context);
void green_click_handler(ClickRecognizerRef recognizer, void *context);

// start, tick and stop
void warm_up(Window *window);
void game_tick();
void clean_up();