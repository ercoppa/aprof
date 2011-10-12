/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aprofplot;

import java.util.*;

/**
 *
 * @author bruno
 */
public abstract class RoutineInfo implements Comparable<RoutineInfo> {

    private int id;
    private String name, dem_name, full_dem_name;
    private String image;
    private double max_time;
    //private double max_ratio;
    //private double mean_ratio;
    private double total_time;
    private int total_calls;
    ArrayList<SmsEntry> time_entries;

    public RoutineInfo(int id, String name, String image) {
        this.id = id;
        this.name = this.dem_name = this.full_dem_name = name;
        this.image = image.substring(image.lastIndexOf('/') + 1);
        max_time = 0;
        total_time = 0;
        total_calls = 0;
        time_entries = new ArrayList<SmsEntry>();
    }

    public void addSmsEntry(SmsEntry t) {
        int old_te_index = time_entries.indexOf(t);
        if (old_te_index >= 0) {
            t = t.mergeWith(time_entries.get(old_te_index));
            time_entries.remove(old_te_index);
        }
        time_entries.add(t);
        if (t.getCost() > max_time) max_time = t.getCost();
        total_time += t.getCost() * t.getOcc();
        total_calls += t.getOcc();
    }

    public String getName() {
        if (Main.getRtnDisplayMode() == Main.DEMANGLED) return this.dem_name;
        else return this.full_dem_name;
    }

    public String getRealName() {
        return this.name;
    }

    public String getDemName() {
        return this.dem_name;
    }

    public String getFullDemName() {
        return this.full_dem_name;
    }

    public void setDemName(String dn) {
        this.dem_name = dn;
    }

    public void setFullDemName(String fdn) {
        this.full_dem_name = fdn;
    }


    public String getImage() {
        return this.image;
    }

    public double getMaxTime() {
        return max_time;
    }

//    public double getMaxRatio() {
//        return max_ratio;
//    }

    public double getMeanRatio() {
        double sum = 0;
        for (int i = 0; i < time_entries.size(); i++)
            sum += time_entries.get(i).getRatio();
        return sum / time_entries.size();
        //return mean_ratio;
    }

    public double getTotalTime() {
        return total_time;
    }

    public int getID() {
        return id;
    }

    public int getTotalCalls() {
        return this.total_calls;
    }

    public int getSizeTimeEntries() {
        return this.time_entries.size();
    }

    public ArrayList<SmsEntry> getTimeEntries() {
        //return (ArrayList<SmsEntry>)time_entries.clone();
        return new ArrayList<SmsEntry>(this.time_entries);
    }

    @Override
    public boolean equals(Object o) {
        if (o != null && getClass().equals(o.getClass())) {
            RoutineInfo rtn_info = (RoutineInfo)o;
            return rtn_info.id==this.id;
        }
        else return false;
    }

    @Override
    public int hashCode() {
        int hash = 3;
        hash = 31 * hash + this.id;
        return hash;
    }

    public int compareTo(RoutineInfo ri) {
        if (total_time == ri.total_time) return 0;
        if (total_time > ri.total_time) return 1;
        return -1;
    }

    public void sortTimeEntriesByAccesses() {
        Collections.sort(time_entries, new Comparator<SmsEntry> () {
           public int compare(SmsEntry t1, SmsEntry t2) {
               if (t1.getSms() == t2.getSms()) return 0;
               if (t1.getSms() > t2.getSms()) return 1;
               return -1;
           }
        });
    }

    public void sortTimeEntriesByTime() {
        Collections.sort(time_entries, new Comparator<SmsEntry> () {
           public int compare(SmsEntry t1, SmsEntry t2) {
               if (t1.getCost() == t2.getCost()) return 0;
               if (t1.getCost() > t2.getCost()) return 1;
               return -1;
           }
        });
    }

    public void sortTimeEntriesByRatio() {
        Collections.sort(time_entries, new Comparator<SmsEntry> () {
           public int compare(SmsEntry t1, SmsEntry t2) {
               if (t1.getRatio() == t2.getRatio()) return 0;
               if (t1.getRatio() > t2.getRatio()) return 1;
               return -1;
           }
        });
    }

    public void sortTimeEntriesByRatio(int type) {
        final int t = type;
        Collections.sort(time_entries, new Comparator<SmsEntry> () {
           public int compare(SmsEntry t1, SmsEntry t2) {
               if (t1.getRatio(t) == t2.getRatio(t)) return 0;
               if (t1.getRatio(t) > t2.getRatio(t)) return 1;
               return -1;
           }
        });
    }

    public void sortTimeEntriesByOccurrences() {
        Collections.sort(time_entries, new Comparator<SmsEntry> () {
           public int compare(SmsEntry t1, SmsEntry t2) {
               if (t1.getOcc() == t2.getOcc()) return 0;
               if (t1.getOcc() > t2.getOcc()) return 1;
               return -1;
           }
        });
    }
    
    public void sortTimeEntriesByVar() {
        Collections.sort(time_entries, new Comparator<SmsEntry> () {
           public int compare(SmsEntry t1, SmsEntry t2) {
               if (t1.getVar() == t2.getVar()) return 0;
               if (t1.getVar() > t2.getVar()) return 1;
               return -1;
           }
        });
    }
    
    public void sortTimeEntriesBySum() {
        Collections.sort(time_entries, new Comparator<SmsEntry> () {
           public int compare(SmsEntry t1, SmsEntry t2) {
               if (t1.getSumCost() == t2.getSumCost()) return 0;
               if (t1.getSumCost() > t2.getSumCost()) return 1;
               return -1;
           }
        });
    }
}
