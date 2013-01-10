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

/*
   syscalls:
   
   -------------------------------------------------------- 
   | NAME     | SEEN AS   | ARCH                          |
   --------------------------------------------------------
    read        SYS_STORE   *
    recv        SYS_STORE   ppc32_linux, ppc64_linux
    recvfrom    SYS_STORE   * - {x86_linux, s390x}
    pread       SYS_STORE   x86_darwin, amd64_darwin
    pread64     SYS_STORE   * - {x86_darwin, amd64_darwin}
    readv       SYS_STORE   *
    preadv      SYS_STORE   * - {x86_darwin, amd64_darwin}
    write       LOAD        *
    send        LOAD        ppc32_linux, ppc64_linux 
    sendto      LOAD        * - {x86_linux, s390x}
    pwrite      LOAD        x86_darwin, amd64_darwin
    pwrite64    LOAD        * - {x86_darwin, amd64_darwin}
    writev      LOAD        *
    pwritev     LOAD        * - {x86_darwin, amd64_darwin}
    msgrcv      SYS_STORE   * - {x86_linux}
    msgsnd      LOAD        * - {x86_linux}
   ---------------------------------------------------------

    '*' := all architectures
    '* - {arch, ...}' := all archs with the exception of ...
    'SYS_STORE' := we treat this as a "kernel" STORE, we do not
                   reset private shadow memories but instead we only
                   increase the global counter and update
                   the global shadow memory
    'LOAD' := a normal load :)

 */

#include "aprof.h"

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

    Int size = (Int) sr_Res(res);
    
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
            
            Addr addr = args[1];
            APROF_(fix_access_size)(addr, size);
            
            APROF_(global_counter)++;
            if(APROF_(global_counter) == 0)
                APROF_(global_counter) = APROF_(overflow_handler)();
            
            while(size > 0) {
                
                APROF_(trace_access)(   STORE, 
                                        addr, 
                                        APROF_(addr_multiple), True);
                
                size -= APROF_(addr_multiple);
                addr += APROF_(addr_multiple);
            }

    } else if (
    
            syscallno == __NR_readv
            #if !defined(VGP_x86_darwin) && !defined(VGP_amd64_darwin)
            || syscallno == __NR_preadv
            #endif
            
            ){
            
            struct vki_iovec * base = (struct  vki_iovec *) args[1];
            UWord iovcnt = args[2];
            UWord i;
            Int iov_len;
            
            APROF_(global_counter)++;
            if(APROF_(global_counter) == 0)
                APROF_(global_counter) = APROF_(overflow_handler)();
            
            for(i = 0; i < iovcnt; i++){
                
                if(size == 0) break;

                Addr addr = (Addr) base[i].iov_base;
                if(base[i].iov_len <= size)                    
                    iov_len = base[i].iov_len;
                else
                    iov_len = size;
                                        
                size -= iov_len;                     
                
                APROF_(fix_access_size)(addr, iov_len);
            
                while(iov_len > 0) {
                    
                    APROF_(trace_access)(   STORE, 
                                            addr, 
                                            APROF_(addr_multiple), True);
                    
                    iov_len -= APROF_(addr_multiple);
                    addr += APROF_(addr_multiple);
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

        APROF_(fix_access_size)(addr, size);
        while (size > 0) {
            
            APROF_(trace_access)(   LOAD, 
                                    addr, 
                                    APROF_(addr_multiple), False);
                                    
            size -= APROF_(addr_multiple);
            addr += APROF_(addr_multiple);
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
        Int iov_len;
        for(i = 0; i < iovcnt; i++){
            
            if(size == 0) break;

            Addr addr = (Addr) base[i].iov_base;
            if(base[i].iov_len <= size)                    
                iov_len = base[i].iov_len;
            else
                iov_len = size;
                                    
            size -= iov_len;                     
            
            APROF_(fix_access_size)(addr, iov_len);
            
            while (iov_len > 0) {
                
                APROF_(trace_access)(   LOAD, 
                                        addr, 
                                        APROF_(addr_multiple), False);
                
                iov_len -= APROF_(addr_multiple);
                addr += APROF_(addr_multiple);
            }
            
        }
    
    } else if (
            
            #if !defined(VGP_x86_linux)
            syscallno == __NR_msgrcv
            #else
            False
            #endif
            ){
        
        Addr addr = args[1];
        size = size + sizeof(long int);
        APROF_(fix_access_size)(addr, size);
        
        APROF_(global_counter)++;
        if(APROF_(global_counter) == 0)
            APROF_(global_counter) = APROF_(overflow_handler)();
        
        while(size > 0) {
            
            APROF_(trace_access)(   STORE, 
                                    addr, 
                                    APROF_(addr_multiple), True);
            
            size -= APROF_(addr_multiple);
            addr += APROF_(addr_multiple);
        }

    } else if (
                #if !defined(VGP_x86_linux)
                syscallno == __NR_msgsnd
                #else
                False
                #endif
                ){
                                
        Addr addr = args[1];
        SizeT s = args[2];
        
        size = s + sizeof(long int);
        APROF_(fix_access_size)(addr, size);
            
        while(size > 0) {
            
            APROF_(trace_access)(   LOAD, 
                                    addr, 
                                    APROF_(addr_multiple), False);
            
            size -= APROF_(addr_multiple);
            addr += APROF_(addr_multiple);
        }
    
    }
  
}

#endif
