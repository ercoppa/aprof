package aprofplot;

import java.util.*;

public abstract class Routine implements Comparable<Routine> {

	// Some statistics about the routine
    
	private double min_cumulative_cost = Double.MAX_VALUE; 
	private double max_cumulative_cost = 0;  
	private double max_avg_cumulative_cost = 0;
    private double total_cumulative_cost = 0;
    
    private double total_cumulative_real_cost = 0;
    
    private double min_self_cost = Double.MAX_VALUE;
    private double max_self_cost = 0;
    private double max_avg_self_cost = 0;
    private double total_self_cost = 0;
    
	private long total_calls = 0;
    
	private long min_input = Integer.MAX_VALUE;
	private long max_input = 0;
    private long total_input = 0;

    private long total_cumulative_syscall_input = 0;
    private long total_cumulative_thread_input = 0;
    private long total_self_syscall_input = 0;
    private long total_self_thread_input = 0;

    public enum SortOrder {
        NONE, BY_INPUT, BY_CUMULATIVE_COST, BY_SELF_COST
    }
    
	private ArrayList<Rms> input_tuples = new ArrayList<Rms>();
    private SortOrder sort_status = SortOrder.NONE;
    
	// amortized analysis
	private long last_amortized_n = -1;
	private double last_amortized_budget = 0;
	private long last_amortized_index = 0;
    private double amortized_constant = 1.0;
    private int chart_cost = Main.COST_CUMULATIVE;
    
	public Routine() {}

	public abstract int getID();
	public abstract String getImage();
	public abstract String getName();
	public abstract String getFullName();
	public abstract String getMangledName();

	// No duplicated rms!
	public void addInput(Rms i) {

		input_tuples.add(i);
		
        // cumulative
		if (i.getCumulativeMinCost() < min_cumulative_cost) 
            min_cumulative_cost = i.getCumulativeMinCost();
		if (i.getCumulativeMaxCost() > max_cumulative_cost) 
            max_cumulative_cost = i.getCumulativeMaxCost();
		if (i.getCumulativeAvgCost() > max_avg_cumulative_cost) 
            max_avg_cumulative_cost = i.getCumulativeAvgCost();
        total_cumulative_cost += i.getTotalCumulativeCost();
        
        // self
        if (i.getSelfMinCost() < min_self_cost) 
            min_self_cost = i.getSelfMinCost();
		if (i.getSelfMaxCost() > max_self_cost) 
            max_self_cost = i.getSelfMaxCost();
		if (i.getSelfAvgCost() > max_avg_self_cost) 
            max_avg_self_cost = i.getSelfAvgCost();
		total_self_cost += i.getTotalSelfCost();
        
        total_cumulative_real_cost += i.getTotalRealCost();
        
        if (i.getRms() > max_input) max_input = i.getRms();
		if (i.getRms() < min_input) min_input = i.getRms();
		total_input = i.getRms();
        
		total_calls += i.getOcc();
        
        total_cumulative_syscall_input += i.getSumRvmsSyscall();
        total_cumulative_thread_input += i.getSumRvmsThread();
        total_self_syscall_input += i.getSumRvmsSyscallSelf();
        total_self_thread_input += i.getSumRvmsThreadSelf();
        
		// Invalid amortized cache
		last_amortized_n = -1;
		
		// Set as unsorted
		sort_status = SortOrder.NONE;
	}
    
	public double getMinCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            return min_cumulative_cost;
		
