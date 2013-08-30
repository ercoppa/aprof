package aprofplot;

public class Rms implements Comparable<Rms> {

	private long rms;
	private long min_cost; // cumulative
	private long max_cost; // cumulative
	private double total_cost; // cumulative
    private double total_sqr_sum_cost; // cumulative
    private double total_real_cost;
	private double total_self;
    private long self_min;
    private long self_max;
    private double total_self_sqr_sum;
	private long occ;
    
    private long sum_rms;
    private long sum_sqr_rms;
    private long rvms_syscall;
    private long rvms_thread;
    private long rvms_syscall_self;
    private long rvms_thread_self;
	
	public static final int MAX_COST = 0;
	public static final int AVG_COST = 1;
	public static final int MIN_COST = 2;

	private double ratio;

	// pos 0 -> n exponent;
	// pos 1 -> log_e(n) exponent;
	// pos 2 -> log_e(log_e(n)) exponent
	private static double[] ratio_config = {1, 0, 0};

	public Rms(long rms, long min_cost, long max_cost, double total_cost, 
                double total_real_cost, double total_self, long occ,
                long self_min, long self_max, double cumul_sqr,
                double self_sqr, long sum_rms, long sum_sqr_rms,
                long rvms_syscall, long rvms_thread, long rvms_syscall_self,
                long rvms_thread_self) {

		this.rms = rms;
		this.min_cost = min_cost;
		this.max_cost = max_cost;
		this.total_cost = total_cost;
        this.total_sqr_sum_cost = cumul_sqr;
        this.total_real_cost = total_real_cost;
		this.total_self = total_self;
        this.self_min = self_min;
        this.self_max = self_max;
        this.total_self_sqr_sum = self_sqr;
		this.occ = occ;
		this.sum_rms = sum_rms;
        this.sum_sqr_rms = sum_sqr_rms;
        this.rvms_syscall = rvms_syscall;
        this.rvms_thread = rvms_thread;
        this.rvms_syscall_self = rvms_syscall_self;
        this.rvms_thread_self = rvms_thread_self;
        
        long cost = 0;
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            cost = this.max_cost;
        else
            cost = this.self_max;
        
		if (rms > 0) this.ratio = cost / this.rms;
		else this.ratio = cost;
	
	}

	public static double[] getRatioConfig() {
		return ratio_config;
	}

	public static void setRatioConfig(double[] rc) {
		ratio_config = rc;
	}

	public long getRms() {
		return rms;
	}

	public double getMinCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            return getCumulativeMinCost();
        
