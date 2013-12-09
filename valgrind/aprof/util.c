/*
 * Utils
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2014, Emilio Coppa (ercoppa@gmail.com),
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

#ifndef EXTERNAL
    #include "aprof.h"
#else
    #include "extra/aprof-helper.h"
    #include "hashtable/hashtable.h"
    #include "data-common.h"
#endif

UInt APROF_(str_hash)(const HChar *s) {
    
    UInt hash_value = 0;
    for (; *s; s++)
        hash_value = 31 * hash_value + *s;
    
    APROF_(debug_assert)(hash_value > 0, "Invalid hash value");
    return hash_value;
}

RoutineInfo * APROF_(new_routine_info)(HashTable * rtn_ht, 
                                Function * fn, UWord target
                                #if APROF_TOOL
                                , ThreadData * tdata
                                #endif
                                ) {
    
    APROF_(debug_assert)(rtn_ht != NULL, "Thread data is not valid");
    APROF_(debug_assert)(fn != NULL, "Invalid function info");
    APROF_(debug_assert)(target > 0, "Invalid target");
    
    RoutineInfo * rtn_info = APROF_(new)(RT_S, sizeof(RoutineInfo));
    
    rtn_info->key = target;
    rtn_info->fn = fn;
        
    if (!rtn_info->fn->discard) {
        
        #if APROF_TOOL
        if (APROF_(runtime).single_report) {
        #endif
        
            if (rtn_info->fn->input_map == NULL)
                rtn_info->fn->input_map = HT_construct(VG_(free));
                
            rtn_info->input_map = rtn_info->fn->input_map;
            rtn_info->routine_id = rtn_info->fn->function_id;
                
        #if APROF_TOOL
        } else {
        
            rtn_info->routine_id = tdata->next_routine_id++;
            rtn_info->input_map = HT_construct(NULL);
        }
        #endif
    }
    
    HT_add_node(rtn_ht, rtn_info->key, rtn_info);
    return rtn_info;
}

void APROF_(destroy_routine_info)(RoutineInfo * ri) {
    
    HT_destruct(ri->input_map);
    APROF_(delete)(RT_S, ri);
}

void APROF_(destroy_function)(void * fnt) {
    
    Function * f = (Function *) fnt;
    
    APROF_(delete)(FN_NAME_S, f->name);
    if (f->mangled) APROF_(delete)(MANGLED_S, f->mangled);
    HT_destruct(f->input_map);
    
    APROF_(delete)(FN_S, f);
}

void APROF_(destroy_object)(void * obj) {
    Object * o = (Object *) obj;
    APROF_(delete)(OBJ_NAME_S, o->name);
    APROF_(delete)(OBJ_S, o);
}
