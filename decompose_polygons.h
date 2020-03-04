#pragma once

#include "data_polygons.h"

#include <list>

namespace polygons
{

std::list<Poly>
decompose(const Poly& vertices, const double margin);

Poly
ensure_cw(const Poly& vertices);

}

