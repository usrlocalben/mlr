
#include "stdafx.h"
#include <iostream>
#include <memory>

#include <conio.h>

#include "utils.h"
#include "jsonfile.h"

using namespace std;

void JsonFile::reload()
{
	jsonroot = make_unique<JsonValue>();
	allocator = make_unique<JsonAllocator>();
	rawdata.clear();

	file_get_contents(filename, rawdata);
	rawdata.push_back(0);
	char *source = &rawdata[0];
	char *endptr;

	int status = jsonParse(source, &endptr, jsonroot.get(), *allocator.get());
	if (status != JSON_OK) {
		cerr << jsonStrError(status) << " at " << endptr - source << endl;
		_getch();
	}
	serial++;
}
