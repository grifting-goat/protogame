#include "event.h"


Event_bus createBus(void) {
	Event_bus bus;
	memset(&bus, 0, sizeof(bus));
	return bus;
}


void queueEvent(Event_bus* bus, int time, eventType type, int value, int value2, int ptrLength, void *ptr ) {
	Event	*event;
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

Event popEvent(Event_bus* bus) {
	if ( bus->head > bus->tail ) {
		bus->tail++;
		return bus->queue[ (bus->tail - 1) & MASK_QUEUED_EVENTS];
	}

	Event ev;
	memset( &ev, 0, sizeof( ev ) );
	ev.time = 0;
	return ev;
}