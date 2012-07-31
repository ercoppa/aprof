/*
 * Report generator
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */
 
 /*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2012, Emilio Coppa (ercoppa@gmail.com),
                            Camil Demetrescu,
                            Irene Finocchi

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

// last modification time of the current program
static ULong binary_time = 0;

static Char * put_delim(Char * str, Int size) {
	
	Int skip = 0;
	Int i = 0;
	for (i = 0; i < size; i++) {
		
		if (str[i] == ' ' && !skip)
			str[i] = '@';
		
		else if (str[i] == '"')
			skip = ~skip;
		
		else if (str[i] == '\0')
			return str;
	}
	
	return str;
}

static Function * merge_tuple(Char * line, Int size, 
							Function * curr, ThreadData * tdata) {
	
	if (size <= 0) return curr;
	line = put_delim(line, size);
	
	Char * token = VG_(strtok)(line, "@");
	if (token == NULL) return curr;
	
	/* FixMe check exec mtime */
	
	
	if (token[0] == 'r') {
		
		// function name
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		token[VG_(strlen)(token) - 1] = '\0'; // remove last "
		token++; // skip first "
		Char * name = VG_(strdup)("fn_name", token);
		
		// object name
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		token[VG_(strlen)(token) - 1] = '\0'; // remove last "
		token++; // skip first "
		Char * obj_name = VG_(strdup)("obj_name", token);
		
		// Search function
		UInt hash = APROF_(str_hash)(name);
		curr = HT_lookup(APROF_(fn_ht), hash);
		while (curr != NULL && VG_(strcmp)(curr->name, name) != 0) {
			curr = curr->next;
		}
		
		//VG_(printf)("Parsed: %s %s\n", name, obj_name);
		
		if (curr == NULL) { // this is a new function
			
			//VG_(printf)("New Function: %s\n", name);
			
			curr = VG_(calloc)("fn", sizeof(Function), 1);
			#if DEBUG
			AP_ASSERT(curr != NULL, "Function not allocable");
			#endif
			
			curr->key = hash;
			curr->name = name;
			
			HT_add_node(APROF_(fn_ht), curr->key, curr);
			
		}
		
		if (curr->obj == NULL) { // new object
			
			UInt hash_obj = APROF_(str_hash)(obj_name);
			Object * obj = HT_lookup(APROF_(obj_ht), hash_obj);
			while (obj != NULL && VG_(strcmp)(obj->name, obj_name) != 0) {
				obj = obj->next;
			}
			
			//VG_(printf)("New object: %s\n", obj_name);
			
			obj = VG_(calloc)("obj", sizeof(Object), 1);
			#if DEBUG
			AP_ASSERT(obj != NULL, "Obj not allocable");
			#endif
			
			obj->key = hash_obj;
			obj->name = obj_name;
			obj->filename = NULL; /* FixMe */
			HT_add_node(APROF_(obj_ht), obj->key, obj);
			
			curr->obj = obj;
			
		}
		
	} else if (token[0] == 'd') { 
		
		if (curr == NULL || curr->mangled != NULL) 
			return curr;
		
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		
		// skip routine ID
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		
		// function mangled name
		token[VG_(strlen)(token) - 1] = '\0'; // remove last "
		token++; // skip first "
		curr->mangled = VG_(strdup)("mangled", token);
	
	} else if (token[0] == 'p') {
		
		// routine ID
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		
		// RMS
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		ULong rms = VG_(strtoull10) (token, NULL);
		if (rms == 0) return curr;
		
		// min
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		ULong min = VG_(strtoull10) (token, NULL);
		if (min == 0) return curr;
		
		// max
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		ULong max = VG_(strtoull10) (token, NULL);
		if (max == 0) return curr;
		
		// sum
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		ULong sum = VG_(strtoull10) (token, NULL);
		if (sum == 0) return curr;
		
		// sqr_sum
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		ULong sqr_sum = VG_(strtoull10) (token, NULL);
		if (sqr_sum == 0) return curr;
		
		// occ
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		ULong occ = VG_(strtoull10) (token, NULL);
		if (occ == 0) return curr;
		
		/*
		VG_(printf)("Tuple: %s %llu %llu %llu %llu %llu %llu\n",
						curr->name, rms, min, max, sum, sqr_sum, occ);
		*/
		
		RoutineInfo * rtn_info = HT_lookup(tdata->routine_hash_table, (UWord)curr);
		if (rtn_info == NULL) {
			
			rtn_info = APROF_(new_routine_info)(tdata, curr, (UWord) curr);
		
		} 
		
		RMSInfo * info_access = HT_lookup(rtn_info->rms_map, rms); 
		if (info_access == NULL) {
			
			info_access = (RMSInfo * ) VG_(calloc)("sms_info", 1, sizeof(RMSInfo));
			#if DEBUG
			AP_ASSERT(info_access != NULL, "rms_info not allocable in function exit");
			#endif
			
			info_access->min_cumulative_time = min;
			info_access->key = rms;
			HT_add_node(rtn_info->rms_map, info_access->key, info_access);
			
		} /*else {
			VG_(printf)("Old tuple: %s %lu %llu %llu %llu %llu %llu\n",
						curr->name, info_access->key, 
						info_access->min_cumulative_time, 
						info_access->max_cumulative_time, 
						info_access->cumulative_time_sum, 
						info_access->cumulative_time_sqr_sum, 
						info_access->calls_number);
		}*/
		
		info_access->cumulative_time_sum += sum;
		info_access->calls_number += occ;
		info_access->cumulative_time_sqr_sum += sqr_sum;
		
		if (info_access->max_cumulative_time < max) 
			info_access->max_cumulative_time = max;
	
		if (info_access->min_cumulative_time > min) 
			info_access->min_cumulative_time = min;
		/*
		VG_(printf)("Current tuple: %s %lu %llu %llu %llu %llu %llu\n",
						curr->name, info_access->key, 
						info_access->min_cumulative_time, 
						info_access->max_cumulative_time, 
						info_access->cumulative_time_sum, 
						info_access->cumulative_time_sqr_sum, 
						info_access->calls_number);
		*/
		
	} else if (token[0] == 'a') {
		
		token = VG_(strtok)(NULL, "@");
		Char app[1024] = {0};
		while (token != NULL) {
			//VG_(printf)("token: %s\n", token);
			VG_(strcat)(app, token);
			VG_(strcat)(app, " ");
			token = VG_(strtok)(NULL, "@");
		}
		if (VG_(strlen)(app) > 0)
			app[VG_(strlen)(app) -1] = '\0';
		
		//VG_(printf)("Command is #%s# versus #%s#\n", app, (Char *) VG_(args_the_exename));
		if (VG_(strcmp)(app, (Char *) VG_(args_the_exename)) != 0) 
			return (void *)1; /* special value */
	
	} else if (token[0] == 'a') {
		
		token = VG_(strtok)(NULL, "@");
		if (token == NULL) return curr;
		ULong sum = VG_(strtoull10) (token, NULL);
		if (sum == 0) return curr;
		
		tdata->other_metric += sum;
		
	}
	
	return curr;
}

