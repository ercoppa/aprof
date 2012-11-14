/*
 * Load, store and modify handlers
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */
 
/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2012, Emilio Coppa (ercoppa@gmail.com),
                            Camil Demetrescu,
                            Irene Finocchi,
                            Romolo Marotta

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include "aprof.h"
#include "pub_tool_vkiscnums.h"


/* Memory resolution: we can aggregate addresses in order
 * to decrese the shadow memory. 
 * - 1 => finest resolution, each byte has its timestamp
 * - 2 => every 2 bytes we have a single timestamp 
 * - ...
 */
UInt APROF_(addr_multiple) = 4;

/* 
 * Global shadow memory used for checking latest "version" of an input.
 * If a routine writes a memory cell, we update the associated timestamp
 * in the global shadow memory with the current value of the global counter 
 */
LookupTable * APROF_(global_shadow_memory) = NULL;

/*
 * This global counter is increased by one when there is:
 * - a new routine activation
 * - a write of a memory cell
 */
UInt APROF_(global_counter) = 0;

VG_REGPARM(3) void APROF_(trace_access)(UWord type, Addr addr, SizeT size) {
	
	ThreadData * tdata = APROF_(current_tdata);
	if(tdata == NULL) return;
	#if DEBUG
	AP_ASSERT(tdata != NULL, "Invalid tdata");
	#endif
	
	#if VERBOSE > 1
	if (type == LOAD) VG_(printf)("Load: %lu:%lu\n", addr, size);
	else if (type == STORE) VG_(printf)("Store: %lu:%lu\n", addr, size);
	else if (type == MODIFY) VG_(printf)("Modify: %lu:%lu\n", addr, size);
	#endif
	
	#if EVENTCOUNT == 1 || EVENTCOUNT == 3
	if (type == LOAD) tdata->num_read++;
	else if (type == STORE) tdata->num_write++;
	else if (type == MODIFY) tdata->num_modify++;
	else AP_ASSERT(0, "Invalid type")
	#endif
	
	#if EMPTY_ANALYSIS
	return;
	#endif
    
	if (tdata->stack_depth == 0) return;
    
	#if TRACE_FUNCTION
	if (tdata->skip) return;
	#endif
	
	#if COSTANT_MEM_ACCESS
	addr = addr & ~(APROF_(addr_multiple)-1);
	//size = 1;
	#else
	
	if (APROF_(addr_multiple) > 1) {
		
		UInt diff = addr & (APROF_(addr_multiple)-1);
		addr -= diff;
		if (size + diff < APROF_(addr_multiple)) 
			size = 1;
		else if (((size + diff) % APROF_(addr_multiple)) == 0)
			size = (size + diff) / APROF_(addr_multiple);
		else
			size = 1 + ((size + diff) / APROF_(addr_multiple));
	
	}
	
	#endif
     
	#if !COSTANT_MEM_ACCESS
	unsigned int i = 0;
	for (i = 0; i < size; i++) {
	#endif

		Activation * act = APROF_(get_activation)(tdata, tdata->stack_depth);
        
		UInt ts;
		UInt old_ts;
		UInt wts;
		
		/*
		 * We are writing a new input value. So, this is a new "version"
		 * of the memory cell. Update its timestamp in the global shadow memory.
		 */
		 if (type == STORE || type == MODIFY) {
			
			ts = APROF_(global_counter);
			
			wts = LK_insert(APROF_(global_shadow_memory), 
							#if !COSTANT_MEM_ACCESS
							addr+(i*APROF_(addr_multiple)),
							#else
							addr,
							#endif
							ts);

		} else { 
			
			/*
			 * This is a load. What is the latest "version" of this
			 * memory cell? Get timestamp of the memory cell in
			 * the global shadow memory.
			 */
			
			ts = APROF_(global_counter);
			wts = LK_lookup(APROF_(global_shadow_memory),
							#if !COSTANT_MEM_ACCESS
							addr+(i*APROF_(addr_multiple)));
							#else
							addr);
							#endif

		}

		/*
		 * Update the timestamp in the private shadow memory.
		 */
		old_ts = LK_insert(tdata->accesses,
							#if !COSTANT_MEM_ACCESS
							addr+(i*APROF_(addr_multiple)),
							#else
							addr,
							#endif
							ts);
		

		if(type == STORE) {
			#if COSTANT_MEM_ACCESS
			return;
			#else
			continue;
			#endif
		}

		if(old_ts < wts){
			act->rms++;
		}
        
		else if (old_ts < act->aid) {
			
			act->rms++;
			//VG_(printf)("Incremented RMS\n");
			if (old_ts > 0 && old_ts >= APROF_(get_activation)(tdata, 1)->aid) {
				
				APROF_(get_activation_by_aid)(tdata, old_ts)->rms--;
				//VG_(printf)("Decremented SMS of ancestor %s\n", 
				//	APROF_(get_activation_by_aid)(tdata, old_ts)->rtn_info->fn->name);
			
			}

		}
		
	#if !COSTANT_MEM_ACCESS
	}
	#endif
	
  
}
#if SYSCALL_WRAPPING == 1

