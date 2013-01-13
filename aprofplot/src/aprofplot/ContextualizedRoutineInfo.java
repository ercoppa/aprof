package aprofplot;

import java.util.*;

public class ContextualizedRoutineInfo extends RoutineInfo {
 
	// Status flag (useful for GUI, maybe should not be placed here...)
	private boolean collapsed;
	
	// All routine contexts
	private ArrayList<RoutineContext> contexts;
	
	// Lazy add list (used for perfomance reason)
	private ArrayList<Rms> lazy_list;

	public ContextualizedRoutineInfo(int id) {
		super(id, null, null, 0);
		this.contexts = new ArrayList<RoutineContext>();
		this.lazy_list = new ArrayList<Rms>();
		this.collapsed = true;
	}

	public ContextualizedRoutineInfo(int id, String name, String image) {
		super(id, name, image, 0);
		this.contexts = new ArrayList<RoutineContext>();
		this.lazy_list = new ArrayList<Rms>();
		this.collapsed = true;
	}

	public int getContextCount() {
		return contexts.size();
	}

	public void addContext(RoutineContext c) {
		
		contexts.add(c);
		c.setContextId(contexts.size());
		lazy_list.addAll(c.getRmsList());
	
	}
	
	public void addRmsLazy(Rms r) {
		lazy_list.add(r);
	}
	
	public void mergeLazyList() {
		
		if (lazy_list.isEmpty()) return;
		
		// Sort the list
		Collections.sort(lazy_list, new Comparator<Rms> () {
			@Override
			public int compare(Rms t1, Rms t2) {
				if (t1.getRms() == t2.getRms()) return 0;
				if (t1.getRms() > t2.getRms()) return 1;
				return -1;
			}
		});
		
		// Merge duplicates
		Iterator i = lazy_list.iterator();
		long current_rms = lazy_list.get(0).getRms();
		long min_cost = Long.MAX_VALUE;
		long max_cost = 0;
		double total_cost = 0;
        double total_real_cost = 0;
		double total_self = 0;
        long self_min = 0;
        long self_max = 0;
		long occ = 0;
        double cumul_sqr = 0;
        double self_sqr = 0;
        long sum_rms = 0;
        long sum_sqr_rms = 0;
        
		while (i.hasNext()) {
			
			Rms r = (Rms) i.next();
			if (r.getRms() > current_rms) {
				
				Rms rr = new Rms(current_rms, min_cost, max_cost, total_cost,
									total_real_cost, total_self, occ, 
                                    self_min, self_max, cumul_sqr, self_sqr,
                                    sum_rms, sum_sqr_rms);
				
				addRms(rr);
				
				// reset
				min_cost = Long.MAX_VALUE;
				max_cost = 0;
				total_cost = 0;
                total_real_cost = 0;
				total_self = 0;
                self_min = Long.MAX_VALUE;
                self_max = 0;
				occ = 0;
                cumul_sqr = 0;
                self_sqr = 0;
                sum_rms = 0;
                sum_sqr_rms = 0;
				current_rms = r.getRms();
				
			}
			
			// update
			min_cost = (long) Math.min(min_cost, r.getCumulativeMinCost());
			max_cost = (long) Math.max(max_cost, r.getCumulativeMaxCost());
			total_cost += r.getTotalCumulativeCost();
            total_real_cost += r.getTotalRealCost();
			total_self += r.getTotalSelfCost();
            self_min = (long) Math.min(self_min, r.getSelfMinCost());
			self_max = (long) Math.max(self_max, r.getSelfMaxCost());
            occ += r.getOcc();
            cumul_sqr += r.getTotalCumulativeSqrCost();
            self_sqr += r.getTotalSelfSqrCost();
            sum_rms += r.getSumRms();
            sum_sqr_rms += r.getSumSqrRms();
			
		}
		
		Rms rr = new Rms(current_rms, min_cost, max_cost, total_cost,
									total_real_cost, total_self, occ,
                                    self_min, self_max, cumul_sqr, self_sqr,
                                    sum_rms, sum_sqr_rms);
				
		addRms(rr);
		
		// free lazy list
		lazy_list.clear();
		
	}

	public ArrayList<RoutineContext> getContexts() {
		return new ArrayList<RoutineContext>(this.contexts);
	}

	public boolean getCollapsed() {
		return collapsed;
	}

	public void setCollapsed(boolean coll) {
		this.collapsed = coll;
	}

}