static Bool merge_report(Char * report, ThreadData * tdata) {
	
	/* open report */
	SysRes res = VG_(open)(report, VKI_O_RDONLY,
								VKI_S_IRUSR|VKI_S_IWUSR);
	int file = (Int) sr_Res(res);
	AP_ASSERT(file != -1, "Can't read a log file.");
	
	Char buf[4096];
	Char line[1024];
	Int offset = 0;
	Function * current_routine = NULL;
	
	/* merge tuples */
	while (1) {
		
		Int r = VG_(read)(file, buf, 4096);
		if (r < 0) {
			VG_(printf)("Error when reading %s\n", report);
			return False;
		} else if (r == 0) return True; /* EOF */
		
		Int i = 0;
		for (i = 0; i < r; i++) {
			
			if (buf[i] == '\n') {
				
				line[offset++] = '\0';
				//VG_(printf)("# %s\n", line);
				current_routine = merge_tuple(line, offset,
						current_routine, tdata);
				
				/* this means that the report has a different command */
				if (current_routine == (void *)1) {
					//VG_(printf)("No merge\n");
					VG_(close)(file);
					return False;
				}
				
				offset = 0;
			
			} else line[offset++] = buf[i]; 
			
			AP_ASSERT(offset < 1024, "Line too long");
		}
		
	} 
	
	line[offset++] = '\0';
	//VG_(printf)("# %s\n", line);
	current_routine = merge_tuple(line, offset, current_routine, tdata);
	
	VG_(close)(file);
	return True;
} 

static Char * report_name(Char * filename_priv, Int tid) {

	#if REPORT_NAME == 1
	
	if (tid > 1)
		VG_(sprintf)(filename_priv, "%s_%u.aprof", VG_(basename)(prog_name), tid - 1);
	else
		VG_(sprintf)(filename_priv, "%s.aprof", VG_(basename)(prog_name));
	
	#elif REPORT_NAME == 2
	VG_(sprintf)(filename_priv, "%d_%u_%d.aprof", VG_(getpid)(), 
									tid - 1, APROF_(addr_multiple));
	#endif

	return filename_priv;

}