		return getSelfMinCost();
	}

	public double getMaxCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
    		return getCumulativeMaxCost();
        
        return getSelfMaxCost();
	}

	public double getTotalCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
    		return getTotalCumulativeCost();
	
        return getTotalSelfCost();
    }
    
    public double getTotalRealCost() {
        
        // old report
        if (total_real_cost == 0 && total_self == 0)
            return getTotalCost();
        
        return total_real_cost;
    }
   
     public double getTotalCumulativeCost() {
        return total_cost;
    }
    
    public double getTotalSelfCost() {
        return total_self;
    }
    
	public double getSqrTotalCost() {
        
		if (Main.getChartCost() == Main.COST_CUMULATIVE)
            return getTotalCumulativeSqrCost();
        
        return getTotalSelfSqrCost();
    }

	public double getVar() {
		double variance = ( 
					( getSqrTotalCost() / getOcc() ) -
					((getTotalCost() / (double)getOcc()) * (getTotalCost() / (double)getOcc()))
				);
        
        if (variance < 0) return 0;
        
		return variance;
	}

	public double getAvgCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
    		return getCumulativeAvgCost();
        
		return getSelfAvgCost();
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
		if (rms == 0 || (rms == 1 && (ratio_config[1] != 0 || ratio_config[2] != 0)))
			switch(cost_type) {
				case MAX_COST: return getMaxCost();
				case AVG_COST: return getAvgCost();
				case MIN_COST: return getMinCost();
				default: return 0;
			}
		double fraction =
				Math.pow(rms, ratio_config[0]) *
				Math.pow(Math.log(rms), ratio_config[1]) *
				Math.pow(Math.log(Math.log(rms)), ratio_config[2]);
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
			case 1: if (rms <= 1) return cost;
					else return (cost / (rms * Math.log(rms)));
			case 2: if (rms == 0) return cost;
					else return (cost / Math.pow(rms, 2));
		}
		return 0;
	}

	public long getOcc() {
		return occ;
	}

	@Override
	public int compareTo(Rms t) {
		if (this.rms == t.rms) return 0;
		if (this.rms > t.rms) return 1;
		return -1;
	}

	@Override
	public boolean equals(Object o) {
		if (o != null && getClass().equals(o.getClass())) {
			Rms te = (Rms)o;
			return te.rms == rms;
		}
		else return false;
	}

	@Override
	public int hashCode() {
		int hash = 7;
		hash = 97 * hash + (int)this.rms;
		return hash;
	}

	public Rms mergeWith(Rms te) {
		if (this.rms != te.rms) return null;
		return new Rms(
				this.rms,
				Math.min(this.min_cost, te.getCumulativeMinCost()),
				Math.max(this.max_cost, te.getCumulativeMaxCost()), 
				this.total_cost + te.getTotalCumulativeCost(),
                this.total_real_cost + te.getTotalRealCost(),
				this.total_self + te.getTotalSelfCost(),
				this.occ + te.getOcc(),
                this.self_min + te.getSelfMinCost(),
                this.self_max + te.getSelfMaxCost(),
                this.total_sqr_sum_cost + te.getTotalCumulativeSqrCost(),
                this.total_sqr_sum_cost + te.getTotalSelfSqrCost(),
                this.sum_rms + te.getSumRms(),
                this.sum_sqr_rms + te.getSumSqrRms(),
                this.rvms_syscall + te.getSumRvmsSyscall(),
                this.rvms_thread + te.getSumRvmsThread(),
                this.rvms_syscall_self + te.getSumRvmsSyscallSelf(),
                this.rvms_thread_self + te.getSumRvmsThreadSelf());
	}

    public long getCumulativeMinCost() {
        return min_cost;
    }
    
    public long getCumulativeMaxCost() {
        return max_cost;
    }
    
    public long getSelfMinCost() {
        return self_min;
    }

    public long getSelfMaxCost() {
        return self_max;
    }

    public double getCumulativeAvgCost() {
        return getTotalCumulativeCost() / getOcc();
    }

    public double getSelfAvgCost() {
        return getTotalSelfCost() / getOcc();
    }

    public double getTotalCumulativeSqrCost() {
        return this.total_sqr_sum_cost;
    }

    public double getTotalSelfSqrCost() {
        return this.total_self_sqr_sum;
    }
    
    public long getSumRms() {
        return this.sum_rms;
    }
    
    public long getSumSqrRms() {
        return this.sum_sqr_rms;
    }
    
    public double getRatioSumRmsRvms() {
        
        if (this.rms > 0)
            return (((double)sum_rms) / ((double)(this.rms * this.occ))); 
        
        return 1;   
    }
    
    public long getSumRvmsSyscall() {
        return rvms_syscall;
    }
    
    public long getSumRvmsThread() {
        return rvms_thread;
    }
    
    public long getSumRvmsSyscallSelf() {
        return rvms_syscall_self;
    }
    
    public long getSumRvmsThreadSelf() {
        return rvms_thread_self;
    }
    
    public double getRatioSumRvmsSyscall() {
         if (this.rvms_syscall > 0)
            return (((double)this.rvms_syscall) / ((double)(this.rms * this.occ))); 
        
        return 0;
    }
    
    public double getRatioSumRvmsThread() {
         if (this.rvms_thread > 0)
            return (((double)this.rvms_thread) / ((double)(this.rms * this.occ))); 
        
        return 0;
    }
}
