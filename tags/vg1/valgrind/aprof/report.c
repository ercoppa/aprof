/*
 * Report generator
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

/* FixMe: capire come ricavare la commandline completa */

void generate_report(ThreadData * tdata) {
	
	Char filename_priv[256];
	ThreadId tid = thread_running();
	Char * prog_name = (Char *) VG_(args_the_exename);
	VG_(sprintf)(filename_priv, "%s_%u.aprof", prog_name, tid);
	Char * filename = VG_(expand_file_name)("aprof log", filename_priv);

    // open report file
	FILE * report = ap_fopen(filename);
	if (report == NULL) failure("Can't create report file");

	char buffer[250];

	// write command line and application name
	VG_(sprintf)(buffer, "f %s\na %s\n", prog_name, prog_name);
	ap_fwrite(report, buffer, VG_(strlen)(buffer));

	// iterate over routines
	HT_ResetIter(tdata->routine_hash_table);
	UWord key;
	void * value = NULL;

	while (HT_Next(tdata->routine_hash_table, &key, &value)) {
		
		RoutineInfo * rtn_info = (RoutineInfo *) value;
		VG_(sprintf)(buffer, "r %s %p %s %llu\n", rtn_info->name, 
						(void *) key, rtn_info->image_name, 
							rtn_info->routine_id);
		ap_fwrite(report, buffer, VG_(strlen)(buffer));
		
		/*
		VG_(sprintf)(buffer, "d %llu %s\n", rtn_info->routine_id, rtn_info->name);
		ap_fwrite(report, buffer, VG_(strlen)(buffer));
		*/

		#if CCT

		UWord ckey;
		void * cvalue;
		HT_ResetIter(rtn_info->context_sms_map);
		
		while (HT_Next(rtn_info->context_sms_map, &ckey, &cvalue)) {
			
			UWord skey;
			void * svalue;
			HT_ResetIter(cvalue);
			
			while (HT_Next(cvalue, &skey, &svalue)) {
				
				SMSInfo * info_access = (SMSInfo *) svalue;
				UWord64 time_exec = info_access->partial_cumulative_time / info_access->partial_calls_number;
				
				VG_(sprintf)(buffer, "q %llu %lu %llu %lu\n", (UWord64) ckey, skey, time_exec, info_access->partial_calls_number);
				ap_fwrite(report, buffer, VG_(strlen)(buffer));

			}
			
		}
		#else

		UWord skey;
		void * svalue;
		// iterate over sms records of current routine
		HT_ResetIter(rtn_info->sms_map);
		
		while (HT_Next(rtn_info->sms_map, &skey, &svalue)) {
			
			SMSInfo * info_access = (SMSInfo *) svalue;
			UWord64 time_exec = info_access->partial_cumulative_time / info_access->partial_calls_number;
			VG_(sprintf)(buffer, "p %llu %lu %llu %lu\n", rtn_info->routine_id, skey, time_exec, info_access->partial_calls_number);
			ap_fwrite(report, buffer, VG_(strlen)(buffer));
			
		}
		#endif

	}
	
	#if CCT
	print_cct_info(report, tdata->root, 0);
	#endif

	// close report file
	ap_fclose(report);
	
}
