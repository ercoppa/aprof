package aprofplot;

import java.io.*;
import java.util.*;
import java.util.regex.MatchResult;

public class AprofReport {

    // input metric type
    public static final int RMS  = 0;
    public static final int RVMS = 1;
    private int input_metric;
    
    // performance metric type
    private static final int BB   = 0;
    private static final int TIME = 1;
    private int performance_metric;
    
	private String appname;
	private String cmdline;
	private int version;
	private double total_cost;
    private double total_self_cost;
	private long total_calls;
	private long total_contexts;
    private long sum_rms;
    private long sum_rvms;
    private long sum_rvms_syscall_self;
    private long sum_rvms_thread_self;
    private long num_rms;
	private ArrayList<RoutineInfo> routines;
	private ArrayList<RoutineContext> contexts;
	private HashSet<String> favorites;
	private File file;
	private HashSet<String> libset;
    boolean has_rvms_stats = false;
    boolean has_distinct_rms = false;

	// Global stats about routines
	private int max_class = 30;
	private long[] num_class_sms = null;
	private long[] tot_calls_class_sms = null;
	private long[] max_calls_class_sms = null;

	public AprofReport(File f) throws Exception {

		// init object
		version = 0;
		total_cost = 0;
		total_calls = 0;
		total_contexts = 0;
        sum_rms = 0;
        sum_rvms = 0;
        num_rms = 0;
		routines = new ArrayList<RoutineInfo>();
		contexts = new ArrayList<RoutineContext>();
		libset = new HashSet<String>();
		favorites = new HashSet<String>();
		file = f;

        performance_metric = BB;
        input_metric = RMS;
        
		// stats
		num_class_sms = new long[max_class];
		tot_calls_class_sms = new long[max_class];
		max_calls_class_sms = new long[max_class];
  
		// read report file
		String str;
		StringTokenizer tokenizer;
		BufferedReader in = new BufferedReader(new FileReader(f));
        
		while ((str = in.readLine()) != null) {
			
			//System.out.println(str);
			
			tokenizer = new StringTokenizer(str);
            if (!tokenizer.hasMoreTokens()) continue;
			String token = tokenizer.nextToken();

			if (token.equals("v")) { // report version number
				this.version = Integer.parseInt(tokenizer.nextToken());
				continue;
			}

			if (token.equals("m")) { // performance metric type
				
                String m = tokenizer.nextToken();
				if (m.equals("bb_count"))
                    this.performance_metric = BB;
                else if (m.equals("time-usec"))
                    this.performance_metric = TIME;
                    
                continue;
			}

            if (token.equals("i")) { // input metric type
				
                String m = tokenizer.nextToken();
				if (m.equals("rms"))
                    this.input_metric = RMS;
                else if (m.equals("rvms"))
                    this.input_metric = RVMS;
                    
                continue;
			}
            
			if (token.equals("k")) { // total cost
				this.total_cost = Double.parseDouble(tokenizer.nextToken());
				continue;
			}

			if (token.equals("f")) { // command line
				this.cmdline = str.substring(2);
				continue;
			}

			if (token.equals("a")) { // application name
				this.appname = tokenizer.nextToken();
				continue;
			}
            
			if (token.equals("r")) { // routine
				
				String rtn_name;
				String lib;
                String sum = null;
				String id;
			
				Scanner s = new Scanner(str);
                s.findInLine("r \"([^\"]+)\" \"([^\"]+)\" ([0-9]+)");
                MatchResult result = s.match();

				rtn_name = result.group(1);
				lib = result.group(2);
                id = result.group(3);
                
				int rtn_id = Integer.parseInt(id);
                
				RoutineInfo r;
				try {
					
					r = routines.get(rtn_id);
					if (r == null) throw new IndexOutOfBoundsException();
					r.setImage(lib);
					r.setName(rtn_name);
                    
				} catch(IndexOutOfBoundsException e) {
					
					if (contexts.isEmpty())
						r = new RoutineInfo(rtn_id, rtn_name, lib, 0);
					else
						r = (RoutineInfo) new ContextualizedRoutineInfo(rtn_id, 
                                                   rtn_name, lib);
					
					while (routines.size() <= rtn_id) // Force capacity of array
						routines.add(null);
					
					routines.set(rtn_id, r);
					
				}
				
				if (!libset.contains(r.getImage()))
					libset.add(r.getImage());
				
				continue;
			}

            if (token.equals("g")) {
                
                if (!tokenizer.hasMoreTokens()) continue;
                int rtn_id = Integer.parseInt(tokenizer.nextToken());
                if (!tokenizer.hasMoreTokens()) continue;
                long rms = Long.parseLong(tokenizer.nextToken());
                
                has_distinct_rms = true;
                
                long calls = 0;
                if (tokenizer.hasMoreTokens()) {
                    
                    calls = Long.parseLong(tokenizer.nextToken());
                    
                }
                
                RoutineInfo r = routines.get(rtn_id);
                r.setCountRms(r.getCountRms() + 1);
                num_rms++;
                
            }
            
			if (token.equals("x")) { // context

				int routine_id = Integer.parseInt(tokenizer.nextToken());
				int context_id = Integer.parseInt(tokenizer.nextToken());
				int parent_id = Integer.parseInt(tokenizer.nextToken());
				
				RoutineContext c = null;
				RoutineContext p = null;
				RoutineInfo r = null;
				ContextualizedRoutineInfo rc = null;
				
				try { 
					r = routines.get(routine_id);
					if (r == null) throw new IndexOutOfBoundsException();
					if (!(r instanceof ContextualizedRoutineInfo)) {
						rc = new ContextualizedRoutineInfo(r.getID(), 
                                    r.getName(), r.getImage());
						routines.set(routine_id, rc);
					} else rc = (ContextualizedRoutineInfo)r;
				} catch (IndexOutOfBoundsException e) {
					rc = new ContextualizedRoutineInfo(routine_id);
					while (routines.size() <= routine_id) // Force capacity of array
						routines.add(null);
					routines.set(routine_id, rc);
				}
				
				try { 
					if (parent_id >= 0) {
						p = contexts.get(parent_id);
						if (p == null) throw new IndexOutOfBoundsException();
					}
				} catch (IndexOutOfBoundsException e) {
					p = new RoutineContext();
					while (contexts.size() <= parent_id) // Force capacity of array
						contexts.add(null);
					contexts.set(parent_id, p);
				}
				
				try {
					c = contexts.get(context_id);
					if (c == null) throw new IndexOutOfBoundsException();
					c.setParent(p);
					c.setOverallRoutine(rc);
					rc.addContext(c);
				} catch (IndexOutOfBoundsException e) {
					c = new RoutineContext(p, rc);
					while (contexts.size() <= context_id) // Force capacity of array
						contexts.add(null);
					contexts.set(context_id, c);
				}
				
				continue;
			}

			if (token.equals("p")) { // routine point
				
				int rtn_id = Integer.parseInt(tokenizer.nextToken());
				long rms = Long.parseLong(tokenizer.nextToken());
				long min_cost = Long.parseLong(tokenizer.nextToken());
				long max_cost = Long.parseLong(tokenizer.nextToken());
				double cost_sum = Double.parseDouble(tokenizer.nextToken());
                
                double cumul_sqr = 0;
                if (this.version < 2 || this.version > 3) 
                    cumul_sqr = Double.parseDouble(tokenizer.nextToken());
                    
                long occ = Long.parseLong(tokenizer.nextToken());
                
                double real = 0;
                double self = 0;
                if (this.version >= 2) {
                    real = Double.parseDouble(tokenizer.nextToken());
                    self = Double.parseDouble(tokenizer.nextToken());
                    total_self_cost += self;
                }
                
                long self_min = 0, self_max = 0;
                if (this.version >= 3) {
                    self_min = Long.parseLong(tokenizer.nextToken());
                    self_max = Long.parseLong(tokenizer.nextToken());
                }
                
                double self_sqr = 0;
                if (this.version > 3) 
                    self_sqr = Double.parseDouble(tokenizer.nextToken());
                
                long sum_rms = 0;
                long sum_sqr_rms = 0;
                long rvms_syscall = 0;
                long rvms_thread = 0;
                long rvms_syscall_self = 0;
                long rvms_thread_self = 0;
                if (this.version >= 5 && this.input_metric == RVMS) {
                    
                    sum_rms = Long.parseLong(tokenizer.nextToken());
                    sum_sqr_rms = Long.parseLong(tokenizer.nextToken());
                    this.sum_rms += sum_rms;
                    this.sum_rvms += rms * occ;
                    
                    has_distinct_rms = true;
                    
                    if (tokenizer.hasMoreTokens()) {
                    
                        rvms_syscall = Long.parseLong(tokenizer.nextToken());
                        rvms_thread = Long.parseLong(tokenizer.nextToken());
                        
                        has_rvms_stats = true;
                    }
                    
                    if (tokenizer.hasMoreTokens()) {
                    
                        rvms_syscall_self = Long.parseLong(tokenizer.nextToken());
                        rvms_thread_self = Long.parseLong(tokenizer.nextToken());
                        
                        this.sum_rvms_syscall_self += rvms_syscall_self;
                        this.sum_rvms_thread_self += rvms_thread_self;
                        
                    }
                }
                
                Rms te = new Rms(rms, min_cost, max_cost, cost_sum, 
                                    real, self, occ, self_min, self_max,
                                    cumul_sqr, self_sqr, sum_rms,
                                    sum_sqr_rms, rvms_syscall,
                                    rvms_thread, rvms_syscall_self,
                                    rvms_thread_self);
                
				RoutineInfo r = null;
				try {
					r = routines.get(rtn_id);
					if (r == null) throw new IndexOutOfBoundsException();
				} catch (IndexOutOfBoundsException e) {
					r = new RoutineInfo(rtn_id);
					while (routines.size() <= rtn_id) // Force capacity of array
						routines.add(null);
					routines.set(rtn_id, r);
				}
				r.addRms(te);
				
				continue;
			}

			if (token.equals("q")) { // context point
			
				int context_id = Integer.parseInt(tokenizer.nextToken());
				long rms = Long.parseLong(tokenizer.nextToken());
				long min_cost = Long.parseLong(tokenizer.nextToken());
				long max_cost = Long.parseLong(tokenizer.nextToken());
				double tot_cost = Double.parseDouble(tokenizer.nextToken());
                
                double cumul_sqr = 0;
                if (this.version < 2 || this.version > 3) 
                    cumul_sqr = Double.parseDouble(tokenizer.nextToken());
                
                long occ = Long.parseLong(tokenizer.nextToken());
                
                double real = 0;
                double self = 0;                
                if (this.version >= 2) {
                    real = Double.parseDouble(tokenizer.nextToken());
                    self = Double.parseDouble(tokenizer.nextToken());
                    total_self_cost += self;
                }
                
                long self_min = 0, self_max = 0;
                if (this.version >= 3) {
                    self_min = Long.parseLong(tokenizer.nextToken());
                    self_max = Long.parseLong(tokenizer.nextToken());
                }
                
                double self_sqr = 0;
                if (this.version > 3) 
                    self_sqr = Double.parseDouble(tokenizer.nextToken());
                
                long sum_rms = 0;
                long sum_sqr_rms = 0;
                long rvms_syscall = 0;
                long rvms_thread = 0;
                long rvms_syscall_self = 0;
                long rvms_thread_self = 0;
                if (this.version >= 5 && this.input_metric == RVMS) {
                    sum_rms = Long.parseLong(tokenizer.nextToken());
                    sum_sqr_rms = Long.parseLong(tokenizer.nextToken());
                    this.sum_rms += sum_rms;
                    this.sum_rvms += rms * occ;
                    has_distinct_rms = true;
                    
                    if (tokenizer.hasMoreTokens()) {
                    
                        rvms_syscall = Long.parseLong(tokenizer.nextToken());
                        rvms_thread = Long.parseLong(tokenizer.nextToken());
                        
                        has_rvms_stats = true;
                    }
                
                    if (tokenizer.hasMoreTokens()) {
                    
                        rvms_syscall_self = Long.parseLong(tokenizer.nextToken());
                        rvms_thread_self = Long.parseLong(tokenizer.nextToken());
                        
                        this.sum_rvms_syscall_self += rvms_syscall_self;
                        this.sum_rvms_thread_self += rvms_thread_self;
                        
                    }
                }
                
				Rms te = new Rms(rms, min_cost, max_cost, tot_cost, 
									real, self, occ, self_min, self_max,
                                    cumul_sqr, self_sqr, sum_rms,
                                    sum_sqr_rms, rvms_syscall,
                                    rvms_thread, rvms_syscall_self,
                                    rvms_thread_self);
				
				RoutineContext c = null;
				try {
					c = contexts.get(context_id);
					if (c == null) throw new IndexOutOfBoundsException();
				} catch (IndexOutOfBoundsException e) {
					c = new RoutineContext();
					while (contexts.size() <= context_id) // Force capacity of array
						contexts.add(null);
					contexts.set(context_id, c);
				}
				
				c.addRmsToContext(te);
				
				continue;
			}

			if (token.equals("d")) { // demangled routine name with full signature
				
				int index = Integer.parseInt(tokenizer.nextToken());
				String name = "";
				while (tokenizer.hasMoreTokens()) {
					name += (tokenizer.nextToken() + " ");
				}
				name = name.trim();
				routines.get(index).setFullName(name.substring(1, name.length() - 2));
				
				continue;
			}

			if (token.equals("u")) { // demangled routine
				
				int index = Integer.parseInt(tokenizer.nextToken());
                //System.out.println(index);
				String name = "";
				while (tokenizer.hasMoreTokens()) {
					name += (tokenizer.nextToken() + " ");
				}
                name = name.trim();
				routines.get(index).setMangledName(name);
				
				continue;
			}

			if (token.equals("c")) { // comment
				// skip comments
				continue;
			}

			if (token.equals("fav")) { // favourite routines
				// load favourite routines list
				while (tokenizer.hasMoreTokens())
					favorites.add(tokenizer.nextToken());
				continue;
			}

			//throw(new Exception());
		}
		in.close();
				
		if (hasContexts()) {
			
			Iterator it = routines.iterator();
			while(it.hasNext()) {
					
				ContextualizedRoutineInfo r = (ContextualizedRoutineInfo) it.next();
				r.mergeLazyList();
				
			}
			
		}
		
		total_contexts = contexts.size(); 
		
		for (int i = 0; i < routines.size(); i++) {
			
            //System.out.println("Routine i=" + i);
            
			long calls = routines.get(i).getTotalCalls();
			if (routines.get(i).getTotalCumulativeCost() > total_cost)
				total_cost = routines.get(i).getTotalCumulativeCost();
			total_calls += calls;
			
			int ne = routines.get(i).getSizeRmsList();
			int id = 0;
			if (ne > 1) {
				double idd = Math.log(ne) / Math.log(2);
				id = (int)idd;
				if (id > max_class - 1) id = max_class - 1;
			}
			num_class_sms[id]++;
			tot_calls_class_sms[id] += calls;
			if (max_calls_class_sms[id] < calls) max_calls_class_sms[id] = calls;
		}
        
        /*
        if (total_self_cost != total_cost) {
            System.out.println("Total cost: " + total_cost);
            System.out.println("Total self: " + total_self_cost);
        }
        */

	}

