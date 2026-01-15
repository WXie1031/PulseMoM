#include <stdlib.h>
#include <string.h>

#include "core_geometry.h"

geom_geometry_t* geom_geometry_create(void) {
    geom_geometry_t* g = (geom_geometry_t*)calloc(1, sizeof(geom_geometry_t));
    return g;
}

void geom_geometry_destroy(geom_geometry_t* geom) {
    if (!geom) return;
    free(geom->entities);
    free(geom->topology);
    free(geom->domain_offsets);
    free(geom);
}

int geom_geometry_add_entity(geom_geometry_t* geom, const geom_entity_t* entity) {
    if (!geom || !entity) return -1;
    int new_count = geom->num_entities + 1;
    geom_entity_t* arr = (geom_entity_t*)realloc(geom->entities, new_count * sizeof(geom_entity_t));
    if (!arr) return -1;
    geom->entities = arr;
    geom->entities[geom->num_entities] = *entity;
    geom->num_entities = new_count;
    return geom->num_entities - 1;
}

int geom_geometry_get_entity(geom_geometry_t* geom, int entity_id, geom_entity_t** out) {
    if (!geom || !out) return -1;
    if (entity_id < 0 || entity_id >= geom->num_entities) {
        *out = NULL;
        return -1;
    }
    *out = &geom->entities[entity_id];
    return 0;
}
