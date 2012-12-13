package aprofplot;

import java.io.*;
import java.util.*;
import java.util.regex.MatchResult;

public class AprofReport {

	private String appname;
	private String cmdline;
	private int version;
	private String metric;
	private double total_cost;
    private double total_self_cost;
	private long total_calls;
	private long total_contexts;
	private ArrayList<RoutineInfo> routines;
	private ArrayList<RoutineContext> contexts;
	private HashSet<String> favorites;
	private File file;
	private HashSet<String> libset;

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
		routines = new ArrayList<RoutineInfo>();
		contexts = new ArrayList<RoutineContext>();
		libset = new HashSet<String>();
		favorites = new HashSet<String>();
		file = f;

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
			String token = tokenizer.nextToken();

			if (token.equals("v")) { // report version number
				this.version = Integer.parseInt(tokenizer.nextToken());
				continue;
			}

			if (token.equals("m")) { // metric type
				this.metric = tokenizer.nextToken();
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
				
				RoutineInfo r = null;
				try {
					
					r = routines.get(rtn_id);
					if (r == null) throw new IndexOutOfBoundsException();
					r.setImage(lib);
					r.setName(rtn_name);
                    
				} catch(IndexOutOfBoundsException e) {
					
					if (contexts.isEmpty())
						r = new RoutineInfo(rtn_id, rtn_name, lib);
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
                
                if (this.version < 2) tokenizer.nextToken(); // skip sqr cost
                
                long occ = Long.parseLong(tokenizer.nextToken());
                
                double real = 0;
                double self = 0;
                if (this.version >= 2) {
                    real = Double.parseDouble(tokenizer.nextToken());
                    self = Double.parseDouble(tokenizer.nextToken());
                    total_self_cost += self;
                }
                
                Rms te = new Rms(rms, min_cost, max_cost, cost_sum, 
                                    real, self, occ);
                
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
                
                if (this.version < 2) tokenizer.nextToken(); // skip sqr cost
                
                long occ = Long.parseLong(tokenizer.nextToken());
                
                double real = 0;
                double self = 0;                
                if (this.version >= 2) {
                    real = Double.parseDouble(tokenizer.nextToken());
                    self = Double.parseDouble(tokenizer.nextToken());
                    total_self_cost += self;
                }
                
				Rms te = new Rms(rms, min_cost, max_cost, tot_cost, 
									real, self, occ);
				
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
				name.trim();
				routines.get(index).setFullName(name.substring(1, name.length() - 2));
				
				continue;
			}

			if (token.equals("u")) { // demangled routine
				
				int index = Integer.parseInt(tokenizer.nextToken());
				String name = "";
				while (tokenizer.hasMoreTokens()) {
					name += (tokenizer.nextToken() + " ");
				}
				name.trim();
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
		
		//System.out.println("Readed whole input file");
		
		if (hasContexts()) {
			
			Iterator it = routines.iterator();
			while(it.hasNext()) {
					
				ContextualizedRoutineInfo r = (ContextualizedRoutineInfo) it.next();
				r.mergeLazyList();
				
			}
			
		}
		
		total_contexts = contexts.size(); 
		
		for (int i=0; i<routines.size(); i++) {
			
			long calls = routines.get(i).getTotalCalls();
			if (routines.get(i).getTotalCost() > total_cost)
				total_cost = routines.get(i).getTotalCost();
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
        
        if (total_self_cost != total_cost) {
            System.out.println("Total cost: " + total_cost);
            System.out.println("Total self: " + total_self_cost);
        }

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
}
