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
    private double cost_sqr_sum;
    private long occ;
    
    public static final int MAX_COST = 0;
    public static final int AVG_COST = 1;
    public static final int MIN_COST = 2;

    private double ratio;

    // pos 0 -> n exponent;
    // pos 1 -> log_e(n) exponent;
    // pos 2 -> log_e(log_e(n)) exponent
    private static double[] ratio_config = {1, 0, 0};

    public SmsEntry(int sms, long min_cost,
            long max_cost, long cost_sum, double cost_sqr_sum, long occ) {

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
                    ((getSumCost() / (double)getOcc()) * (getSumCost() / (double)getOcc()))
                );
        return variance;
    }

    public double getAvgCost() {
        return (double)this.cost_sum / this.occ;
    }

    public double getCost() {
        return getCost(MAX_COST);
    }
    
    public double getCost(int cost_type) {
        switch(cost_type) {
            case MAX_COST: return getMaxCost();
            case AVG_COST: return getAvgCost();
            case MIN_COST: return getMinCost();
            default: return 0; 
        }
    }

    public double getRatio() {
        return getRatioCost(MAX_COST);
    }

    public double getRatioCost(int cost_type) {
        if (sms == 0 || (sms == 1 && (ratio_config[1] != 0 || ratio_config[2] != 0)))
            switch(cost_type) {
                case MAX_COST: return getMaxCost();
                case AVG_COST: return getAvgCost();
                case MIN_COST: return getMinCost();
                default: return 0;
            }
        double fraction =
                Math.pow(sms, ratio_config[0]) *
                Math.pow(Math.log(sms), ratio_config[1]) *
                Math.pow(Math.log(Math.log(sms)), ratio_config[2]);
        switch(cost_type) {
            case MAX_COST: 
                return (fraction == 0) ? getMaxCost() : getMaxCost() / fraction;
            case AVG_COST: 
                return (fraction == 0) ? getAvgCost() : getAvgCost() / fraction;
            case MIN_COST: 
                return (fraction == 0) ? getMinCost() : getMinCost() / fraction;
            default: return 0;
        }
        
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

    public long getOcc() {
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
