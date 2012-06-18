package aprofplot;

import java.util.*;

public abstract class Routine implements Comparable<Routine> {

	// Some statistics about the routine
	private double min_cost;
	private double max_cost;
	private double max_avg_cost;
	private double total_cost;
	private long total_calls;
	private long min_rms;
	private long max_rms;
	// Read memory size elements for this routine
	private ArrayList<Rms> rms_list;
	
	// Mcc cache hack
	private long last_mcc_n = -1;
	private double last_mcc_est = 0;
	private double last_mcc_occ = 0;
	private long last_mcc_index = 0;
	
	// Sort rms list status
	public final static int UNSORTED = 0;
	public final static int SORT_BY_ACCESS = 1;
	public final static int SORT_OTHER = 2;
	private int sort_status = UNSORTED;
	
	public Routine() {
		
		min_cost = Long.MAX_VALUE;
		max_cost = 0;
		total_cost = 0;
		total_calls = 0;
		min_rms = Integer.MAX_VALUE;
		max_rms = 0;
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
		
		if (r.getCost() < min_cost) min_cost = r.getCost();
		if (r.getCost() > max_cost) max_cost = r.getCost();
		if (r.getAvgCost() > max_avg_cost) max_avg_cost = r.getAvgCost();
		if (r.getRms() > max_rms) max_rms = r.getRms();
		if (r.getRms() < min_rms) min_rms = r.getRms();
		
		total_cost += r.getTotalCost();
		total_calls += r.getOcc();
	
		// Invalid mcc cache
		last_mcc_n = -1;
		
		// Set as unsorted
		sort_status = UNSORTED;
		
	}

	public double getMinCost() {
		return min_cost;
	}
	
	public double getMaxCost() {
		return max_cost;
	}

	public double getMaxAvgCost() {
		return max_avg_cost;
	}

	public double getTotalCost() {
		return total_cost;
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
	
	public double getMcc(long n) {
		
		if (sort_status != SORT_BY_ACCESS) sortRmsListByAccesses();
		
		double est = 0;
		double sum_occ = 0;
		long i = 0;
		
		// Use cache if possible...
		if (last_mcc_n != -1 && n >= last_mcc_n) {
			est = last_mcc_est;
			sum_occ = last_mcc_occ;
			i = last_mcc_index + 1;
		}
		
		for(; i < rms_list.size(); i++) {
			Rms s = rms_list.get((int)i);
			if (s.getRms() > n) {
				i--;
				break;
			}
			est += s.getTotalCost();
			sum_occ += s.getOcc();
		}
		
		// Update cache
		last_mcc_est = est;
		last_mcc_occ = sum_occ;
		last_mcc_n = n;
		last_mcc_index = i;
		
		return est / sum_occ;
	}

	public void sortRmsListByAccesses() {
		
		if (sort_status == SORT_BY_ACCESS) return;
		sort_status = SORT_BY_ACCESS;
		Collections.sort(rms_list, new Comparator<Rms> () {
			@Override
			public int compare(Rms t1, Rms t2) {
				if (t1.getRms() == t2.getRms()) return 0;
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
	
}
