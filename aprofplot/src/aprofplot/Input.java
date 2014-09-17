package aprofplot;

import java.util.Comparator;

public class Input implements Comparable<Input>, Cloneable {

    public enum CostKind {
        MIN, AVG, MAX
    }
    
    public enum CostType {
        CUMULATIVE, SELF
    }
    
	private long size;
    
	private double min_cumulative_cost;
	private double max_cumulative_cost; 
	private double sum_cumulative_cost; 
    private double sqr_cumulative_cost;
    
    private double sum_cumulative_real_cost;
    
    private double min_self_cost;
    private double max_self_cost;
    private double sum_self_cost;
    private double sqr_self_cost;
    
	private long calls;
    
    private long sum_cumulative_syscall_input;
    private long sum_cumulative_thread_input;
    private long sum_self_syscall_input;
    private long sum_self_thread_input;
    
    private long amortized_index = -1;

	// pos 0 -> n exponent;
	// pos 1 -> log_e(n) exponent;
	// pos 2 -> log_e(log_e(n)) exponent
	private static double[] ratio_config = {1, 0, 0};

    public static class ComparatorCostInput implements Comparator<Input> {
        @Override
        public int compare(Input t1, Input t2) {
            if (t1.getCost() == t2.getCost()) return 0;
            if (t1.getCost() > t2.getCost()) return 1;
            return -1;
        }
    }
    
    public static class ComparatorAvgCostInput implements Comparator<Input> {
        @Override
        public int compare(Input t1, Input t2) {
            if (t1.getAvgCost() == t2.getAvgCost()) return 0;
            if (t1.getAvgCost() > t2.getAvgCost()) return 1;
            return -1;
        }
    }
    
	public Input(long size, 
                double min_cumulative_cost, 
                double max_cumulative_cost, 
                double sum_cumulative_cost, 
                double sum_cumulative_real_cost, 
                double sum_self_cost, long calls,
                double min_self_cost, double max_self_cost, 
                double sqr_cumulative_cost, double sqr_self_cost, 
                long sum_cumulative_syscall_input, 
                long sum_cumulative_thread_input, 
                long sum_self_syscall_input,
                long sum_self_thread_input) {

        /*
        if (size == 0) 
            throw new RuntimeException("Invalid input size");
        */
        
        if (min_cumulative_cost == 0 || max_cumulative_cost == 0 
                || min_cumulative_cost > max_cumulative_cost) 
            throw new RuntimeException("Invalid min/max cumulative cost");
        
        if (calls == 0)
            throw new RuntimeException("Invalid number of calls");
        
        /*
        if (min_self_cost == 0 || max_self_cost == 0 
                || min_self_cost > max_self_cost) 
            throw new RuntimeException("Invalid min/max self cost");
        */
        
		this.size = size;
        
		this.min_cumulative_cost = min_cumulative_cost;
		this.max_cumulative_cost = max_cumulative_cost;
		this.sum_cumulative_cost = sum_cumulative_cost;
        this.sqr_cumulative_cost = sqr_cumulative_cost;
        
        this.sum_cumulative_real_cost = sum_cumulative_real_cost;
        
        this.min_self_cost = min_self_cost;
		this.max_self_cost = max_self_cost;
        this.sum_self_cost = sum_self_cost;
        this.sqr_self_cost = sqr_self_cost;
        
		this.calls = calls;
		
        this.sum_cumulative_syscall_input = sum_cumulative_syscall_input;
        this.sum_cumulative_thread_input = sum_cumulative_thread_input;
        this.sum_self_syscall_input = sum_self_syscall_input;
        this.sum_self_thread_input = sum_self_thread_input;
        
	}

    long getAmortizedIndex() {
        return amortized_index;
    }
    
    public void setAmortizedIndex(long index) {
        amortized_index = index;
    }
    
	public static double[] getRatioConfig() {
		return ratio_config;
	}

	public static void setRatioConfig(double[] rc) {
		ratio_config = rc;
	}

	public long getSize() {
		return size;
	}

	public double getMinCost() {
        
        if (Main.getChartCostMode() == CostType.CUMULATIVE)
            return getMinCumulativeCost();
        
		return getMinSelfCost();
	}

	public double getMaxCost() {
        
        if (Main.getChartCostMode() == CostType.CUMULATIVE)
    		return getMaxCumulativeCost();
        
        return getMaxSelfCost();
	}

	public double getSumCost() {
        
        if (Main.getChartCostMode() == CostType.CUMULATIVE)
    		return getSumCumulativeCost();
	
        return getSumSelfCost();
    }
    
    public double getSumCumulativeRealCost() {
        
        // old report
        if (sum_cumulative_real_cost == 0 && sum_self_cost == 0)
            return getSumCost();
        
        return sum_cumulative_real_cost;
    }
   
     public double getSumCumulativeCost() {
        return sum_cumulative_cost;
    }
    
    public double getSumSelfCost() {
        return sum_self_cost;
    }
    
	public double getSqrCost() {
        
		if (Main.getChartCostMode() == CostType.CUMULATIVE)
            return getSqrCumulativeCost();
        
        return getSqrSelfCost();
    }

