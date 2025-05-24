// shark_actor.hpp
#pragma once
#include "skinned_actor.hpp"
#include "cgp/cgp.hpp"

/// A specialized skinned_actor with autonomous swimming behavior
struct npc_actor : public skinned_actor {
    float        speed             = 1.0f;      ///< units per second
    cgp::vec3    origin            {0,0,0};     ///< current position
    cgp::vec3    target            {0,0,0};     ///< destination point

    /**
     * Initializes position and target for the shark
     */

    virtual void start_position(skinned_actor const& target_actor) = 0;

    /**
     * Swim movement + directional alignment.
     */
    void update_position(float dt);

    /**
     * Checks for collision between the shark and another skinned_actor
     */

     virtual bool check_for_collision(skinned_actor const&  actor) = 0;

     bool check_for_end_of_life();

private:
    /// Align the mesh forward (-Y) to given direction
    virtual void align_to(cgp::vec3 const& dir) = 0;
};     

