/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aprofplot;

/**
 *
 * @author bruno
 */
public class TimeEntry implements Comparable<TimeEntry> {

    private int accesses;
    private double time;
    private double ratio;
    private int occurrences;

    private static double[] ratio_configuration = {1, 0, 0}; //pos 0 -> n exponent; pos 1 -> log_e(n) exponent; pos 2 -> log_e(log_e(n)) exponent

    public TimeEntry(int accesses, double time, int occurrences) {
        this.accesses = accesses;
        this.time = time;
        this.occurrences = occurrences;
        if (this.accesses != 0) this.ratio = this.time / this.accesses;
        else this.ratio = this.time;
    }

    public static double[] getRatioConfiguration() {
        return ratio_configuration;
    }

    public static void setRatioConfiguration(double[] rc) {
        ratio_configuration = rc;
    }

    public int getAccesses() {
        return accesses;
    }

    public double getTime() {
        return time;
    }

    public double getRatio() {
        //return ratio;
        if (accesses == 0 || (accesses == 1 && (ratio_configuration[1] != 0 || ratio_configuration[2] != 0))) return time;
        double fraction = Math.pow(accesses, ratio_configuration[0]) * Math.pow(Math.log(accesses), ratio_configuration[1]) * Math.pow(Math.log(Math.log(accesses)), ratio_configuration[2]);
        return (fraction == 0)? time : time / fraction;
    }

    public double getRatio(int type) {
        switch (type) {
            case 0: return ratio;
            case 1: if (accesses <= 1) return time; else return (time / (accesses * Math.log(accesses)));
            case 2: if (accesses == 0) return time; else return (time / Math.pow(accesses, 2));
        }
        return 0;
    }

    public int getOccurrences() {
        return occurrences;
    }

    protected void setData(int accesses, double time, int occurrences) {
        this.accesses = accesses;
        this.time = time;
        this.occurrences = occurrences;
        if (this.accesses != 0) this.ratio = this.time / this.accesses;
        else this.ratio = this.time;
    }

    public int compareTo(TimeEntry t) {
        if (this.accesses == t.accesses) return 0;
        if (this.accesses > t.accesses) return 1;
        return -1;
    }

    @Override
    public boolean equals(Object o) {
        if (o != null && getClass().equals(o.getClass())) {
            TimeEntry te = (TimeEntry)o;
            return te.accesses == accesses;
        }
        else return false;
    }

    public TimeEntry mergeWith(TimeEntry te) {
        if (this.accesses != te.accesses) return null;
        int new_accesses = this.accesses;
        int new_occurrences = this.occurrences + te.occurrences;
        double new_time = (this.time * this.occurrences + te.time * te.occurrences) / new_occurrences;
        return new TimeEntry(new_accesses, new_time, new_occurrences);
    }
}