#include "event.h"


sysEventBus sys_createBus(void) {
    return (sysEventBus){.head = 0, .tail = 0};
}


void sys_queueEvent(sysEventBus* bus, int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t	*event;
	event = &bus->queue[bus->head & MASK_QUEUED_EVENTS ];
	if ( bus->head - bus->tail >= MAX_QUEUED_EVENTS ) {
		printf("Sys_QueEvent: overflow\n");

		// we are discarding an event, but don't leak memory
		if ( event->ptr ) {
			free(event->ptr);
		}
		bus->tail++;
	}

	bus->head++;

	event->time = time;
	event->eventType = type;
	event->value = value;
	event->value2 = value2;
	event->ptrLength = ptrLength;
	event->ptr = ptr;
}

sysEvent_t sys_popEvent(sysEventBus* bus) {
    
    if ( bus->head > bus->tail ) {
		bus->tail++;
		return bus->queue[ (bus->tail - 1) & MASK_QUEUED_EVENTS];
	}

    sysEvent_t	ev;
    memset( &ev, 0, sizeof( ev ) );
	ev.time = 0;

    return ev;



}