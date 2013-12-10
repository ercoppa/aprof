package aprofplot;

import java.util.*;

public abstract class Routine implements Comparable<Routine> {

	// Some statistics about the routine
	private double min_cost; // cumulative
	private double max_cost; // cumulative
	private double max_avg_cost; // cumulative
    
    private double total_cumulative_cost;
    private double total_cost;
    private double total_self;
    private double self_min; 
    private double self_max_avg;
    private double self_max;
	private long total_calls;
	private long min_rms;
	private long max_rms;
    private long sum_rms;
    private long sum_rvms;
    private long rvms_syscall;
    private long rvms_thread;
    private long rvms_syscall_self;
    private long rvms_thread_self;
    private long num_rms;
	// Read memory size elements for this routine
	private ArrayList<Rms> rms_list;
	
	// amortized cache hack
    private int chart_cost = Main.COST_CUMULATIVE;
	private long last_amortized_n = -1;
	private double last_amortized_budget = 0;
	private long last_amortized_index = 0;
	
	// Sort rms list status
	public final static int UNSORTED = 0;
	public final static int SORT_BY_ACCESS = 1;
	public final static int SORT_OTHER = 2;
	private int sort_status = UNSORTED;
	     
    private double amortized_constant = 1.0;
    
	public Routine(long num_rms) {
		
		min_cost = Double.MAX_VALUE;
		max_cost = 0;
        max_avg_cost = 0;
		total_calls = 0;
		min_rms = Integer.MAX_VALUE;
		max_rms = 0;
        self_min = Double.MAX_VALUE;
        self_max = 0;
        self_max_avg = 0;
        this.num_rms = num_rms;
        total_cumulative_cost = 0;
		rms_list = new ArrayList<Rms>();
	
	}

	public abstract int getID();
	public abstract String getImage();
	public abstract String getName();
	public abstract String getFullName();
	public abstract String getMangledName();

	// No duplicated rms!
	public void addRms(Rms r) {

		rms_list.add(r);
		
		if (r.getCumulativeMinCost() < min_cost) min_cost = r.getCumulativeMinCost();
		if (r.getCumulativeMaxCost() > max_cost) max_cost = r.getCumulativeMaxCost();
		if (r.getCumulativeAvgCost() > max_avg_cost) max_avg_cost = r.getCumulativeAvgCost();
        
        if (r.getSelfMinCost() < self_min) self_min = r.getSelfMinCost();
		if (r.getSelfMaxCost() > self_max) self_max = r.getSelfMaxCost();
		if (r.getSelfAvgCost() > self_max_avg) self_max_avg = r.getSelfAvgCost();
		
        if (r.getRms() > max_rms) max_rms = r.getRms();
		if (r.getRms() < min_rms) min_rms = r.getRms();
		
        total_cumulative_cost += r.getTotalCumulativeCost();
        total_cost += r.getTotalRealCost();
        total_self += r.getTotalSelfCost();
		total_calls += r.getOcc();
	
        sum_rvms += r.getOcc() * r.getRms();
        sum_rms += r.getSumRms();
        
        rvms_syscall += r.getSumRvmsSyscall();
        rvms_thread += r.getSumRvmsThread();
        
        rvms_syscall_self += r.getSumRvmsSyscallSelf();
        rvms_thread_self += r.getSumRvmsThreadSelf();
        
		// Invalid amortized cache
		last_amortized_n = -1;
		
		// Set as unsorted
		sort_status = UNSORTED;
		
	}
    
	public double getMinCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            return min_cost;
		
