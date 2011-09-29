/*
 * Event handlers and instrumentation
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

/* Up to this many unnotified events are allowed.  Must be at least two,
   so that reads and writes to the same address can be merged into a modify.
   Beyond that, larger numbers just potentially induce more spilling due to
   extending live ranges of address temporaries. */
#define N_EVENTS 4

/* Maintain an ordered list of memory events which are outstanding, in
   the sense that no IR has yet been generated to do the relevant
   helper calls.  The SB is scanned top to bottom and memory events
   are added to the end of the list, merging with the most recent
   notified event where possible (Dw immediately following Dr and
   having the same size and EA can be merged).

   This merging is done so that for architectures which have
   load-op-store instructions (x86, amd64), the instr is treated as if
   it makes just one memory reference (a modify), rather than two (a
   read followed by a write at the same address).

   At various points the list will need to be flushed, that is, IR
   generated from it.  That must happen before any possible exit from
   the block (the end, or an IRStmt_Exit).  Flushing also takes place
   when there is no space to add a new event.

   If we require the simulation statistics to be up to date with
   respect to possible memory exceptions, then the list would have to
   be flushed before each memory reference.  That's a pain so we don't
   bother.

   Flushing the list consists of walking it start to end and emitting
   instrumentation IR for each event, in the order in which they
   appear. 
*/

#define MAX_DSIZE    512
typedef enum { Event_Ir, Event_Dr, Event_Dw, Event_Dm } EventKind;

typedef struct {
	EventKind  ekind;
	IRAtom*    addr;
	Int        size;
} Event;

static Event events[N_EVENTS];
static Int   events_used = 0;

void flushEvents(IRSB* sb) {
	
	Int        i;
	Char*      helperName = NULL;
	void*      helperAddr = NULL;
	IRExpr**   argv = NULL;
	IRDirty*   di;
	Event*     ev;

	for (i = 0; i < events_used; i++) {

		ev = &events[i];

		// Decide on helper fn to call and args to pass it.
		switch (ev->ekind) {
		
			case Event_Ir: break;
			
			case Event_Dr:	helperName = "trace_load";
							argv = mkIRExprVec_3(	mkIRExpr_HWord(LOAD), 
													ev->addr, 
													mkIRExpr_HWord( ev->size ) );
							helperAddr = trace_access;
							break;

			case Event_Dw:  helperName = "trace_store";
							argv = mkIRExprVec_3(	mkIRExpr_HWord(STORE), 
													ev->addr, 
													mkIRExpr_HWord( ev->size ) );
							helperAddr =  trace_access; break;

			case Event_Dm:  helperName = "trace_modify";
							argv = mkIRExprVec_3(	mkIRExpr_HWord(MODIFY), 
													ev->addr, 
													mkIRExpr_HWord( ev->size ) );
							helperAddr =  trace_access; break;
			
			default:
				tl_assert(0);
		
		}

		// Add the helper.
		if (ev->ekind != Event_Ir) {
			
			di = unsafeIRDirty_0_N( 3, helperName, 
									VG_(fnptr_to_fnentry)( helperAddr ),
									argv );
			
			addStmtToIRSB( sb, IRStmt_Dirty(di) );
		}
	}

	events_used = 0;
}

// WARNING:  If you aren't interested in instruction reads, you can omit the
// code that adds calls to trace_instr() in flushEvents().  However, you
// must still call this function, addEvent_Ir() -- it is necessary to add
// the Ir events to the events list so that merging of paired load/store
// events into modify events works correctly.
void addEvent_Ir ( IRSB* sb, IRAtom* iaddr, UInt isize ) {
	
	Event* evt;
	tl_assert( (VG_MIN_INSTR_SZB <= isize && isize <= VG_MAX_INSTR_SZB)
				|| VG_CLREQ_SZB == isize );
	if (events_used == N_EVENTS)
		flushEvents(sb);
	tl_assert(events_used >= 0 && events_used < N_EVENTS);
	evt = &events[events_used];
	evt->ekind = Event_Ir;
	evt->addr  = iaddr;
	evt->size  = isize;
	events_used++;
}

void addEvent_Dr ( IRSB* sb, IRAtom* daddr, Int dsize ) {
	
	Event* evt;
	tl_assert(isIRAtom(daddr));
	tl_assert(dsize >= 1 && dsize <= MAX_DSIZE);
	if (events_used == N_EVENTS)
		flushEvents(sb);
	tl_assert(events_used >= 0 && events_used < N_EVENTS);
	evt = &events[events_used];
	evt->ekind = Event_Dr;
	evt->addr  = daddr;
	evt->size  = dsize;
	events_used++;
}

void addEvent_Dw ( IRSB* sb, IRAtom* daddr, Int dsize ) {
	
	Event* lastEvt;
	Event* evt;
	tl_assert(isIRAtom(daddr));
	tl_assert(dsize >= 1 && dsize <= MAX_DSIZE);

	// Is it possible to merge this write with the preceding read?
	lastEvt = &events[events_used-1];
	if (events_used > 0
		&& lastEvt->ekind == Event_Dr
		&& lastEvt->size  == dsize
		&& eqIRAtom(lastEvt->addr, daddr)) {
		lastEvt->ekind = Event_Dm;
		return;
	}

	// No.  Add as normal.
	if (events_used == N_EVENTS)
		flushEvents(sb);
	tl_assert(events_used >= 0 && events_used < N_EVENTS);
	evt = &events[events_used];
	evt->ekind = Event_Dw;
	evt->size  = dsize;
	evt->addr  = daddr;
	events_used++;
}
