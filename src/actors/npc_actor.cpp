#include "npc_actor.hpp"
#include "cgp/cgp.hpp"

bool npc_actor::check_for_end_of_life(){
    float tol = 0.5;
    return cgp::norm(drawable.model.translation - target) <= tol;
}

/**
 * Swim movement + directional alignment.
 */
void npc_actor::update_position(float dt)
{
    cgp::vec3 dir = target - origin;
    float dist = cgp::norm(dir);
    if (dist < 1e-4f) return;

    dir /= dist;            // normalize
    origin += dir * speed * dt;
    drawable.model.translation = origin;
    align_to(dir);
}