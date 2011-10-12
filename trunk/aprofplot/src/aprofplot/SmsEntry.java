/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aprofplot;

/**
 *
 * @author bruno
 */
public class SmsEntry implements Comparable<SmsEntry> {

    private int sms;
    private long min_cost;
    private long max_cost;
    private long cost_sum;
    private long cost_sqr_sum;
    private int occ;

    private double ratio;

    // pos 0 -> n exponent;
    // pos 1 -> log_e(n) exponent;
    // pos 2 -> log_e(log_e(n)) exponent
    private static double[] ratio_config = {1, 0, 0};

    public SmsEntry(int sms, long min_cost,
            long max_cost, long cost_sum, long cost_sqr_sum, int occ) {

        this.sms = sms;
        this.min_cost = min_cost;
        this.max_cost = max_cost;
        this.cost_sum = cost_sum;
        this.cost_sqr_sum = cost_sqr_sum;
        this.occ = occ;
        if (this.sms != 0) this.ratio = this.getCost() / this.sms;
        else this.ratio = this.getCost();
    }

    public static double[] getRatioConfig() {
        return ratio_config;
    }

    public static void setRatioConfig(double[] rc) {
        ratio_config = rc;
    }

    public int getSms() {
        return sms;
    }

    public double getMinCost() {
        return this.min_cost;
    }

    public double getMaxCost() {
        return this.max_cost;
    }

    public double getSumCost() {
        return this.cost_sum;
    }

    public double getSumSquareCost() {
        return this.cost_sqr_sum;
    }

    public double getVar() {
        double variance = ( 
                    ( getSumSquareCost() / getOcc() ) -
                    ((getSumCost() / getOcc()) * (getSumCost() / getOcc()))
                );
        return variance;
    }

    public double getAvgCost() {
        return (double)this.cost_sum / this.occ;
    }

    public double getCost() {
        return this.getMaxCost();
    }

    public double getRatio() {
        if (sms == 0 || (sms == 1 && (ratio_config[1] != 0 || ratio_config[2] != 0)))
            return this.getCost();
        double fraction =
                Math.pow(sms, ratio_config[0]) *
                Math.pow(Math.log(sms), ratio_config[1]) *
                Math.pow(Math.log(Math.log(sms)), ratio_config[2]);
        return (fraction == 0)? this.getCost() : this.getCost() / fraction;
    }

    public double getRatio(int type) {
        double cost = this.getCost();
        switch (type) {
            case 0: return ratio;
            case 1: if (sms <= 1) return cost;
                    else return (cost / (sms * Math.log(sms)));
            case 2: if (sms == 0) return cost;
                    else return (cost / Math.pow(sms, 2));
        }
        return 0;
    }

    public int getOcc() {
        return occ;
    }

    public int compareTo(SmsEntry t) {
        if (this.sms == t.sms) return 0;
        if (this.sms > t.sms) return 1;
        return -1;
    }

    @Override
    public boolean equals(Object o) {
        if (o != null && getClass().equals(o.getClass())) {
            SmsEntry te = (SmsEntry)o;
            return te.sms == sms;
        }
        else return false;
    }

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 97 * hash + this.sms;
        return hash;
    }

    public SmsEntry mergeWith(SmsEntry te) {
        if (this.sms != te.sms) return null;
        return new SmsEntry(
                this.sms,
                Math.min(this.min_cost, te.min_cost),
                Math.max(this.max_cost, te.max_cost), 
                this.cost_sum + te.cost_sum,
                this.cost_sqr_sum + te.cost_sqr_sum,
                this.occ + te.occ);
    }
}
