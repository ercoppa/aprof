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
        NONE, BY_INPUT, BY_CUMULATIVE_COST, BY_SELF_COST, BY_AVG_COST,
    }
    
	private final ArrayList<Input> input_tuples = new ArrayList<Input>();
    private SortOrder sort_status = SortOrder.NONE;
    
	// amortized analysis
	private long amortized_cut_index = -1;
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
		total_input += i.getSize() * i.getCalls();
        
		total_calls += i.getCalls();
        
        total_cumulative_syscall_input += i.getSumCumulativeSyscallInput();
        total_cumulative_thread_input += i.getSumCumulativeThreadInput();
        total_self_syscall_input += i.getSumSelfSyscallInput();
        total_self_thread_input += i.getSumSelfThreadInput();
            
		// Set as unsorted
		sort_status = SortOrder.NONE;
	}
    
    private void computeAmortizedCutIndex() {
        
        if (sort_status != SortOrder.BY_AVG_COST) 
            sortInputTuplesByAvgCost();
        
        long index = 0;
        long total_cost = 0;
        
        for (Input i : input_tuples) {
            total_cost += i.getSumCost();
            i.setAmortizedIndex(index++);
        }
        
        
        double sum_lower = 0;
        double sum_upper = total_cost;
        /*
        for (Input i : input_tuples) {
            
            sum_lower += i.getSumCost();
            sum_upper -= i.getSumCost();

            if (sum_lower * amortized_constant >= sum_upper) {
                amortized_cut_index = i.getAmortizedIndex();
                //System.out.println("Computed cut index is: " + amortized_cut_index);
                break;
            }
        }
        */
        
        sum_upper = total_cost;
        sum_lower = 0;
        
        double min_charge = Double.MAX_VALUE; 
        double min_alpha = Double.MAX_VALUE; 
        long min_rms = Long.MAX_VALUE;
        long cut_index = Long.MAX_VALUE;
        for (Input i : input_tuples) {
            
            sum_lower += i.getSumCost();
            sum_upper -= i.getSumCost();
            
            double alpha = sum_upper / sum_lower;
            
            if (alpha > 2) continue;
            
            double charge = (1 + alpha) * i.getAvgCost();
            
            if (charge < min_charge) {
                min_charge = charge;
                min_alpha = alpha;
                min_rms = i.getSize();
                cut_index = i.getAmortizedIndex();
            }
        }
        
        amortized_cut_index = cut_index;
        amortized_constant = min_alpha;
        
        /*
        System.out.println("Min alpha is: " + min_alpha);
        System.out.println("Given by rms: " + min_rms);
        System.out.println("Charge is: " + min_charge);
        */
    }

    private void sortInputTuplesByAvgCost() {
        
        if (Main.getChartCostMode() == Input.CostType.CUMULATIVE) {
            
            if (sort_status == SortOrder.BY_AVG_COST)
                return;
            else
                sort_status = SortOrder.BY_AVG_COST;
        
        } else if (Main.getChartCostMode() == Input.CostType.SELF) {
            
            if (sort_status == SortOrder.BY_AVG_COST)
                return;
            else
                sort_status = SortOrder.BY_AVG_COST;
        }
        
		Collections.sort(input_tuples, new Input.ComparatorAvgCostInput());
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
        amortized_cut_index = -1;
    }
    
    public void setAmortizedConstant(double alpha) {
        amortized_constant = alpha;
    }
    
    public double getAmortizedConstant() {
        return amortized_constant;
    }
    
    public double getCostBalancePoint() {
        
        if (input_tuples.size() <= 1)
            return Double.NaN;
        
        if (sort_status != SortOrder.BY_INPUT) 
            sortInputTuplesByInput();
        
        double sum_lower = 0;
        double sum_upper = 0;
        
        for(Input s : input_tuples)
            sum_upper += s.getSumCost();

        for(Input s : input_tuples) {
            
            sum_lower += s.getSumCost();
            sum_upper -= s.getSumCost();
            
            double alpha = sum_upper / sum_lower;
            if (alpha <= 1.0) {
                
                double size_t = s.getSize();
                double size_max = input_tuples.get(input_tuples.size() - 1).getSize();
                double size_min = input_tuples.get(0).getSize();
                    
                return 100 * ((size_t - size_min) / (size_max - size_min));
            }
        }
    
        throw new RuntimeException("Cost Balance Point not found for routine " 
                                        + getName());
    }
    
	public double getAmortizedValue(Input i) {
		
        if (amortized_cut_index < 0)
            computeAmortizedCutIndex();

        if (i.getAmortizedIndex() > amortized_cut_index) {
            return 0;
        }
            
        return (1 + amortized_constant) * i.getAvgCost();
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
