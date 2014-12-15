
#ifndef __ROCKET_H
#define __ROCKET_H

#include "stdafx.h"

#include <map>

#include "../rocket/sync.h"

class Rocket {
private:
	sync_device * const rocket;
	std::map<std::string, const struct sync_track*> tracks;
	double currow;

public:
	Rocket(const std::string dataprefix);
	~Rocket();

	const struct sync_track *getTrack(const std::string trackname);
	const double get(const std::string trackname);
	inline const float getf(const std::string trackname) {
		return static_cast<float>(this->get(trackname));
	}

	void update(const double row, struct sync_cb* cb, void* cb_param);
};

#endif //__ROCKET_H
