
#include "stdafx.h"

#include <iostream>

#include "rocket.h"
#include "player.h"


using namespace std;


Rocket::Rocket(const string dataprefix)
	:rocket(sync_create_device(dataprefix.c_str()))
{
#ifndef SYNC_PLAYER
	if (sync_connect(rocket, "localhost", SYNC_DEFAULT_PORT)) {
		cout << "could not connect to editor." << endl;
		while (1);
	}
#endif
}


Rocket::~Rocket()
{
	cout << "rocket is shutting down." << endl;
#ifndef SYNC_PLAYER
	sync_save_tracks(rocket);
#endif
	sync_destroy_device(rocket);
}


const struct sync_track *Rocket::getTrack(const string trackname) {
	return sync_get_track(rocket, trackname.c_str());
}


const double Rocket::get(string trackname) {
	auto track = tracks.find(trackname);
	if (track == tracks.end()) {
		auto track = tracks[trackname] = getTrack(trackname);
		return sync_get_val(track, currow);
	} else {
		return sync_get_val(track->second, currow);
	}
}

void Rocket::update(const double row, struct sync_cb* cb, void* cb_param) {
	currow = row;
#ifndef SYNC_PLAYER
	if (sync_update(rocket, (int)floor(row), cb, cb_param)) {
		sync_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
	}
#endif
}


