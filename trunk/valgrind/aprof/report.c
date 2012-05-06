/*
 * Report generator
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

/*
static char * basename (char * path) {
	
	char * ptr = NULL;
	while(1) {
		
		ptr = VG_(strrchr)(path, '/');
		if (ptr == NULL) break;
		
		path = ptr + 1;
		if (*path == '\0') break;
		
	}
	
	return path;
	
}
*/

static char * basename(char *p) {

	char *last_component;
	int was_slash;

	last_component = p;
	was_slash = (*p++ == '/');

	while (*p) {
		
		if (*p == '/') {
			*p = '\0';
			was_slash = 1;
		} else {
			if (was_slash) {
			last_component = p;
			was_slash = 0;
			}
		}
		p++;
	}

	return last_component;
}


void generate_report(ThreadData * tdata, ThreadId tid) {
	
	Char filename_priv[2048];
	Char * prog_name = (Char *) VG_(args_the_exename);
	
	#if REPORT_NAME == 1
	if (tid > 1)
		VG_(sprintf)(filename_priv, "%s_%u.aprof", basename(prog_name), tid - 1);
	else
		VG_(sprintf)(filename_priv, "%s.aprof", basename(prog_name));
	#elif REPORT_NAME == 2
	VG_(sprintf)(filename_priv, "%d_%u_%d.aprof", VG_(getpid)(), tid - 1, ADDR_MULTIPLE);
	#endif
	/* Add path to log filename */
	Char * filename = VG_(expand_file_name)("aprof log", filename_priv);

    // open report file
	FILE * report = ap_fopen(filename);
	AP_ASSERT(report != NULL, "Can't create report file");

	char buffer[10000];

	#if REPORT_VERSION > 0

	// write header
	VG_(sprintf)(buffer, "c -------------------------------------\n");
	ap_fwrite(report, buffer, VG_(strlen)(buffer));
	VG_(sprintf)(buffer, "c report generated by aprof-valgrind\n");
	ap_fwrite(report, buffer, VG_(strlen)(buffer));
	VG_(sprintf)(buffer, "c -------------------------------------\n");
	ap_fwrite(report, buffer, VG_(strlen)(buffer));
	
	// write version 
	VG_(sprintf)(buffer, "v %d\n", REPORT_VERSION);
	ap_fwrite(report, buffer, VG_(strlen)(buffer));
	
	#if REPORT_VERSION == 1
	#if TIME == BB_COUNT
	VG_(sprintf)(buffer, "k %llu\n", tdata->bb_c);
	#elif TIME == RDTSC
	VG_(sprintf)(buffer, "k %llu\n", ap_time() - tdata->entry_time);
	#endif
	ap_fwrite(report, buffer, VG_(strlen)(buffer));
	#endif
	
	// write metric type
	#if TIME == BB_COUNT
	VG_(sprintf)(buffer, "m bb-count\n");
	ap_fwrite(report, buffer, VG_(strlen)(buffer));
	#elif TIME == RDTSC
	VG_(sprintf)(buffer, "m time-usec\n");
	ap_fwrite(report, buffer, VG_(strlen)(buffer));
	#endif
	
	#if EVENTCOUNT
	VG_(sprintf)(buffer, "c JSR=%llu - RTS=%llu - RD=%llu - WR=%llu\n", 
			tdata->num_func_enter, tdata->num_func_exit, 
			tdata->num_read + tdata->num_modify,
			tdata->num_write + tdata->num_modify);
	ap_fwrite(report, buffer, VG_(strlen)(buffer));
    #endif
	
	#endif
	
	// write application name
	VG_(sprintf)(buffer, "a %s\n", prog_name);
	ap_fwrite(report, buffer, VG_(strlen)(buffer));
	
	// write commandline
	VG_(sprintf)(buffer, "f %s", prog_name);
	ap_fwrite(report, buffer, VG_(strlen)(buffer));
	XArray * x = VG_(args_for_client);
	int i = 0;
	for (i = 0; i < VG_(sizeXA)(x); i++) {
		HChar ** c = VG_(indexXA)(x, i);
		if (c != NULL) {
			VG_(sprintf)(buffer, " %s", *c);
			ap_fwrite(report, buffer, VG_(strlen)(buffer));
		}
	}
	VG_(sprintf)(buffer, "\n");
	ap_fwrite(report, buffer, VG_(strlen)(buffer));

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
		
		char * rtn_name = rtn_info->fn->mangled;
		if (rtn_name == NULL) rtn_name = rtn_info->fn->name;
		
		#if REPORT_VERSION == 0
		VG_(sprintf)(buffer, "r %s %s %llu\n", rtn_name, obj_name, 
							rtn_info->routine_id);
		#elif REPORT_VERSION == 1
		VG_(sprintf)(buffer, "r \"%s\" \"%s\" %llu\n", rtn_name, 
					obj_name, rtn_info->routine_id);
		#endif
		ap_fwrite(report, buffer, VG_(strlen)(buffer));
		
		#if REPORT_VERSION == 1
		if (rtn_info->fn->mangled != NULL) {
			VG_(sprintf)(buffer, "d %llu \"%s\"\n", rtn_info->routine_id, rtn_info->fn->name);
			ap_fwrite(report, buffer, VG_(strlen)(buffer));
		}
		#endif

		#if CCT

		HT_ResetIter(rtn_info->context_sms_map);
		HashTable * ht = HT_RemoveNext(rtn_info->context_sms_map);
		
		while (ht != NULL) {
			
			HT_ResetIter(ht);
			SMSInfo * info_access = HT_RemoveNext(ht);
			
			while (info_access != NULL) {
				
				#if REPORT_VERSION == 0
				ULong time_exec = info_access->cumulative_time_sum 
										/ info_access->calls_number;
				
				VG_(sprintf)(buffer, "q %lu %lu %llu %lu\n", 
								ht->key, info_access->key, time_exec,
								info_access->calls_number);
				#elif REPORT_VERSION == 1
				VG_(sprintf)(buffer, "q %lu %lu %u %u %llu %llu %llu\n",
					ht->key, 
					info_access->key,
					info_access->min_cumulative_time,
					info_access->max_cumulative_time,
					info_access->cumulative_time_sum, 
					info_access->cumulative_time_sqr_sum, 
					info_access->calls_number);
				#endif
				ap_fwrite(report, buffer, VG_(strlen)(buffer));

				VG_(free)(info_access);
				info_access = HT_RemoveNext(ht);

			}
			
			HT_destruct(ht);
			ht = HT_RemoveNext(rtn_info->context_sms_map);
		}
		#else

		// iterate over sms records of current routine
		HT_ResetIter(rtn_info->sms_map);
		SMSInfo * info_access = HT_RemoveNext(rtn_info->sms_map);
		
		while (info_access != NULL) {
			
			#if REPORT_VERSION == 1
			VG_(sprintf)(buffer, "p %llu %lu %u %u %llu %llu %llu\n", 
				rtn_info->routine_id,
				info_access->key,
				info_access->min_cumulative_time,
				info_access->max_cumulative_time,
				info_access->cumulative_time_sum, 
				info_access->cumulative_time_sqr_sum, 
				info_access->calls_number);
			#elif REPORT_VERSION == 0
			ULong time_exec = info_access->cumulative_time_sum 
									/ info_access->calls_number;
			VG_(sprintf)(buffer, "p %llu %lu %llu %llu\n", 
				rtn_info->routine_id, info_access->key, time_exec, 
				info_access->calls_number);
			#endif
			
			ap_fwrite(report, buffer, VG_(strlen)(buffer));
			
			VG_(free)(info_access);
			info_access = HT_RemoveNext(rtn_info->sms_map);
		}
		#endif
		
		#if DISCARD_UNKNOWN
		}
		#endif

		#if CCT
		HT_destruct(rtn_info->context_sms_map);
		#else
		HT_destruct(rtn_info->sms_map);
		#endif
		VG_(free)(rtn_info);
		rtn_info = HT_RemoveNext(tdata->routine_hash_table);

	}
	
	#if CCT
	print_cct_info(report, tdata->root, 0);
	
	#if CCT_GRAPHIC
	VG_(sprintf)(filename_priv, "%s_%u.graph", basename(prog_name), tid - 1);
	filename = VG_(expand_file_name)("aprof log", filename_priv);
	FILE * cct_rep = ap_fopen(filename);
	AP_ASSERT(cct_rep != NULL, "Can't create CCT report file");
	VG_(sprintf)(buffer, "digraph G {\n");
	ap_fwrite(cct_rep, buffer, VG_(strlen)(buffer));
	print_cct_graph(cct_rep, tdata->root, 0, NULL);
	VG_(sprintf)(buffer, "}\n");
	ap_fwrite(cct_rep, buffer, VG_(strlen)(buffer));
	ap_fclose(cct_rep);
	#endif
	
	#endif

	// close report file
	ap_fclose(report);
	
}
