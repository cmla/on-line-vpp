// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright 2012, Enric Meinhardt Llopis <enric.meinhardt@cmla.ens-cachan.fr>
// All rights reserved.

#ifndef _XMALLOC_H
#define _XMALLOC_H

#include <stdlib.h>

// this function is like "malloc", but it returns always a valid pointer
void *xmalloc(size_t size);
#endif//_XMALLOC_H
