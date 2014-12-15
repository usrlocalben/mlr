
#include "stdafx.h"

#include <iostream>

#include "player.h"

using std::cout;
using std::string;
using std::endl;


Player::Player(string filename, double bpm, int rows_per_beat)
	:filename(filename)
	,bpm(bpm)
	,rows_per_beat(rows_per_beat)
	,rows_per_second(bpm / 60.0*rows_per_beat)
{
	cout << "yo!" << endl;
	BASS_Init(-1, 44100, 0, 0, NULL);
	track = BASS_StreamCreateFile(FALSE, filename.c_str(), 0, 0, BASS_STREAM_PRESCAN);
	if (!track) {
		cout << "failed to open soundtrack." << endl;
		while (1);
	}
}

Player::~Player() {
	BASS_StreamFree(track);
	BASS_Free();
	cout << "cya!" << endl;
}

void Player::play() {
	BASS_ChannelPlay(track, false);
}

double Player::getSecond() const {
	return BASS_ChannelBytes2Seconds(
		track,
		BASS_ChannelGetPosition(track, BASS_POS_BYTE)
	);
}

double Player::getRow() const {
	return getSecond() * rows_per_second;
}

void Player::tick() const {
	BASS_Update(0);
}

void Player::setRow(int row) {
	BASS_ChannelSetPosition(
		track,
		BASS_ChannelSeconds2Bytes(track, row / rows_per_second),
		BASS_POS_BYTE
	);
}

int Player::isPlaying() {
	return BASS_ChannelIsActive(track) == BASS_ACTIVE_PLAYING;
}

void Player::pause(int flag) {
	if (flag) {
		BASS_ChannelPause(track);
	} else {
		BASS_ChannelPlay(track, false);
	}
}

int Player::callback_isPlaying(void*ptr) {
	Player *p = static_cast<Player*>(ptr);
	return p->isPlaying();
}

void Player::callback_pause(void* ptr, int flag) {
	Player *p = static_cast<Player*>(ptr);
	p->pause(flag);
}

void Player::callback_setRow(void* ptr, int row) {
	Player *p = static_cast<Player*>(ptr);
	p->setRow(row);
}
