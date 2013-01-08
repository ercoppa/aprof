/*
 * syscall handlers
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2013, Emilio Coppa (ercoppa@gmail.com),
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

inline void APROF_(fix_access_size)(Addr * addr, SizeT * size) {

    if (APROF_(addr_multiple) > 1) {
        
        UInt diff = (*addr) & (APROF_(addr_multiple)-1);
        (*addr) -= diff;
        if ((*size) + diff < APROF_(addr_multiple)) 
            (*size) = 1;
        else if ((((*size) + diff) % APROF_(addr_multiple)) == 0)
            (*size) = ((*size) + diff) / APROF_(addr_multiple);
        else
            (*size) = 1 + (((*size) + diff) / APROF_(addr_multiple));
    
    }
    
}

#if SYSCALL_WRAPPING == 1 && INPUT_METRIC == RVMS
  
void APROF_(pre_syscall)(ThreadId tid, UInt syscallno, 
                            UWord * args, UInt nArgs) {}
  
void APROF_(post_syscall)(ThreadId tid, UInt syscallno, 
                            UWord * args, UInt nArgs, SysRes res) {

    #if defined(VGO_linux)
    if(res._isError) return;
    #elif defined(VGO_darwin)
    if(res._mode == SysRes_UNIX_ERR) return;
    #endif

    SizeT size = (SizeT) sr_Res(res);
    
    if(    
        syscallno == __NR_read 

        #if defined(VGP_ppc32_linux) || defined(VGP_ppc64_linux)
        || syscallno == __NR_recv 
        #endif

        #if !defined(VGP_x86_linux) && !defined(VGP_s390x_linux)
        || syscallno == __NR_recvfrom
        #endif

        #if defined(VGP_x86_darwin) || defined(VGP_amd64_darwin)
        || syscallno == __NR_pread
        #else
        || syscallno == __NR_pread64  
        #endif

        ){

            APROF_(global_counter)++;
            if(APROF_(global_counter) == 0)
                APROF_(global_counter) = APROF_(overflow_handler)();
            
            Addr addr = args[1];
            APROF_(fix_access_size)(&addr, &size);
            
            UInt i = 0;
            for (i = 0; i < size; i++) {
                
                APROF_(trace_access)(   STORE, 
                                        addr+(i*APROF_(addr_multiple)), 
                                        APROF_(addr_multiple), True);
            }

    } else if (
    
            syscallno == __NR_readv
            #if !defined(VGP_x86_darwin) && !defined(VGP_amd64_darwin)
            || syscallno == __NR_preadv
            #endif
            
            ){
            
            APROF_(global_counter)++;
            if(APROF_(global_counter) == 0)
                APROF_(global_counter) = APROF_(overflow_handler)();
            
            struct vki_iovec * base = (struct  vki_iovec *) args[1];
            UWord iovcnt = args[2];
            UWord i;
            SizeT iov_len;
            for(i = 0; i < iovcnt; i++){
                
                if(size == 0) break;

                Addr addr = (Addr) base[i].iov_base;
                if(base[i].iov_len <= size)                    
                    iov_len = base[i].iov_len;
                else
                    iov_len = size;
                                        
                size -= iov_len;                     
                
                APROF_(fix_access_size)(&addr, &iov_len);
            
                UInt k = 0;
                for (k = 0; k < iov_len; k++) {
                    
                    APROF_(trace_access)(   STORE, 
                                            addr+(k*APROF_(addr_multiple)), 
                                            APROF_(addr_multiple), True);
                }
                
            }

    } else if (
                syscallno == __NR_write

                #if defined(VGP_ppc32_linux) || defined(VGP_ppc64_linux)
                || syscallno== __NR_send
                #endif

                #if !defined(VGP_x86_linux) && !defined(VGP_s390x_linux)
                || syscallno== __NR_sendto
                #endif

                #if defined(VGP_x86_darwin) || defined(VGP_amd64_darwin)
                || syscallno== __NR_pwrite
                #else
                || syscallno== __NR_pwrite64  
                #endif

                ){
        
        Addr addr = args[1];

        APROF_(fix_access_size)(&addr, &size);
            
        UInt i = 0;
        for (i = 0; i < size; i++) {
            
            APROF_(trace_access)(   LOAD, 
                                    addr+(i*APROF_(addr_multiple)), 
                                    APROF_(addr_multiple), False);
        }

    } else if (
                syscallno == __NR_writev
                #if !defined(VGP_x86_darwin) && !defined(VGP_amd64_darwin)
                || syscallno == __NR_pwritev
                #endif
                ){
                
        struct vki_iovec * base = (struct vki_iovec *) args[1];
        UWord iovcnt = args[2];
        UWord i;
        SizeT iov_len;
        for(i = 0; i < iovcnt; i++){
            
            if(size == 0) break;

            Addr addr = (Addr) base[i].iov_base;
            if(base[i].iov_len <= size)                    
                iov_len = base[i].iov_len;
            else
                iov_len = size;
                                    
            size -= iov_len;                     
            
            APROF_(fix_access_size)(&addr, &iov_len);
            
            UInt k = 0;
            for (k = 0; k < iov_len; k++) {
                
                APROF_(trace_access)(   LOAD, 
                                        addr+(k*APROF_(addr_multiple)), 
                                        APROF_(addr_multiple), False);
            }
            
        }
    
    } else if (
            
            #if !defined(VGP_x86_linux)
            syscallno == __NR_msgrcv
            #else
            False
            #endif
            
            #if !defined(VGP_x86_darwin) && !defined(VGP_amd64_darwin)
            //|| syscallno== __NR_pwritev
            #endif
            
            ){
                    
        APROF_(global_counter)++;
        if(APROF_(global_counter) == 0)
            APROF_(global_counter) = APROF_(overflow_handler)();
        
        Addr addr = args[1];
        size = size + sizeof(long int);
        APROF_(fix_access_size)(&addr, &size);
            
        UInt i = 0;
        for (i = 0; i < size; i++) {
            
            APROF_(trace_access)(   LOAD, 
                                    addr+(i*APROF_(addr_multiple)), 
                                    APROF_(addr_multiple), True);
        }

    } else if (
                #if !defined(VGP_x86_linux)
                syscallno == __NR_msgsnd
                #else
                False
                #endif
                
                #if !defined(VGP_x86_darwin) && !defined(VGP_amd64_darwin)
                //|| syscallno== __NR_pwritev
                #endif
                
                ){
                                
        Addr addr = args[1];
        SizeT s = args[2];
        
        size = s + sizeof(long int);
        APROF_(fix_access_size)(&addr, &size);
            
        UInt i = 0;
        for (i = 0; i < size; i++) {
            
            APROF_(trace_access)(   LOAD, 
                                    addr+(i*APROF_(addr_multiple)), 
                                    APROF_(addr_multiple), False);
        }
    
    }
  
}

#endif
