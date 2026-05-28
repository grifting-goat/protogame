#ifndef STATES_H
#define STATES_H

#include "entity.h"

//this is stupid and needs fixed

#define GROUNDED 1
#define IN_AIR (1 << 1)
#define FLYING (1 << 2)
#define SWIMMING (1 << 3)
#define RUNNING (1 << 4)
#define SLIDING (1 << 5)
#define GLIDING (1 << 6)
#define DEAD (1 << 7)

#define THIRDPERSON (1 << 8)


static inline bool is_state(Entity* ent, uint32_t flag)  { return ent->states & flag; }

static inline void set_state(Entity* ent, uint32_t flag)   { ent->states |= flag; }
static inline void clear_state(Entity* ent, uint32_t flag) { ent->states &= ~flag; }
static inline void toggle_state(Entity* ent, uint32_t flag){ ent->states ^= flag; }



#endif //STATES_H