static Char ** search_report(Int * n, Bool all_runs) {
	
	SysRes r = VG_(open)("./", VKI_O_RDONLY, 
						VKI_S_IRUSR|VKI_S_IWUSR);
	int dir = (Int) sr_Res(r);
	AP_ASSERT(dir != -1, "Can't open directory.");
	
	struct vki_dirent * file;
	Char buf[1024] = {0};
	Char ** reports = VG_(calloc)("report array", sizeof(Char *) * 256, 1);
	
	for ( ; ; ) {
	
		Int res = VG_(getdents)(dir, (struct vki_dirent *) &buf, 1024);
		if (res == -1 || res == 0) {
			break;
		}
		
		Int i = 0;
		for (i = 0; i < res;) {

			file = (struct vki_dirent *) (buf + i);
			if (all_runs) {
				if (VG_(strlen)(file->d_name) > 6 && 
						VG_(strcmp)(".aprof", file->d_name + 
						VG_(strlen)(file->d_name) - 6) == 0) {
					
					reports[(*n)++] = VG_(strdup)("report", file->d_name);
					//VG_(printf)("File %s - %d\n", file->d_name, file->d_reclen);
				}
			} else {
				
				#if REPORT_NAME == 1
				
				if (VG_(strncmp)(file->d_name, VG_(basename)(prog_name),
						VG_(strlen)(VG_(basename)(prog_name))) == 0) {
				
				#elif REPORT_NAME == 2
				
				Char pid[10] = {0};
				VG_(sprintf)(pid, "%d", VG_(getpid)()); 
				if (VG_(strncmp)(file->d_name, pid,
						VG_(strlen)(pid)) == 0) {
				
				#endif	
				
					reports[(*n)++] = VG_(strdup)("report", file->d_name);
					//VG_(printf)("File %s - %d\n", file->d_name, file->d_reclen);
				}
				
			}
			i += file->d_reclen;
			
		}
	
	}
	
	return reports;
}

void APROF_(generate_report)(ThreadData * tdata, ThreadId tid) {
	
	Char filename_priv[1024] = {0};
	Char * prog_name = (Char *) VG_(args_the_exename);
	
	Int n = 0, j = 0;
	Char ** reports = NULL;
	
	if (APROF_(merge_report_runs))
		reports = search_report(&n, True);
	else if (APROF_(merge_report_threads))
		reports = search_report(&n, False);
	
	for (j = 0; j < n; j++) {
		
		//VG_(printf)("Merging report: %s\n", reports[j]);
		Bool m = merge_report(reports[j], tdata);
		if (m) {
			
			Char name[1024];
			VG_(sprintf)(name, "%s_merged", reports[j]);  
			VG_(rename) (reports[j], name);
			
			//VG_(unlink) (reports[j]);
		}
	}
	
	/*
	 * This does not work because we don't have the real path
	if (binary_time == 0) {
		
		// Get binary time
		struct vg_stat buf;
		SysRes b = VG_(stat)(prog_name, &buf);
		binary_time = buf.mtime;
		
	}
	*/
	
	/* Add path to log filename */
	Char * filename = VG_(expand_file_name)("aprof log", 
			report_name(filename_priv, tid));

    // open report file
	FILE * report = APROF_(fopen)(filename);
	AP_ASSERT(report != NULL, "Can't create report file");

	char buffer[10000];

	// write header
	VG_(sprintf)(buffer, "c -------------------------------------\n");
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
	VG_(sprintf)(buffer, "c report generated by aprof (valgrind)\n");
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
	VG_(sprintf)(buffer, "c -------------------------------------\n");
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
	
	// write version 
	VG_(sprintf)(buffer, "v %d\n", REPORT_VERSION);
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
	
	// Maximum value for the metric
	#if TIME == BB_COUNT
	VG_(sprintf)(buffer, "k %llu\n", tdata->bb_c + tdata->other_metric);
	#elif TIME == RDTSC
	VG_(sprintf)(buffer, "k %llu\n", APROF_(time)() - tdata->entry_time + tdata->other_metric);
	#endif
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
	
	// write mtime binary
	VG_(sprintf)(buffer, "e %llu\n", binary_time);
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
	
	// write metric type
	#if TIME == BB_COUNT
	VG_(sprintf)(buffer, "m bb-count\n");
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
	#elif TIME == RDTSC
	VG_(sprintf)(buffer, "m time-usec\n");
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
	#endif
	
	#if EVENTCOUNT
	VG_(sprintf)(buffer, "c JSR=%llu - RTS=%llu - RD=%llu - WR=%llu\n", 
			tdata->num_func_enter, tdata->num_func_exit, 
			tdata->num_read + tdata->num_modify,
			tdata->num_write + tdata->num_modify);
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
    #endif
	
	// write application name
	VG_(sprintf)(buffer, "a %s\n", prog_name);
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
	
	// write commandline
	VG_(sprintf)(buffer, "f %s", prog_name);
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
	XArray * x = VG_(args_for_client);
	int i = 0;
	for (i = 0; i < VG_(sizeXA)(x); i++) {
		HChar ** c = VG_(indexXA)(x, i);
		if (c != NULL) {
			VG_(sprintf)(buffer, " %s", *c);
			APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
		}
	}
	VG_(sprintf)(buffer, "\n");
	APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));

	#if DEBUG
	AP_ASSERT(tdata->routine_hash_table != NULL, "Invalid rtn ht");
	#endif

	// iterate over routines
	HT_ResetIter(tdata->routine_hash_table);

	RoutineInfo * rtn_info = HT_RemoveNext(tdata->routine_hash_table);
	while (rtn_info != NULL) {
		
		#if DEBUG
		AP_ASSERT(rtn_info != NULL, "Invalid rtn");
		AP_ASSERT(rtn_info->fn != NULL, "Invalid fn");
		AP_ASSERT(rtn_info->fn->name != NULL, "Invalid name fn");
		#endif
		
		#if DISCARD_UNKNOWN
		if (!rtn_info->fn->discard_info) {
		#endif
		
		char * obj_name = "NONE";
		if (rtn_info->fn->obj != NULL) obj_name = rtn_info->fn->obj->name; 
		
		VG_(sprintf)(buffer, "r \"%s\" \"%s\" %llu\n", rtn_info->fn->name, 
					obj_name, rtn_info->routine_id);
		APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
		
		if (rtn_info->fn->mangled != NULL) {
			VG_(sprintf)(buffer, "u %llu \"%s\"\n", rtn_info->routine_id, 
							rtn_info->fn->mangled);
			APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
		}

		#if CCT

		HT_ResetIter(rtn_info->context_rms_map);
		HashTable * ht = HT_RemoveNext(rtn_info->context_rms_map);
		
		while (ht != NULL) {
			
			HT_ResetIter(ht);
			RMSInfo * info_access = HT_RemoveNext(ht);
			
			while (info_access != NULL) {
				
				VG_(sprintf)(buffer, "q %lu %lu %u %u %llu %llu %llu\n",
					ht->key, 
					info_access->key,
					info_access->min_cumulative_time,
					info_access->max_cumulative_time,
					info_access->cumulative_time_sum, 
					info_access->cumulative_time_sqr_sum, 
					info_access->calls_number);

				APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));

				VG_(free)(info_access);
				info_access = HT_RemoveNext(ht);

			}
			
			HT_destruct(ht);
			ht = HT_RemoveNext(rtn_info->context_rms_map);
		}
		#else

		// iterate over rms records of current routine
		HT_ResetIter(rtn_info->rms_map);
		RMSInfo * info_access = HT_RemoveNext(rtn_info->rms_map);
		
		while (info_access != NULL) {
			
			VG_(sprintf)(buffer, "p %llu %lu %llu %llu %llu %llu %llu\n", 
				rtn_info->routine_id,
				info_access->key,
				info_access->min_cumulative_time,
				info_access->max_cumulative_time,
				info_access->cumulative_time_sum, 
				info_access->cumulative_time_sqr_sum, 
				info_access->calls_number);
			
			APROF_(fwrite)(report, buffer, VG_(strlen)(buffer));
			
			VG_(free)(info_access);
			info_access = HT_RemoveNext(rtn_info->rms_map);
		}
		#endif
		
		#if DISCARD_UNKNOWN
		}
		#endif

		#if CCT
		HT_destruct(rtn_info->context_rms_map);
		#else
		HT_destruct(rtn_info->rms_map);
		#endif
		
		VG_(free)(rtn_info);
		rtn_info = HT_RemoveNext(tdata->routine_hash_table);

	}
	
	#if CCT
	APROF_(print_report_CCT)(report, tdata->root, 0);
	#endif

	// close report file
	APROF_(fclose)(report);
	
}
