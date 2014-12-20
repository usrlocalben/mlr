
#ifndef __JSONFILE_H
#define __JSONFILE_H

#include "stdafx.h"

#include <memory>
#include <string>
#include <vector>

#include "gason.h"
#include "utils.h"


class JsonWalker {
public:
	JsonWalker(const JsonValue * const jv) : jv(jv) {}

	JsonWalker get(const std::string& key) {
		for (const auto& item : *jv) {
			if (key == item->key) {
				return JsonWalker(&item->value);
			}
		}
		//		return nullptr;
	}
	char* toString() const { return jv->toString(); }
	double toNumber() const { return jv->toNumber(); }
	int toInt() const { return static_cast<int>(jv->toNumber()); }
	bool toBool() const { return jv->getTag() == JSON_TRUE; }

private:
	const JsonValue * const jv;
};

class JsonFile {
public:
	JsonFile(const std::string& fn) {
		serial = 0;
		filename = fn;
		last_mtime = getmtime();
		reload();
	}
	void maybe_reload() {
		long long mtime_now = getmtime();
		if (mtime_now != last_mtime) {
			last_mtime = mtime_now;
			reload();
		}
	}

	JsonWalker root() {
		return JsonWalker(jsonroot.get());
	}

private:
	void reload();

	long long getmtime() const {
		return ::getmtime(filename);
	}

private:
	int serial;
	long long last_mtime;
	std::string filename;
	std::vector<char> rawdata;
	std::unique_ptr<JsonValue> jsonroot;
	std::unique_ptr<JsonAllocator> allocator;
};

#endif //__JSONFILE_H
