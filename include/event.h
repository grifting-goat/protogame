#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>

//quake 3

#define	MAX_QUEUED_EVENTS		1024
#define	MASK_QUEUED_EVENTS	( MAX_QUEUED_EVENTS - 1 )

typedef enum {
  // bk001129 - make sure SE_NONE is zero
	SE_NONE = 0,	// evTime is still valid
	SE_ACTION,
	SE_KEY,		// evValue is a key code, evValue2 is the down flag
	SE_CHAR,	// evValue is an ascii char
	SE_MOUSE,	// evValue and evValue2 are reletive signed x / y moves
	SE_CONSOLE,	// evPtr is a char*
	SE_PACKET	// evPtr is a netadr_t followed by data bytes to evPtrLength
} eventType;


typedef enum {
    DASH = 0,
    SHOOT,
    JUMP,

} actionType; //move to events

typedef struct {
	int				time;
	eventType	eventType;
	int				value; 
    int             value2;
	int				ptrLength;	// bytes of data pointed to by evPtr
	void			*ptr;			// this must be manually freed if not NULL
} Event;


typedef struct {

    Event queue[MAX_QUEUED_EVENTS];
    int	head;
    int tail;


} Event_bus;


Event_bus createBus(void);

void queueEvent(Event_bus* bus, int time, eventType type, int value, int value2, int ptrLength, void *ptr );

Event popEvent(Event_bus* bus);



#endif EVENT_H