    public HashMap<String, Routine> getHashMapRoutines() {
        
        HashMap<String, Routine> h = new HashMap<String, Routine>();
        Iterator i = routines.iterator();
        while(i.hasNext()) {
            Routine r = (Routine) i.next();
            h.put(r.getName(), r);
        }
            
        return h;
    }
    
	public void save() throws Exception {
		
		File tmp = new File(this.file.getParent(), "aprof-plot.tmp");
		tmp.createNewFile();
		BufferedReader in = new BufferedReader(new FileReader(this.file));
		PrintWriter out = new PrintWriter(new FileWriter(tmp));
		String str;
		while ((str = in.readLine()) != null) {
			StringTokenizer tokenizer = new StringTokenizer(str);
			String token = tokenizer.nextToken();
			if (!token.equals("fav")) out.println(str);
		}
		if (!favorites.isEmpty()) {
			out.print("fav ");
			Iterator<String> iterator = favorites.iterator();
			while (iterator.hasNext()) {
				out.print(iterator.next() + " ");
			}
			out.println("");
		}
		in.close();
		out.close();
		this.file.delete();
		tmp.renameTo(this.file);
		this.file = tmp;
		
	}

	// check whether these reports come from the same program
	private static boolean checkFiles(File[] files) throws Exception {
		
		String cmdline = null, str;
		for (int i = 0; i < files.length; i++) {
			BufferedReader in = new BufferedReader(new FileReader(files[i]));
			while ((str = in.readLine()) != null) {
				StringTokenizer tokenizer = new StringTokenizer(str);
				String token = tokenizer.nextToken();
				if (token.equals("f")) {
					String s = str.substring(2);
					if (cmdline == null) cmdline = s;
					else if (!cmdline.equals(s)) return false;
					break;
				}
			}
			in.close();
		}
		return true;
	
	}

