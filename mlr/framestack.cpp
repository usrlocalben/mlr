
#include "stdafx.h"

#include <vector>

#include "aligned_allocator.h"

vectorsse<unsigned char> store;
int storepos;

void fs_init()
{
	store.resize(1024 * 1024);
}
void fs_reset()
{
	storepos = 0;
}

void * fs_alloc(const size_t t)
{
	auto nextpos = storepos;

	auto over = t & 0xf;
	auto addtl = over ? 16 - over : 0;
	auto amt = t + addtl;

	storepos += amt;
	return &store[nextpos];
}