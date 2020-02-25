#pragma once

#include "data_polygons.h"

namespace polygons
{

std::tuple<PolyToColors, PolyToColors> extract(const std::string& filename);

}

