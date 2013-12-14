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
    
	private ArrayList<Input> input_tuples = new ArrayList<Input>();
    private SortOrder sort_status = SortOrder.NONE;
    
	// amortized analysis
	private long last_amortized_n = -1;
	private double last_amortized_budget = 0;
	private long last_amortized_index = 0;
    private double amortized_constant = 1.0;
    private Input.CostType chart_cost_mode = Input.CostType.CUMULATIVE;
    
	public Routine() {}

	public abstract int getID();
	public abstract String getImage();
	public abstract String getName();
	public abstract String getFullName();
	public abstract String getMangledName();

	// No duplicated rms!
	public void addInput(Input i) {

		input_tuples.add(i);
		
        // cumulative
		if (i.getMinCumulativeCost() < min_cumulative_cost) 
            min_cumulative_cost = i.getMinCumulativeCost();
		if (i.getMaxCumulativeCost() > max_cumulative_cost) 
            max_cumulative_cost = i.getMaxCumulativeCost();
		if (i.getAvgCumulativeCost() > max_avg_cumulative_cost) 
            max_avg_cumulative_cost = i.getAvgCumulativeCost();
        total_cumulative_cost += i.getSumCumulativeCost();
        
        // self
        if (i.getMinSelfCost() < min_self_cost) 
            min_self_cost = i.getMinSelfCost();
		if (i.getMaxSelfCost() > max_self_cost) 
            max_self_cost = i.getMaxSelfCost();
		if (i.getAvgSelfCost() > max_avg_self_cost) 
            max_avg_self_cost = i.getAvgSelfCost();
		total_self_cost += i.getSumSelfCost();
        
        total_cumulative_real_cost += i.getSumCumulativeRealCost();
        
        if (i.getSize() > max_input) max_input = i.getSize();
		if (i.getSize() < min_input) min_input = i.getSize();
		total_input = i.getSize();
        
		total_calls += i.getCalls();
        
        total_cumulative_syscall_input += i.getSumCumulativeSyscallInput();
        total_cumulative_thread_input += i.getSumCumulativeThreadInput();
        total_self_syscall_input += i.getSumSelfSyscallInput();
        total_self_thread_input += i.getSumSelfThreadInput();
        
		// Invalid amortized cache
		last_amortized_n = -1;
		
		// Set as unsorted
		sort_status = SortOrder.NONE;
	}
    
	public double getMinCost() {
        
        if (Main.getChartCostMode() == Input.CostType.CUMULATIVE)
            return min_cumulative_cost;
		
        return min_self_cost;
	}
	
	public double getMaxCost() {
        
        if (Main.getChartCostMode() == Input.CostType.CUMULATIVE)
            return max_cumulative_cost;
		
        return max_self_cost;
	}

	public double getMaxAvgCost() {
        
        if (Main.getChartCostMode() == Input.CostType.CUMULATIVE)
            return max_avg_cumulative_cost;
		
        return max_avg_self_cost;
	}

	public double getTotalCost() {
        
        if (Main.getChartCostMode() == Input.CostType.CUMULATIVE)
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
	
	public Input getInputTuple(int index) {
		return input_tuples.get(index);
	}
	
	public ArrayList<Input> getInputTuples() {
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
        
		if (Main.getChartCostMode() != this.chart_cost_mode) {
            this.chart_cost_mode = Main.getChartCostMode();
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
			Input s = input_tuples.get((int)i);
			if (s.getSize() > n) {
				i--;
				break;
			}
              
            if (s.getSumCost() < budget) {
                budget -= s.getSumCost();
                am_value = 0;
            } else {
                am_value = ((alpha + 1) * s.getSumCost()) / s.getCalls(); 
                budget = budget + (alpha * s.getSumCost()); 
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
                
                Input s = input_tuples.get(i);
                if (s.getSumCost() < accum) {
                    accum -= s.getSumCost();
                    diff = 0;
                } else {
                    diff = ((alpha + 1) * s.getSumCost()) / s.getCalls(); 
                    accum = accum + (alpha * s.getSumCost()); 
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
        
        Collections.sort(input_tuples, new Comparator<Input> () {
			@Override
			public int compare(Input t1, Input t2) {
				if (t1.getSize() == t2.getSize()) {
                    if (t1.getCalls() == t2.getCalls()) return 0;
                    if (t1.getCalls() > t2.getCalls()) return -1;
                    return 1;
                }
				if (t1.getSize() > t2.getSize()) return 1;
				return -1;
			}
		});
        sort_status = SortOrder.BY_INPUT;
	}

	public void sortInputTuplesByCost() {
        
        if (Main.getChartCostMode() == Input.CostType.CUMULATIVE) {
            
            if (sort_status == SortOrder.BY_CUMULATIVE_COST)
                return;
            else
                sort_status = SortOrder.BY_CUMULATIVE_COST;
        
        } else if (Main.getChartCostMode() == Input.CostType.SELF) {
            
            if (sort_status == SortOrder.BY_SELF_COST)
                return;
            else
                sort_status = SortOrder.BY_SELF_COST;
        }
        
		Collections.sort(input_tuples, new Input.ComparatorCostInput());
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
