package aprofplot;

public class Rms implements Comparable<Rms> {

	private int rms;
	private long min_cost;
	private long max_cost;
	private double total_cost;
	private double sqr_total_cost;
	private long occ;
	
	public static final int MAX_COST = 0;
	public static final int AVG_COST = 1;
	public static final int MIN_COST = 2;

	private double ratio;

	// pos 0 -> n exponent;
	// pos 1 -> log_e(n) exponent;
	// pos 2 -> log_e(log_e(n)) exponent
	private static double[] ratio_config = {1, 0, 0};

	public Rms(int rms, long min_cost,
			long max_cost, double total_cost, double sqr_total_cost, long occ) {

		this.rms = rms;
		this.min_cost = min_cost;
		this.max_cost = max_cost;
		this.total_cost = total_cost;
		this.sqr_total_cost = sqr_total_cost;
		this.occ = occ;
		
		if (rms > 0) this.ratio = this.max_cost / this.rms;
		else this.ratio = this.max_cost;
	
	}

	public static double[] getRatioConfig() {
		return ratio_config;
	}

	public static void setRatioConfig(double[] rc) {
		ratio_config = rc;
	}

	public int getRms() {
		return rms;
	}

	public double getMinCost() {
		return min_cost;
	}

	public double getMaxCost() {
		return max_cost;
	}

	public double getTotalCost() {
		return total_cost;
	}

	public double getSqrTotalCost() {
		return sqr_total_cost;
	}

	public double getVar() {
		double variance = ( 
					( getSqrTotalCost() / getOcc() ) -
					((getTotalCost() / (double)getOcc()) * (getTotalCost() / (double)getOcc()))
				);
		return variance;
	}

	public double getAvgCost() {
		return (double) total_cost / occ;
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
		hash = 97 * hash + this.rms;
		return hash;
	}

	public Rms mergeWith(Rms te) {
		if (this.rms != te.rms) return null;
		return new Rms(
				this.rms,
				Math.min(this.min_cost, te.min_cost),
				Math.max(this.max_cost, te.max_cost), 
				this.total_cost + te.getTotalCost(),
				this.sqr_total_cost + te.getSqrTotalCost(),
				this.occ + te.getOcc());
	}
}
