/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aprofplot;

import java.io.*;
import java.util.*;
import java.util.regex.MatchResult;

/**
 *
 * @author bruno
 */
public class AprofReport {

    private String appname;
    private String cmdline;
    private int version;
    private String metric;
    private long total_cost;
    private int total_calls;
    private int total_contexts;
    private ArrayList<RoutineInfo> routines;
    private HashSet<String> favorites;
    private File file;
    private ArrayList<String> liblist;

    public AprofReport(File f) throws Exception {

        this.total_cost = 0;
        this.total_calls = 0;
        this.total_contexts = 0;
        this.routines = new ArrayList<RoutineInfo>();
        this.liblist = new ArrayList<String>();
        this.favorites = new HashSet<String>();
        this.file = f;

        HashSet<String> libset = new HashSet<String>();
        BufferedReader in = new BufferedReader(new FileReader(f));
        RoutineInfo rtn_info = null;
//        ArrayList<ContextualizedRoutineInfo> contexts = new ArrayList<ContextualizedRoutineInfo>(); // temporary
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

        // build all stuff...
//        System.out.println("Processing routines...");
        //routines:
        for (int i = 0; i < routines_temporary.size(); i++) {
            routines.add(null);
        }
        for (int i = 0; i < routines_temporary.size(); i++) {
//            System.out.println("Processing string: " + routines_temporary.get(i));
            str = routines_temporary.get(i);
            
            
            if (str.length() - 1 < 2)
                throw(new Exception("Invalid routine line"));
                
            String rtn_name = null;
            String target = null;
            String lib = null;
            String id = null;
            
            if (str.charAt(2) == '"') {
                
                Scanner s = new Scanner(str);
                s.findInLine("r \"([^\"]+)\" ([0-9A-Fx]+) \"([^\"]+)\" ([0-9]+)");
                MatchResult result = s.match();
                
                rtn_name = result.group(1);
                target = result.group(2);
                lib = result.group(3);
                id = result.group(4);
                
            } else {
                
                tokenizer = new StringTokenizer(str);
                tokenizer.nextToken(); // discard first token
                //if (rtn_info == null) System.out.println("rtn_info == null !!!");
                rtn_name = tokenizer.nextToken();
                target = tokenizer.nextToken();
                lib = tokenizer.nextToken();
                id = tokenizer.nextToken();
                
            }
            
            int index = Integer.parseInt(id);
            rtn_info = new UncontextualizedRoutineInfo(rtn_name, 
                                                        target, lib);
            routines.remove(index);
            routines.add(index, rtn_info);
            if (!libset.contains(rtn_info.getImage())) {
                libset.add(rtn_info.getImage());
                liblist.add(rtn_info.getImage());
            }
        }
//        for (int i = 0; i < routines_temporary.size(); i++) {
//            if (routines.get(i) == null) System.out.println(i + "-> null");
//        }
        //p points:
//        System.out.println("Processing p points...");
        for (int i = 0; i < p_points_temporary.size(); i++) {
            tokenizer = new StringTokenizer(p_points_temporary.get(i));
            tokenizer.nextToken(); // discard first token
            int index = Integer.parseInt(tokenizer.nextToken());
            int accesses = Integer.parseInt(tokenizer.nextToken());
            double time = Double.parseDouble(tokenizer.nextToken());
            int occurrences = Integer.parseInt(tokenizer.nextToken());
            TimeEntry te = new TimeEntry(accesses, time, occurrences);
            routines.get(index).addTimeEntry(te);
        }
//        System.out.println("Processing demangled names...");
        //demangled names:
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
//        System.out.println("Processing full demangled names...");
        //full demangled names:
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
//        System.out.println("Processing contexts...");
        //contexts:
        contexts = new ContextualizedRoutineInfo[contexts_temporary.size() + 1];
//        for (int i = 0; i <= contexts_temporary.size(); i++) {
//            contexts.add(null);
//        }
        for (int i = 0; i < contexts_temporary.size(); i++) {
            //System.out.println("Processing context: " + contexts_temporary.get(i));
            tokenizer = new StringTokenizer(contexts_temporary.get(i));
            tokenizer.nextToken(); // discard first token
            int routine_id = Integer.parseInt(tokenizer.nextToken());
            int context_id = Integer.parseInt(tokenizer.nextToken());
            int parent_id = Integer.parseInt(tokenizer.nextToken());
            UncontextualizedRoutineInfo rtn = (UncontextualizedRoutineInfo)routines.get(routine_id);
            ContextualizedRoutineInfo parent = null;
            if (parent_id >= 0) parent = contexts[parent_id];//contexts.get(parent_id);
            rtn_info = new ContextualizedRoutineInfo(rtn.getRealName(), rtn.getAddress(), rtn.getImage(), parent, rtn);
            rtn_info.setDemName(rtn.getDemName());
            rtn_info.setFullDemName(rtn.getFullDemName());
            rtn.addContext((ContextualizedRoutineInfo)rtn_info);
            //contexts.remove(context_id);
            contexts[context_id] = (ContextualizedRoutineInfo)rtn_info;//contexts.add(context_id, (ContextualizedRoutineInfo)rtn_info);
            this.total_contexts++;
        }
//        System.out.println("Processing q points...");
        //q points:
        for (int i = 0; i < q_points_temporary.size(); i++) {
            tokenizer = new StringTokenizer(q_points_temporary.get(i));
            tokenizer.nextToken(); // discard first token
            int context_id = Integer.parseInt(tokenizer.nextToken());
            int accesses = Integer.parseInt(tokenizer.nextToken());
            double time = Double.parseDouble(tokenizer.nextToken());
            int occurrences = Integer.parseInt(tokenizer.nextToken());
            TimeEntry te = new TimeEntry(accesses, time, occurrences);
            contexts[context_id].addTimeEntry(te);//contexts.get(context_id).addTimeEntry(te);
            rtn_info = contexts[context_id].getOverallRoutineInfo();//contexts.get(context_id).getOverallRoutineInfo();
            rtn_info.addTimeEntry(te);
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
        if (!checkFiles(files)) throw(new Exception("reports come from different programs"));
        File res;
        String appname = null, cmdline = null;
        HashMap<RoutineInfo, HashMap<Integer, TimeEntry>> rtnmap = new HashMap<RoutineInfo, HashMap<Integer, TimeEntry>>();
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
                        rtnmap.put(rtn_info, new HashMap<Integer, TimeEntry>());
                    }
                    indexmap.put(new Integer(index), rtn_info);
                    continue;
                }
                if (token.equals("p")) {
                    int index = Integer.parseInt(tokenizer.nextToken());
                    RoutineInfo rtn_info = indexmap.get(new Integer(index));
                    HashMap<Integer, TimeEntry> timemap = rtnmap.get(rtn_info);
                    int accesses = Integer.parseInt(tokenizer.nextToken());
                    double time = Double.parseDouble(tokenizer.nextToken());
                    int occurrences = Integer.parseInt(tokenizer.nextToken());
                    if (timemap.containsKey(new Integer(accesses))) {
                        TimeEntry te = timemap.get(new Integer(accesses));
                        int finaloccurrences = te.getOccurrences() + occurrences;
                        double finaltime = (te.getTime() * te.getOccurrences() + time * occurrences) / (finaloccurrences);
                        te.setData(te.getAccesses(), finaltime, finaloccurrences);
                    }
                    else {
                        TimeEntry te = new TimeEntry(accesses, time, occurrences);
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
                    //routines.get(index).setFullDemName(name);
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
                    //routines.get(index).setDemName(name);
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
        Set<Map.Entry<RoutineInfo, HashMap<Integer, TimeEntry>>> set = rtnmap.entrySet();
        int i = 0;
        Iterator<Map.Entry<RoutineInfo, HashMap<Integer, TimeEntry>>> iterator = set.iterator();
        while(iterator.hasNext()) {
            Map.Entry<RoutineInfo, HashMap<Integer, TimeEntry>> entry = iterator.next();
            RoutineInfo rtn_info = entry.getKey();
            out.println("r " + rtn_info.getRealName() + " " + rtn_info.getAddress() + " " + rtn_info.getImage() + " " + i);
            i++;
        }
        i = 0;
        iterator = set.iterator();
        while(iterator.hasNext()) {
            Map.Entry<RoutineInfo, HashMap<Integer, TimeEntry>> entry = iterator.next();
            HashMap<Integer, TimeEntry> timemap = entry.getValue();
            Set<Map.Entry<Integer, TimeEntry>> timeset = timemap.entrySet();
            Iterator<Map.Entry<Integer, TimeEntry>> timeiterator = timeset.iterator();
            while(timeiterator.hasNext()) {
                Map.Entry<Integer, TimeEntry> timemapentry = timeiterator.next();
                TimeEntry te = timemapentry.getValue();
                java.text.NumberFormat formatter = new java.text.DecimalFormat("#");
                out.println("p " + i + " " + te.getAccesses() + " " + formatter.format(te.getTime()) + " " + te.getOccurrences());
            }
            i++;
        }
        i = 0;
        iterator = set.iterator();
        while(iterator.hasNext()) {
            Map.Entry<RoutineInfo, HashMap<Integer, TimeEntry>> entry = iterator.next();
            RoutineInfo rtn_info = entry.getKey();
            if (!rtn_info.getRealName().equals(rtn_info.getFullDemName()))
                out.println("d " + i + " " + rtn_info.getFullDemName());
        }
        while(iterator.hasNext()) {
            Map.Entry<RoutineInfo, HashMap<Integer, TimeEntry>> entry = iterator.next();
            RoutineInfo rtn_info = entry.getKey();
            if (!rtn_info.getRealName().equals(rtn_info.getDemName()))
                out.println("u " + i + " " + rtn_info.getDemName());
        }
        out.close();
        return res;
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

    public int getTotalCalls() {
        return total_calls;
    }

    public int getTotalContexts() {
        return total_contexts;
    }

    public int getRoutineCount() {
        return this.routines.size();
    }

    public boolean isFavourite(String addr) {
        return favorites.contains(addr);
    }

    public void addToFavourites(String addr) {
        if (!favorites.contains(addr)) favorites.add(addr);
    }

    public void removeFromFavourites(String addr) {
        if (favorites.contains(addr)) favorites.remove(addr);
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
}