        return self_min;
	}
	
	public double getMaxCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            return max_cost;
		
        return self_max;
	}

	public double getMaxAvgCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            return max_avg_cost;
		
        return self_max_avg;
	}

	public double getTotalCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            return total_cost;
		
        return total_self;
	}
    
    public double getTotalCumulativeCost() {
        return total_cost;
    }
    
    public double getTotalSelfCost() {
        return total_self;
    }
    
	public long getTotalCalls() {
		return total_calls;
	}
	
	public long getMaxRms() {
		return max_rms;
	}
	
	public long getMinRms() {
		return min_rms;
	}

	public int getSizeRmsList() {
		return rms_list.size();
	}

	public Iterator getRmsListIterator() {
		sortRmsListByAccesses();
		return new RmsIterator(rms_list.iterator());
	}
	
	public Rms getRmsItem(int index) {
		return rms_list.get(index);
	}
	
	public ArrayList<Rms> getRmsList() {
		return rms_list;
	}

	@Override
	public int compareTo(Routine r) {
		if (total_cost == r.getTotalCost()) return 0;
		if (total_cost > r.getTotalCost()) return 1;
		return -1;
	}
	
    public void InvalidAmortizedCache() {
        last_amortized_n = -1;
    }
    
    public void setAmortizedConstant(double alpha) {
        this.amortized_constant = alpha;
    }
    
    public double getAmortizedConstant() {
        return this.amortized_constant;
    }
    
	public double getAmortizedValue(long n) {
		
		if (sort_status != SORT_BY_ACCESS) sortRmsListByAccesses();
		if (Main.getChartCost() != this.chart_cost) {
            this.chart_cost = Main.getChartCost();
            last_amortized_n = -1;
        }
        
		long i = 0;
		double am_value = 0;
        double budget = 0;
        double alpha = getAmortizedConstant();
        
		// Use cache if possible...
		if (last_amortized_n != -1 && n >= last_amortized_n) {
			budget = last_amortized_budget;
			i = last_amortized_index + 1;
		}
        
		for(; i < rms_list.size(); i++) {
			Rms s = rms_list.get((int)i);
			if (s.getRms() > n) {
				i--;
				break;
			}
              
            if (s.getTotalCost() < budget) {
                budget -= s.getTotalCost();
                am_value = 0;
            } else {
                am_value = ((alpha + 1) * s.getTotalCost()) / s.getOcc();;  
                budget = budget + (alpha * s.getTotalCost()); 
            }
		}
		
		// Update cache
		last_amortized_budget = budget;
		last_amortized_n = n;
		last_amortized_index = i;

        return am_value;
		//return est / sum_occ;
	}
    
    public ArrayList<Double> estimateAmortizedConstant() {
        
        ArrayList<Double> list = new ArrayList<Double>();
        
        if (sort_status != SORT_BY_ACCESS) 
            sortRmsListByAccesses();
        
        double alpha = 0;
        int round = 0;
        while (round < 10000) {

            double accum = 0;
            double diff = 0;
            double count = 0;
            for(int i = 0; i < rms_list.size(); i++) {
                
                Rms s = rms_list.get(i);
                if (s.getTotalCost() < accum) {
                    accum -= s.getTotalCost();
                    diff = 0;
                } else {
                    diff = ((alpha + 1) * s.getTotalCost()) / s.getOcc(); 
                    accum = accum + (alpha * s.getTotalCost()); 
                }
                
                if (i > 0 && diff > 0) count++;
                
            }
            
            list.add(alpha);
            list.add(count);
            
            if (count <= 0) break;
            
            if (alpha == 0) alpha = 0.001;
            else alpha = alpha * 1.03;
            
            round++;
        }
        
        return list;
    }

	public void sortRmsListByAccesses() {
		
		if (sort_status == SORT_BY_ACCESS) return;
		sort_status = SORT_BY_ACCESS;
		Collections.sort(rms_list, new Comparator<Rms> () {
			@Override
			public int compare(Rms t1, Rms t2) {
				if (t1.getRms() == t2.getRms()) {
                    if (t1.getOcc() == t2.getOcc()) return 0;
                    if (t1.getOcc() > t2.getOcc()) return -1;
                    return 1;
                }
				if (t1.getRms() > t2.getRms()) return 1;
				return -1;
			}
		});
	
	}

	public void sortRmsListByCost() {
		sort_status = SORT_OTHER;
		Collections.sort(rms_list, new Comparator<Rms> () {
			@Override
			public int compare(Rms t1, Rms t2) {
				if (t1.getCost() == t2.getCost()) return 0;
				if (t1.getCost() > t2.getCost()) return 1;
				return -1;
			}
		});
	}

	public void sortRmsListByRatio() {
		sort_status = SORT_OTHER;
		Collections.sort(rms_list, new Comparator<Rms> () {
			@Override
			public int compare(Rms t1, Rms t2) {
				if (t1.getRatio() == t2.getRatio()) return 0;
				if (t1.getRatio() > t2.getRatio()) return 1;
				return -1;
			}
		});
	}

	public void sortRmsListByRatio(int type) {
		sort_status = SORT_OTHER;
		final int t = type;
		Collections.sort(rms_list, new Comparator<Rms> () {
			@Override
			public int compare(Rms t1, Rms t2) {
				if (t1.getRatio(t) == t2.getRatio(t)) return 0;
				if (t1.getRatio(t) > t2.getRatio(t)) return 1;
				return -1;
			}
		});
	}

	public void sortRmsListByOccurrences() {
		sort_status = SORT_OTHER;
		Collections.sort(rms_list, new Comparator<Rms> () {
			@Override
			public int compare(Rms t1, Rms t2) {
				if (t1.getOcc() == t2.getOcc()) return 0;
				if (t1.getOcc() > t2.getOcc()) return 1;
				return -1;
			}
		});
	}
	
	public void sortRmsListByVar() {
		sort_status = SORT_OTHER;
		Collections.sort(rms_list, new Comparator<Rms> () {
			@Override
			public int compare(Rms t1, Rms t2) {
				if (t1.getVar() == t2.getVar()) return 0;
				if (t1.getVar() > t2.getVar()) return 1;
				return -1;
			}
		});
	}
	
	public void sortRmsListByTotalCost() {
		sort_status = SORT_OTHER;
		Collections.sort(rms_list, new Comparator<Rms> () {
			@Override
			public int compare(Rms t1, Rms t2) {
				if (t1.getTotalCost() == t2.getTotalCost()) return 0;
				if (t1.getTotalCost() > t2.getTotalCost()) return 1;
				return -1;
			}
		});
	}

    public double sumRms() {
        return sum_rms;
    }
    
    public double sumRvms() {
        return sum_rvms;
    }
    
    public double getRatioSumRmsRvms() {
        
        if (sum_rms > 0 && sum_rvms == 0)
            throw new RuntimeException("Invalid RVMS");
        else if (sum_rms == 0 && sum_rvms == 0)
            return 1;
           
        if (this.getName().equals("diff"))
            System.out.println("sum rms: " + sum_rms + "  sum rvms: " + sum_rvms);
            
        return (((double) sum_rms) / ((double) sum_rvms));
    }
    
    public void setCountRms(long distinct_rms) {
        this.num_rms = distinct_rms;
    }
	
    public long getCountRms() {
        return num_rms;
    }
    
    public double getRatioRvmsRms() {
        
        if (getCountRms() == 0) 
            return getSizeRmsList();
        
        if (getName().equals("GOMP_taskwait"))
            System.out.println("#rms " + getCountRms() +
                                    " #rvms " + getSizeRmsList() +
                                    " ratio " + (((double)getSizeRmsList() - getCountRms()) /
                ((double)getCountRms()))
                        );
        
        return (((double)getSizeRmsList() - getCountRms()) /
                ((double)getCountRms()));
    }
    
    public double getRatioSumRvmsSyscall() {
        
         if (this.rvms_syscall > 0)
            return (((double)this.rvms_syscall) / ((double)this.sum_rvms)); 
        
        return 0;
    }
    
    public double getRatioSumRvmsThread() {
        
         if (this.rvms_thread > 0) {
             return (((double)this.rvms_thread) / ((double)this.sum_rvms)); 
         }
            
        return 0;
    }
    
    public double getSumRvmsSyscallSelf() {
        return this.rvms_syscall_self;
    }
    
    public double getSumRvmsThreadSelf() {            
        return this.rvms_thread_self;
    }
    
    public double getRatioInducedAccesses() {
    
        if (this.sum_rvms <= 0) return 0;
        
        return (((double)this.rvms_thread + this.rvms_syscall) 
                    / ((double)this.sum_rvms)); 
        
    }
    
}