	public static File merge(File[] files, File savepath) throws Exception {
		
		if (!checkFiles(files)) throw(new Exception("reports come from different programs"));
		return null;
		
		/*
		File res;
		String appname = null, cmdline = null;
		HashMap<RoutineInfo, HashMap<Integer, SmsEntry>> rtnmap = new HashMap<RoutineInfo, HashMap<Integer, SmsEntry>>();
		for (int i = 0; i < files.length; i++) {
			
			HashMap<Integer, RoutineInfo> indexmap = new HashMap<Integer, RoutineInfo>();
			BufferedReader in = new BufferedReader(new FileReader(files[i]));
			String str;
			while ((str = in.readLine()) != null) {
				StringTokenizer tokenizer = new StringTokenizer(str);
				String token = tokenizer.nextToken();
				if (token.equals("f")) {
					cmdline = str.substring(2);
					continue;
				}
				if (token.equals("a")) {
					appname = tokenizer.nextToken();
					continue;
				}
				if (token.equals("r")) {
					RoutineInfo rtn_info = new UncontextualizedRoutineInfo(tokenizer.nextToken(), tokenizer.nextToken(), tokenizer.nextToken());
					int index = Integer.parseInt(tokenizer.nextToken());
					if (!rtnmap.containsKey(rtn_info)) {
						rtnmap.put(rtn_info, new HashMap<Integer, SmsEntry>());
					}
					indexmap.put(new Integer(index), rtn_info);
					continue;
				}
				if (token.equals("p")) {
					int index = Integer.parseInt(tokenizer.nextToken());
					RoutineInfo rtn_info = indexmap.get(new Integer(index));
					HashMap<Integer, SmsEntry> timemap = rtnmap.get(rtn_info);
					int accesses = Integer.parseInt(tokenizer.nextToken());
					double time = Double.parseDouble(tokenizer.nextToken());
					int occurrences = Integer.parseInt(tokenizer.nextToken());
					if (timemap.containsKey(new Integer(accesses))) {
						SmsEntry te = timemap.get(new Integer(accesses));
						int finaloccurrences = te.getOcc() + occurrences;
						double finaltime = (te.getCost() * te.getOcc() + time * occurrences) / (finaloccurrences);
						te.setData(te.getRms(), finaltime, finaloccurrences);
					}
					else {
						// CD111003: temporary...
						SmsEntry te = 
							new SmsEntry(accesses, 0, 0, time*occurrences, 0, occurrences);
						timemap.put(new Integer(accesses), te);
					}
					continue;
				}
				if (token.equals("d")) {
					// skip comments
					int index = Integer.parseInt(tokenizer.nextToken());
					String name = "";
					while (tokenizer.hasMoreTokens()) {
						name += (tokenizer.nextToken() + " ");
					}
					name.trim();
					RoutineInfo rtn_info = indexmap.get(new Integer(index));
					rtn_info.setFullDemName(name);
					//routines.get(rtn_id).setFullDemName(name);
					continue;
				}
				if (token.equals("u")) {
					// skip comments
					int index = Integer.parseInt(tokenizer.nextToken());
					String name = "";
					while (tokenizer.hasMoreTokens()) {
						name += (tokenizer.nextToken() + " ");
					}
					name.trim();
					RoutineInfo rtn_info = indexmap.get(new Integer(index));
					rtn_info.setDemName(name);
					//routines.get(rtn_id).setDemName(name);
					continue;
				}
				if (token.equals("c")) {
					// skip comments
					continue;
				}
				//throw(new Exception(files[i] + " is not a valid aprof report file"));
			}
			in.close();
		}
		int index = appname.indexOf(File.separator);
		String s = (index >= 0)? appname.substring(index + 1) : appname;
		res = new File(savepath, s + "_aprof.log");
		res.createNewFile();
		PrintWriter out = new PrintWriter(new FileWriter(res));
		out.println("f " + cmdline);
		out.println("a " + appname);
		Set<Map.Entry<RoutineInfo, HashMap<Integer, SmsEntry>>> set = rtnmap.entrySet();
		int i = 0;
		Iterator<Map.Entry<RoutineInfo, HashMap<Integer, SmsEntry>>> iterator = set.iterator();
		while(iterator.hasNext()) {
			Map.Entry<RoutineInfo, HashMap<Integer, SmsEntry>> entry = iterator.next();
			RoutineInfo rtn_info = entry.getKey();
			out.println("r " + rtn_info.getRealName() + " " + rtn_info.getAddress() + " " + rtn_info.getImage() + " " + i);
			i++;
		}
		i = 0;
		iterator = set.iterator();
		while(iterator.hasNext()) {
			Map.Entry<RoutineInfo, HashMap<Integer, SmsEntry>> entry = iterator.next();
			HashMap<Integer, SmsEntry> timemap = entry.getValue();
			Set<Map.Entry<Integer, SmsEntry>> timeset = timemap.entrySet();
			Iterator<Map.Entry<Integer, SmsEntry>> timeiterator = timeset.iterator();
			while(timeiterator.hasNext()) {
				Map.Entry<Integer, SmsEntry> timemapentry = timeiterator.next();
				SmsEntry te = timemapentry.getValue();
				java.text.NumberFormat formatter = new java.text.DecimalFormat("#");
				out.println("p " + i + " " + te.getRms() + " " + formatter.format(te.getCost()) + " " + te.getOcc());
			}
			i++;
		}
		i = 0;
		iterator = set.iterator();
		while(iterator.hasNext()) {
			Map.Entry<RoutineInfo, HashMap<Integer, SmsEntry>> entry = iterator.next();
			RoutineInfo rtn_info = entry.getKey();
			if (!rtn_info.getRealName().equals(rtn_info.getFullDemName()))
				out.println("d " + i + " " + rtn_info.getFullDemName());
		}
		while(iterator.hasNext()) {
			Map.Entry<RoutineInfo, HashMap<Integer, SmsEntry>> entry = iterator.next();
			RoutineInfo rtn_info = entry.getKey();
			if (!rtn_info.getRealName().equals(rtn_info.getDemName()))
				out.println("u " + i + " " + rtn_info.getDemName());
		}
		out.close();
		return res;
		
		*/
		
	}