void APROF_(sys_trace_access)(Addr addr, SizeT size) {
	
	#if COSTANT_MEM_ACCESS
	addr = addr & ~(APROF_(addr_multiple)-1);
	//size = 1;
	#else
	
	if (APROF_(addr_multiple) > 1) {
		
		UInt diff = addr & (APROF_(addr_multiple)-1);
		addr -= diff;
		if (size + diff < APROF_(addr_multiple)) 
			size = 1;
		else if (((size + diff) % APROF_(addr_multiple)) == 0)
			size = (size + diff) / APROF_(addr_multiple);
		else
			size = 1 + ((size + diff) / APROF_(addr_multiple));
	
	}
	
	#endif
     
	#if !COSTANT_MEM_ACCESS
	unsigned int i = 0;
	for (i = 0; i < size; i++) {
	#endif

		UInt ts = APROF_(global_counter);
			
		LK_insert(APROF_(global_shadow_memory), 
						#if !COSTANT_MEM_ACCESS
						addr+(i*APROF_(addr_multiple)),
						#else
						addr,
						#endif
						ts);
	
		#if COSTANT_MEM_ACCESS
		return;
		#else
		continue;
		#endif

	#if !COSTANT_MEM_ACCESS
	}
	#endif
	
  
}

void APROF_(pre_syscall)(ThreadId tid, UInt syscallno, UWord* args, UInt nArgs){}
	
void APROF_(post_syscall)(ThreadId tid, UInt syscallno, UWord* args, UInt nArgs, SysRes res){

	#if defined(VGO_linux)
	if(res._isError) return;
	#elif defined(VGO_darwin)
	if(res._mode==SysRes_UNIX_ERR)return;
	#endif

	SizeT size = (SizeT)sr_Res(res);
	
	if(	
		syscallno == __NR_read || 

		#if defined(VGP_ppc32_linux) || defined(VGP_ppc64_linux)
		syscallno == __NR_recv ||
		#endif

		#if !defined(VGP_x86_linux) && !defined(VGP_s390x_linux)
		syscallno == __NR_recvfrom ||
		#endif

		#if defined(VGP_x86_darwin) || defined(VGP_amd64_darwin)
		syscallno == __NR_pread
		#else
		syscallno == __NR_pread64  
		#endif

		){

			APROF_(global_counter)++;
			if(APROF_(global_counter) == 0)
				APROF_(global_counter) = APROF_(overflow_handler)();
			Addr addr = args[1];
			APROF_(sys_trace_access)(addr, size);

	} else if (
			syscallno == __NR_readv
			#if !defined(VGP_x86_darwin) && !defined(VGP_amd64_darwin)
			|| syscallno == __NR_preadv
			#endif
			){
			
			APROF_(global_counter)++;
			if(APROF_(global_counter) == 0)
				APROF_(global_counter) = APROF_(overflow_handler)();
			struct iovec* base = (struct iovec*)args[1];
			UWord iovcnt = args[2];
			UWord i;
			SizeT iov_len;
			for(i = 0;i < iovcnt; i++){
				
				if(size == 0) break;

				Addr tadd = (Addr) base[i].iov_base;
				if(base[i].iov_len <= size)					
					iov_len = base[i].iov_len;
				else
					iov_len = size;
										
				size -= iov_len; 					
				
				APROF_(sys_trace_access)(tadd, iov_len);
				
			}

	} else if (
				syscallno == __NR_write ||

				#if defined(VGP_ppc32_linux) || defined(VGP_ppc64_linux)
				syscallno== __NR_send ||
				#endif

				#if !defined(VGP_x86_linux) && !defined(VGP_s390x_linux)
				syscallno== __NR_sendto ||
				#endif

				#if defined(VGP_x86_darwin) || defined(VGP_amd64_darwin)
				syscallno== __NR_pwrite
				#else
				syscallno== __NR_pwrite64  
				#endif

				){
		
		Addr addr = args[1];
		APROF_(trace_access)(LOAD, addr, size);

	} else if (
				syscallno == __NR_writev
				#if !defined(VGP_x86_darwin) && !defined(VGP_amd64_darwin)
				|| syscallno == __NR_pwritev
				#endif
				){
				
		struct iovec* base = (struct iovec*)args[1];
		UWord iovcnt = args[2];
		UWord i;
		SizeT iov_len;
		for(i = 0; i < iovcnt; i++){
			
			if(size == 0)break;

			Addr tadd = base[i].iov_base;
			if(base[i].iov_len <= size)					
				iov_len = base[i].iov_len;
			else
				iov_len = size;
									
			size -= iov_len; 					
			
			APROF_(trace_access)(LOAD, tadd, iov_len);
			
		}
	
	}else if(
				syscallno== __NR_msgrcv
				#if !defined(VGP_x86_darwin) && !defined(VGP_amd64_darwin)
				//|| syscallno== __NR_pwritev
				#endif
				){
					
		APROF_(global_counter)++;
		if(APROF_(global_counter) == 0)
			APROF_(global_counter) =APROF_(overflow_handler)();
		
		Addr addr = args[1];
		APROF_(sys_trace_access)( addr, size + sizeof(long int));

	} else if (
	
				syscallno == __NR_msgsnd
				#if !defined(VGP_x86_darwin) && !defined(VGP_amd64_darwin)
				//|| syscallno== __NR_pwritev
				#endif
				){
								
		Addr addr = args[1];
		SizeT s = args[2];
		APROF_(trace_access)(LOAD, addr, s + sizeof(long int));

	}
}

#endif









