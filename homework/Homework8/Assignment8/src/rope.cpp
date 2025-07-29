#include <iostream>
#include <vector>

#include "CGL/vector2D.h"

#include "mass.h"
#include "rope.h"
#include "spring.h"

namespace CGL {

    Rope::Rope(Vector2D start, Vector2D end, int num_nodes, float node_mass, float k, vector<int> pinned_nodes)
    {
        // TODO (Part 1): Create a rope starting at `start`, ending at `end`, and containing `num_nodes` nodes.
        for (int i = 0; i < num_nodes; i++) {
            Vector2D position = (start + i * (end - start) / (num_nodes - 1));
            Mass* mass = new Mass(position, node_mass, false);
            masses.push_back(mass);
        }
//        Comment-in this part when you implement the constructor
       for (auto &i : pinned_nodes) {
           masses[i]->pinned = true;
       }
       for (int i = 1; i < num_nodes; i++) {
            Spring* spring = new Spring(masses[i - 1], masses[i], k);
            springs.push_back(spring);
       }
    }

    void Rope::simulateEuler(float delta_t, Vector2D gravity)
    {
        for (auto &s : springs)
        {
            // TODO (Part 2): Use Hooke's law to calculate the force on a node
            Vector2D ab = s->m2->position - s->m1->position;
            Vector2D force = s->k * (ab.unit()) * (ab.norm() - s->rest_length);
            s->m1->forces += force;
            s->m2->forces -= force;
        }

        float k_d = 0.1;

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                // TODO (Part 2): Add the force due to gravity, then compute the new velocity and position
                m->forces += gravity * m->mass;
                // TODO (Part 2): Add global damping
                m->forces += -k_d * m->velocity;
                auto a = m->forces / m->mass;
                m->velocity += a * delta_t;
                m->position += m->velocity * delta_t;
            }

            // Reset all forces on each mass
            m->forces = Vector2D(0, 0);
        }
    }

    void Rope::simulateVerlet(float delta_t, Vector2D gravity)
    {
        for (auto &s : springs)
        {
            // TODO (Part 3): Simulate one timestep of the rope using explicit Verlet （solving constraints)
            Vector2D ab = s->m2->position - s->m1->position;
            Vector2D force = s->k * (ab.unit()) * (ab.norm() - s->rest_length);
            s->m1->forces += force;
            s->m2->forces -= force;
        }

        double damping_factor = 0.00005;

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                m->forces += gravity * m->mass;
                auto a = m->forces / m->mass;
                Vector2D temp_position = m->position;
                // TODO (Part 3.1): Set the new position of the rope mass
                m->position += ((1 - damping_factor) * (m->position - m->last_position) + a * delta_t * delta_t);
                m->last_position = temp_position;
                // TODO (Part 4): Add global Verlet damping
            }
            
            m->forces = Vector2D(0, 0);
        }
    }
}