        return min_self_cost;
	}
	
	public double getMaxCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            return max_cumulative_cost;
		
        return max_self_cost;
	}

	public double getMaxAvgCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            return max_avg_cumulative_cost;
		
        return max_avg_self_cost;
	}

	public double getTotalCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            return total_cumulative_real_cost;
		
        return total_self_cost;
	}
    
    public double getTotalCumulativeCost() {
        return total_cumulative_real_cost;
    }
    
    public double getTotalSelfCost() {
        return total_self_cost;
    }
    
	public long getTotalCalls() {
		return total_calls;
	}
	
	public long getMaxInput() {
		return max_input;
	}
	
	public long getMinInput() {
		return min_input;
	}

	public int getInputTuplesCount() {
		return input_tuples.size();
	}

	public Iterator getInputTuplesIterator() {
		sortInputTuplesByInput();
		return input_tuples.iterator();
	}
	
	public Rms getInputTuple(int index) {
		return input_tuples.get(index);
	}
	
	public ArrayList<Rms> getInputTuples() {
		return input_tuples;
	}

	@Override
	public int compareTo(Routine r) {
		if (total_cumulative_real_cost == r.getTotalCost()) return 0;
		if (total_cumulative_real_cost > r.getTotalCost()) return 1;
		return -1;
	}
	
    public void invalidAmortizedCache() {
        last_amortized_n = -1;
    }
    
    public void setAmortizedConstant(double alpha) {
        amortized_constant = alpha;
    }
    
    public double getAmortizedConstant() {
        return amortized_constant;
    }
    
	public double getAmortizedValue(long n) {
		
		if (sort_status != SortOrder.BY_INPUT) 
            sortInputTuplesByInput();
        
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
        
		for(; i < input_tuples.size(); i++) {
			Rms s = input_tuples.get((int)i);
			if (s.getRms() > n) {
				i--;
				break;
			}
              
            if (s.getTotalCost() < budget) {
                budget -= s.getTotalCost();
                am_value = 0;
            } else {
                am_value = ((alpha + 1) * s.getTotalCost()) / s.getOcc(); 
                budget = budget + (alpha * s.getTotalCost()); 
            }
		}
		
		// Update cache
		last_amortized_budget = budget;
		last_amortized_n = n;
		last_amortized_index = i;

        return am_value;
	}
    
    public ArrayList<Double> estimateAmortizedConstant() {
        
        ArrayList<Double> list = new ArrayList<Double>();
        
        if (sort_status != SortOrder.BY_INPUT) 
            sortInputTuplesByInput();
        
        double alpha = 0;
        int round = 0;
        while (round < 10000) {

            double accum = 0;
            double diff = 0;
            double count = 0;
            for(int i = 0; i < input_tuples.size(); i++) {
                
                Rms s = input_tuples.get(i);
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

	public void sortInputTuplesByInput() {
		
		if (sort_status == SortOrder.BY_INPUT) return;
		Collections.sort(input_tuples, new Comparator<Rms> () {
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
        sort_status = SortOrder.BY_INPUT;
	}

	public void sortRmsListByCost() {
        
        if (Main.getChartCost() == Main.COST_CUMULATIVE) {
            
            if (sort_status == SortOrder.BY_CUMULATIVE_COST)
                return;
            else
                sort_status = SortOrder.BY_CUMULATIVE_COST;
        
        } else if (Main.getChartCost() == Main.COST_SELF) {
            
            if (sort_status == SortOrder.BY_SELF_COST)
                return;
            else
                sort_status = SortOrder.BY_SELF_COST;
        
        }
        
		Collections.sort(input_tuples, new Comparator<Rms> () {
			@Override
			public int compare(Rms t1, Rms t2) {
				if (t1.getCost() == t2.getCost()) return 0;
				if (t1.getCost() > t2.getCost()) return 1;
				return -1;
			}
		});
	}
    
    public double getRatioSyscallInput() {
        
         if (total_input > 0)
            return (((double)total_cumulative_syscall_input) / ((double)total_input)); 
        
        return 0;
    }
    
    public double getRatioThreadInput() {
        
         if (total_input > 0) {
             return (((double)total_cumulative_thread_input) / ((double)total_input)); 
         }
            
        return 0;
    }
    
    public double getTotalSelfSyscallInput() {
        return this.total_self_syscall_input;
    }
    
    public double getTotalSelfThreadInput() {            
        return this.total_self_thread_input;
    }
    
    // deprecated
    public double getRatioSumRmsRvms() { return 0; }
    public double getRatioRvmsRms() { return 0; }
    public double getRatioInducedAccesses() { return 0; } 
}
