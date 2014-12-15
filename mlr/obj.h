
#ifndef __OBJ_H
#define __OBJ_H

#include "stdafx.h"

#include <tuple>
#include <string>

std::tuple<struct Mesh, class MaterialStore> loadObj(const std::string& prepend, const std::string& fn);


#endif //__OBJ_H