	public String getAppName() {
		return this.appname;
	}

	public String getCommandLine() {
		return this.cmdline;
	}

	public double getTotalCost() {
		return total_cost;
	}
    
    public double getTotalSelfCost() {
        return total_self_cost;
    }

	public long getTotalCalls() {
		return total_calls;
	}

	public long getTotalContexts() {
		return total_contexts;
	}

	public int getRoutineCount() {
		return this.routines.size();
	}

	public boolean isFavorite(String fav) {
		return favorites.contains(fav);
	}

	public void addToFavorites(String fav) {
		if (!favorites.contains(fav)) favorites.add(fav);
	}

	public void removeFromFavorites(String fav) {
		if (favorites.contains(fav)) favorites.remove(fav);
	}

	public void sortRoutinesByTotalTimeDescending() {
		Collections.sort(routines, new Comparator<RoutineInfo> () {
			@Override
			public int compare(RoutineInfo r1, RoutineInfo r2) {
			   if (r1.getTotalCost() == r2.getTotalCost()) return 0;
			   if (r1.getTotalCost() < r2.getTotalCost()) return 1;
			   return -1;
			}
		});
	}
    
    public void sortRoutinesByRatioTuples() {
		Collections.sort(routines, new Comparator<RoutineInfo> () {
			@Override
			public int compare(RoutineInfo r1, RoutineInfo r2) {
			   if (r1.getRatioRvmsRms() == r2.getRatioRvmsRms()) return 0;
			   if (r1.getRatioRvmsRms() < r2.getRatioRvmsRms()) return 1;
			   return -1;
			}
		});
	}

