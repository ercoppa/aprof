package aprofplot;

import java.io.*;
import java.util.*;
import java.util.regex.MatchResult;

public class AprofReport {

    private String appname;
    private String cmdline;
    private int version;
    private String metric;
    private long total_cost;
    private long total_calls;
    private long total_contexts;
    private ArrayList<RoutineInfo> routines;
    private HashSet<String> favorites;
    private File file;
    private ArrayList<String> liblist;

    public AprofReport(File f) throws Exception {

        // init object
        this.version = 0;
        this.total_cost = 0;
        this.total_calls = 0;
        this.total_contexts = 0;
        this.routines = new ArrayList<RoutineInfo>();
        this.liblist = new ArrayList<String>();
        this.favorites = new HashSet<String>();
        this.file = f;

        // init locals
        HashSet<String> libset = new HashSet<String>();
        BufferedReader in = new BufferedReader(new FileReader(f));
        RoutineInfo rtn_info = null;
        ContextualizedRoutineInfo[] contexts;
        ArrayList<String> routines_temporary = new ArrayList<String>();
        ArrayList<String> contexts_temporary = new ArrayList<String>();
        ArrayList<String> p_points_temporary = new ArrayList<String>();
        ArrayList<String> q_points_temporary = new ArrayList<String>();
        ArrayList<String> demangled_temporary = new ArrayList<String>();
        ArrayList<String> full_demangled_temporary = new ArrayList<String>();

        // read report file
        String str;
        StringTokenizer tokenizer;
        while ((str = in.readLine()) != null) {

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

            if (token.equals("f")) { // command line
                this.cmdline = str.substring(2);
                continue;
            }

            if (token.equals("a")) { // application name
                this.appname = tokenizer.nextToken();
                continue;
            }

            if (token.equals("r")) { // routine
                routines_temporary.add(str);
                continue;
            }

            if (token.equals("x")) { // context
                contexts_temporary.add(str);
                continue;
            }

            if (token.equals("p")) { // routine point
                p_points_temporary.add(str);
                continue;
            }

            if (token.equals("q")) { // context point
                q_points_temporary.add(str);
                continue;
            }

            if (token.equals("d")) { // demangled routine name
                demangled_temporary.add(str);
                continue;
            }

            if (token.equals("u")) { // demangled routine name with full signature
                full_demangled_temporary.add(str);
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

        // build routine data...
        for (int i = 0; i < routines_temporary.size(); i++)
            routines.add(null);

        for (int i = 0; i < routines_temporary.size(); i++) {

            str = routines_temporary.get(i);

            if (str.length() - 1 < 2)
                throw(new Exception("Invalid routine line"));

            String rtn_name = null;
            String lib = null;
            String id = null;

            // parse routine line (report version > 0)
            if (this.version > 0) {
                Scanner s = new Scanner(str);
                s.findInLine("r \"([^\"]+)\" \"([^\"]+)\" ([0-9]+)");
                MatchResult result = s.match();

                rtn_name = result.group(1);
                lib = result.group(2);
                id = result.group(3);
            }

            // parse routine line (report version == 0)
            else {
                tokenizer = new StringTokenizer(str);
                tokenizer.nextToken(); // discard first token
                rtn_name = tokenizer.nextToken();
                tokenizer.nextToken(); // discard routine address
                lib = tokenizer.nextToken();
                id = tokenizer.nextToken();
            }

            int rtn_id = Integer.parseInt(id);
            rtn_info = new UncontextualizedRoutineInfo(rtn_id, rtn_name, lib);
            routines.remove(rtn_id);
            routines.add(rtn_id, rtn_info);
            if (!libset.contains(rtn_info.getImage())) {
                libset.add(rtn_info.getImage());
                liblist.add(rtn_info.getImage());
            }
        }

        // p points:
        if (this.version > 0)
            for (int i = 0; i < p_points_temporary.size(); i++) {
                tokenizer = new StringTokenizer(p_points_temporary.get(i));
                tokenizer.nextToken(); // discard first token
                int rtn_id = Integer.parseInt(tokenizer.nextToken());
                int sms = Integer.parseInt(tokenizer.nextToken());
                long min_cost = Long.parseLong(tokenizer.nextToken());
                long max_cost = Long.parseLong(tokenizer.nextToken());
                long cost_sum = Long.parseLong(tokenizer.nextToken());
                long cost_sqr_sum = Long.parseLong(tokenizer.nextToken());
                int occ = Integer.parseInt(tokenizer.nextToken());
                SmsEntry te = new SmsEntry(sms,
                    min_cost, max_cost, cost_sum, cost_sqr_sum, occ);
                routines.get(rtn_id).addSmsEntry(te);
            }
        else
            for (int i = 0; i < p_points_temporary.size(); i++) {
                tokenizer = new StringTokenizer(p_points_temporary.get(i));
                tokenizer.nextToken(); // discard first token
                int index = Integer.parseInt(tokenizer.nextToken());
                int accesses = Integer.parseInt(tokenizer.nextToken());
                double time = Double.parseDouble(tokenizer.nextToken());
                int occurrences = Integer.parseInt(tokenizer.nextToken());
                SmsEntry te = new SmsEntry(
                    accesses, 0, 0, (long) (time * occurrences), 0, occurrences);
                routines.get(index).addSmsEntry(te);
            }

        // demangled names:
        for (int i = 0; i < demangled_temporary.size(); i++) {
            tokenizer = new StringTokenizer(demangled_temporary.get(i));
            tokenizer.nextToken(); // discard first token
            int index = Integer.parseInt(tokenizer.nextToken());
            String name = "";
            while (tokenizer.hasMoreTokens()) {
                name += (tokenizer.nextToken() + " ");
            }
            name.trim();
            routines.get(index).setFullDemName(name);
        }

        // full demangled names:
        for (int i = 0; i < full_demangled_temporary.size(); i++) {
            tokenizer = new StringTokenizer(full_demangled_temporary.get(i));
            tokenizer.nextToken(); // discard first token
            int index = Integer.parseInt(tokenizer.nextToken());
            String name = "";
            while (tokenizer.hasMoreTokens()) {
                name += (tokenizer.nextToken() + " ");
            }
            name.trim();
            routines.get(index).setDemName(name);
        }

        //contexts:
        contexts = new ContextualizedRoutineInfo[contexts_temporary.size() + 1];

        for (int i = 0; i < contexts_temporary.size(); i++) {
            tokenizer = new StringTokenizer(contexts_temporary.get(i));
            tokenizer.nextToken(); // discard first token
            int routine_id = Integer.parseInt(tokenizer.nextToken());
            int context_id = Integer.parseInt(tokenizer.nextToken());
            int parent_id = Integer.parseInt(tokenizer.nextToken());
            UncontextualizedRoutineInfo rtn =
                    (UncontextualizedRoutineInfo)routines.get(routine_id);
            ContextualizedRoutineInfo parent = null;
            if (parent_id >= 0) parent = contexts[parent_id];
            rtn_info = 
                    new ContextualizedRoutineInfo(
                        rtn.getID(),
                        rtn.getRealName(),
                        rtn.getImage(),
                        parent,
                        rtn);
            rtn_info.setDemName(rtn.getDemName());
            rtn_info.setFullDemName(rtn.getFullDemName());
            rtn.addContext((ContextualizedRoutineInfo)rtn_info);
            contexts[context_id] = (ContextualizedRoutineInfo)rtn_info;
            this.total_contexts++;
        }

        // q points:
        if (this.version > 0)
            for (int i = 0; i < q_points_temporary.size(); i++) {
                tokenizer = new StringTokenizer(q_points_temporary.get(i));
                tokenizer.nextToken(); // discard first token
                int context_id = Integer.parseInt(tokenizer.nextToken());
                int sms = Integer.parseInt(tokenizer.nextToken());
                long min_cost = Long.parseLong(tokenizer.nextToken());
                long max_cost = Long.parseLong(tokenizer.nextToken());
                long cost_sum = Long.parseLong(tokenizer.nextToken());
                long cost_sqr_sum = Long.parseLong(tokenizer.nextToken());
                int occ = Integer.parseInt(tokenizer.nextToken());
                SmsEntry te = new SmsEntry(sms,
                    min_cost, max_cost, cost_sum, cost_sqr_sum, occ);
                contexts[context_id].addSmsEntry(te);
                rtn_info = contexts[context_id].getOverallRoutineInfo();
                rtn_info.addSmsEntry(te);
            }
        else
            for (int i = 0; i < q_points_temporary.size(); i++) {
                tokenizer = new StringTokenizer(q_points_temporary.get(i));
                tokenizer.nextToken(); // discard first token
                int context_id = Integer.parseInt(tokenizer.nextToken());
                int sms = Integer.parseInt(tokenizer.nextToken());
                double time = Double.parseDouble(tokenizer.nextToken());
                int occ = Integer.parseInt(tokenizer.nextToken());
                SmsEntry te = new SmsEntry(
                    sms, 0, 0, (long) (time * occ), 0, occ);
                contexts[context_id].addSmsEntry(te);
                rtn_info = contexts[context_id].getOverallRoutineInfo();
                rtn_info.addSmsEntry(te);
            }

        for (int i=0; i<routines.size(); i++) {
            total_cost += routines.get(i).getTotalTime();
            total_calls += routines.get(i).getTotalCalls();
        }
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

    public void saveAs(File f) throws Exception {
        
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
        throw(new Exception("unsupported merge operation"));
        /*
        if (!checkFiles(files)) throw(new Exception("reports come from different programs"));
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
                        te.setData(te.getSms(), finaltime, finaloccurrences);
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
                out.println("p " + i + " " + te.getSms() + " " + formatter.format(te.getCost()) + " " + te.getOcc());
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

    public double getTotalTime() {
        return total_cost;
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
        //Collections.sort(routines);
        Collections.sort(routines, new Comparator<RoutineInfo> () {
           public int compare(RoutineInfo r1, RoutineInfo r2) {
               if (r1.getTotalTime() == r2.getTotalTime()) return 0;
               if (r1.getTotalTime() < r2.getTotalTime()) return 1;
               return -1;
           }
        });
    }

    public ArrayList<RoutineInfo> getRoutines() {
        return new ArrayList<RoutineInfo>(this.routines);
    }

    public ArrayList<String> getLibList() {
        return this.liblist;
    }
    
    public String getName() {
        return this.file.toString();
    }
}
