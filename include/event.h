#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>

//quake 3

#define	MAX_QUEUED_EVENTS		512
#define	MASK_QUEUED_EVENTS	( MAX_QUEUED_EVENTS - 1 )

typedef enum {
  // bk001129 - make sure SE_NONE is zero
	SE_NONE = 0,	// evTime is still valid
	SE_KEY,		// evValue is a key code, evValue2 is the down flag
	SE_CHAR,	// evValue is an ascii char
	SE_MOUSE,	// evValue and evValue2 are reletive signed x / y moves
	SE_CONSOLE,	// evPtr is a char*
	SE_PACKET	// evPtr is a netadr_t followed by data bytes to evPtrLength
} sysEventType_t;

typedef struct {
	int				time;
	sysEventType_t	eventType;
	int				value; 
    int             value2;
	int				ptrLength;	// bytes of data pointed to by evPtr, for journaling
	void			*ptr;			// this must be manually freed if not NULL
} sysEvent_t;


typedef struct {

    sysEvent_t	queue[MAX_QUEUED_EVENTS];
    int	head;
    int tail;


} sysEventBus;


sysEventBus sys_createBus(void);

void sys_queueEvent(sysEventBus* bus, int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );

sysEvent_t sys_popEvent(sysEventBus* bus);



#endif EVENT_H