    public void sortRoutinesByExternalInput() {
		Collections.sort(routines, new Comparator<RoutineInfo> () {
			@Override
			public int compare(RoutineInfo r1, RoutineInfo r2) {
			   if (r1.getRatioSumRmsRvms() == r2.getRatioSumRmsRvms()) return 0;
			   if ((1 - r1.getRatioSumRmsRvms()) < (1-r2.getRatioSumRmsRvms())) return 1;
			   return -1;
			}
		});
	}
    
	public ArrayList<Routine> getRoutines() {
		return new ArrayList<Routine>(this.routines);
	}

	public ArrayList<String> getLibList() {
		ArrayList<String> liblist = new ArrayList<String>(libset);
		return liblist;
	}
	
	public String getName() {
		return this.file.toString();
	}

	public final boolean hasContexts() {
		return !(contexts.isEmpty());
	}
	
	public long[] getMaxCallsClassSms() {
		return max_calls_class_sms.clone();
	}

	public long[] getNumCallsClassSms() {
		return num_class_sms.clone();
	}

	public long[] getTotCallsClassSms() {
		return tot_calls_class_sms.clone();
	}

	public long getCallsHottestRoutine() {
		long hottest_calls = 0;
		for (int i = 0; i < max_calls_class_sms.length; i++)
			if (max_calls_class_sms[i] > hottest_calls)
				hottest_calls = max_calls_class_sms[i];

		return hottest_calls;
	}

