
#ifndef __FRAMESTACK_H
#define __FRAMESTACK_H

#include "stdafx.h"

void fs_init();
void fs_reset();
void * fs_alloc(const size_t t);

#endif //__FRAMESTACK_H