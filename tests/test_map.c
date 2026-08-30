#include <assert.h>

#include "map.h"

int main(void)
{
    Map map;
    map_init(&map);
    assert(map_get_cost(&map, 1) == 200.0);
    assert(map_get_cost(&map, 29) == 500.0);
    assert(map_get_cost(&map, 36) == 300.0);
    assert(map_get_property_owner(&map, 1) == MAP_PROPERTY_UNOWNED);
    assert(map_set_property(&map, 1, 1, 0));
    assert(map_get_property_owner(&map, 1) == 1);
    assert(map_get_property_level(&map, 1) == 0);
    assert(map_set_property(&map, 1, 1, MAP_MAX_PROPERTY_LEVEL));
    assert(!map_set_property(&map, 1, 1, MAP_MAX_PROPERTY_LEVEL + 1));
    assert(!map_set_property(&map, 0, 1, 1));
    assert(map_clear_property(&map, 1));
    assert(map_get_property_owner(&map, 1) == MAP_PROPERTY_UNOWNED);
    return 0;
}
