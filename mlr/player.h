
#ifndef __PLAYER_H
#define __PLAYER_H

#include <string>

#include "bass.h"


class Player {

	const std::string filename;
	const double bpm;
	const int rows_per_beat;
	const double rows_per_second;

	HSTREAM track;

public:
	Player(std::string filename, double bpm, int rows_per_beat);
	~Player();
	void play();
	double getSecond() const;
	double getRow() const;
	void setRow(int row);
	int isPlaying();
	void pause(int flag);
	void tick() const;

	static int callback_isPlaying(void* ptr);
	static void callback_pause(void* ptr, int flag);
	static void callback_setRow(void* ptr, int row);

};

#endif //__PLAYER_H