	public double getVar() {
		
        double variance =   ( 
                                (getSqrCost() / getCalls()) 
                                    -
                                (   (getSumCost() / (double)getCalls()) 
                                        * 
                                    (getSumCost() / (double)getCalls())
                                )
                            );
        
        if (variance < 0) 
            throw new RuntimeException("Invalid variance");
        
		return variance;
	}

	public double getAvgCost() {
        
        if (Main.getChartCostMode() == CostType.CUMULATIVE)
    		return getAvgCumulativeCost();
        
		return getAvgSelfCost();
	}

	public double getCost() {
		return getCost(CostKind.MAX);
	}
	
	public double getCost(CostKind type) {
		switch(type) {
			case MAX: return getMaxCost();
			case AVG: return getAvgCost();
			case MIN: return getMinCost();
			default: return 0; 
		}
	}

	public double getRatio() {
		return getRatioCost(CostKind.MAX);
	}

	public double getRatioCost(CostKind type) {
		if (size == 0 || (size == 1 && (ratio_config[1] != 0 || ratio_config[2] != 0)))
			switch(type) {
				case MAX: return getMaxCost();
				case AVG: return getAvgCost();
				case MIN: return getMinCost();
				default: return 0;
			}
		
        double fraction =
				Math.pow(size, ratio_config[0]) *
				Math.pow(Math.log(size), ratio_config[1]) *
				Math.pow(Math.log(Math.log(size)), ratio_config[2]);
		switch(type) {
			case MAX: 
				return (fraction == 0) ? getMaxCost() : getMaxCost() / fraction;
			case AVG: 
				return (fraction == 0) ? getAvgCost() : getAvgCost() / fraction;
			case MIN: 
				return (fraction == 0) ? getMinCost() : getMinCost() / fraction;
			default: return 0;
		}
		
	}

	public double getRatio(int type) {
		double cost = this.getCost();
		switch (type) {
			case 0: return cost / size;
			case 1: if (size <= 1) return cost;
					else return (cost / (size * Math.log(size)));
			case 2: if (size == 0) return cost;
					else return (cost / Math.pow(size, 2));
		}
		return 0;
	}

	public long getCalls() {
		return calls;
	}

	@Override
	public int compareTo(Input t) {
		if (this.size == t.size) return 0;
		if (this.size > t.size) return 1;
		return -1;
	}

	@Override
	public boolean equals(Object o) {
		if (o != null && getClass().equals(o.getClass())) {
			Input te = (Input)o;
			return te.size == size;
		}
		else return false;
	}

	@Override
	public int hashCode() {
		int hash = 7;
		hash = 97 * hash + (int)this.size;
		return hash;
	}

	public boolean merge(Input te) {
		
        if (this.size != te.size) return false;
        
        min_cumulative_cost = Math.min(this.min_cumulative_cost, te.getMinCumulativeCost());
        max_cumulative_cost = Math.max(this.max_cumulative_cost, te.getMaxCumulativeCost());
        sum_cumulative_cost += te.getSumCumulativeCost();
        sqr_cumulative_cost += te.getSqrCumulativeCost();
        
        sum_cumulative_real_cost += te.getSumCumulativeRealCost();
        
        min_self_cost = Math.min(te.getMinSelfCost(), min_self_cost);
        max_self_cost = Math.max(te.getMaxSelfCost(), max_self_cost);
        sum_self_cost += te.getSumSelfCost();
        sqr_cumulative_cost += te.getSqrSelfCost();
        
        sum_cumulative_syscall_input += te.getSumCumulativeSyscallInput();
        sum_cumulative_thread_input += te.getSumCumulativeThreadInput();
        sum_self_syscall_input += te.getSumSelfSyscallInput();
        sum_self_thread_input += te.getSumSelfThreadInput();
        
        calls += te.getCalls();
        
        return true;
	}

    @Override
    public Input clone() throws CloneNotSupportedException {
        return (Input) super.clone();
    }
    
    public double getMinCumulativeCost() {
        return min_cumulative_cost;
    }
    
    public double getMaxCumulativeCost() {
        return max_cumulative_cost;
    }
    
    public double getMinSelfCost() {
        return min_self_cost;
    }

    public double getMaxSelfCost() {
        return max_self_cost;
    }

    public double getAvgCumulativeCost() {
        return getSumCumulativeCost() / getCalls();
    }

    public double getAvgSelfCost() {
        return getSumSelfCost() / getCalls();
    }

    public double getSqrCumulativeCost() {
        return this.sqr_cumulative_cost;
    }

    public double getSqrSelfCost() {
        return this.sqr_self_cost;
    }
    
    public long getSumCumulativeSyscallInput() {
        return sum_cumulative_syscall_input;
    }
    
    public long getSumCumulativeThreadInput() {
        return sum_cumulative_thread_input;
    }
    
    public long getSumSelfSyscallInput() {
        return sum_self_syscall_input;
    }
    
    public long getSumSelfThreadInput() {
        return sum_self_thread_input;
    }
    
    public double getRatioSyscallInput() {
         
        if (sum_cumulative_syscall_input > 0)
            return (((double)sum_cumulative_syscall_input) / ((double)(size * calls))); 
        
        return 0;
    }
    
    public double getRatioThreadInput() {
         
        if (sum_cumulative_thread_input > 0)
            return (((double)sum_cumulative_thread_input) / ((double)(size * calls))); 
        
        return 0;
    }

    @Override
    public String toString() {
        return getSize() + " " + getCalls();
    }
}