    public int getInputMetric() {
        return input_metric;
    }

    public double getRatioRmsRvms() {
        return (((double) sum_rms) / ((double) sum_rvms));
    }
    
    public boolean hasDistinctRms() {
        return has_distinct_rms;
    }

    public boolean hasRvmsStats() {
        return has_rvms_stats;
    }

    public void sortRoutinesByRatioRvmsThread() {
        Collections.sort(routines, new Comparator<RoutineInfo> () {
			@Override
			public int compare(RoutineInfo r1, RoutineInfo r2) {
			   if (r1.getRatioSumRvmsThread() == r2.getRatioSumRvmsThread()) return 0;
			   if ((r1.getRatioSumRvmsThread()) < (r2.getRatioSumRvmsThread())) return 1;
			   return -1;
			}
		});
    }
    
    public void sortRoutinesByRatioRvmsSyscall() {
        Collections.sort(routines, new Comparator<RoutineInfo> () {
			@Override
			public int compare(RoutineInfo r1, RoutineInfo r2) {
			   if (r1.getRatioSumRvmsSyscall() == r2.getRatioSumRvmsSyscall()) return 0;
			   if ((r1.getRatioSumRvmsSyscall()) < (r2.getRatioSumRvmsSyscall())) return 1;
			   return -1;
			}
		});
    }
    
    public void sortRoutinesByInducedAccesses() {
        Collections.sort(routines, new Comparator<RoutineInfo> () {
			@Override
			public int compare(RoutineInfo r1, RoutineInfo r2) {
			   if (r1.getRatioInducedAccesses() == r2.getRatioInducedAccesses()) return 0;
			   if (r1.getRatioInducedAccesses() < r2.getRatioInducedAccesses()) return 1;
			   return -1;
			}
		});
    }
    
    public long getSumRvmsSyscallSelf() {
        return sum_rvms_syscall_self;
    }

    public long getSumRvmsThreadSelf() {
        return sum_rvms_thread_self;
    }
}

