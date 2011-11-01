package aprofplot;

import aprofplot.*;
import java.io.*;
import java.util.*;

public class extractData {
	
	/* How many point at least need a routine in order to be considered rich? */
	public final static int MIN_POINT_RICH = 10; 
	
	/* Max cost (2^exp) for a poor routine */
	public final static int MAX_EXP_COST_POOR = 64;
	
	public static void main(String[] args) {
		
		if (args.length < 1) {
			System.out.println("You need to pass the name of the report...");
			return;
		}
		System.out.println("Opening report " + args[0] +  "...");
		
		AprofReport report = null;
		File file = new File(args[0]);
		try {
			report = new AprofReport(file);
		} catch (Exception e) {
			System.out.println("Problem during opening the report: " + args[0]);
			return;
		}
		
		int count_rich = 0;
		int count_poor = 0;
		int[] poor_avg_cost = new int[MAX_EXP_COST_POOR];
		int[] poor_max_cost = new int[MAX_EXP_COST_POOR];
		int over_max = 0;
		int over_avg = 0;
		
		System.out.println("Start collecting stats about rich/poor routines");
		
		ArrayList<RoutineInfo> rr = report.getRoutines();
		for (int i = 0; i < rr.size(); i++) {
			
			RoutineInfo r = rr.get(i);
			if (r.getSizeTimeEntries() >= 10) count_rich++;
			else {
				
				count_poor++;
				int index_max = (int) (Math.log(r.getMaxTime()) / Math.log(2));
				int index_avg = (int) (Math.log(r.getMaxAvgCost()) / Math.log(2));
				
				if (index_max < 0) index_max = 0;
				if (index_avg < 0) index_avg = 0;
				
				//System.out.println("Index max " + index_max);
				//System.out.println("Index avg " + index_avg);
				
				if (index_max >= MAX_EXP_COST_POOR) over_max++;
				else poor_max_cost[index_max]++;
				
				if (index_avg >= MAX_EXP_COST_POOR) over_avg++;
				else poor_avg_cost[index_avg]++;
				
			}
		}
		
		if (over_max > 0 || over_avg > 0) {
			System.out.println("Some poor routine has an higher cost than allowed");
			return;
		}
		
		try {
			File tmp = new File(report.getName() + ".poor_max");
			tmp.createNewFile();
			PrintWriter out = new PrintWriter(new FileWriter(tmp));
			
			double acc = 0;
			for (int i = 0; i < poor_max_cost.length; i++) {
				out.print((int) Math.pow(2, i));
				double perc = 100 * ((double)poor_max_cost[i] / (double)count_poor);
				//System.out.println("Perc " + perc + " count " + poor_max_cost[i]);
				acc += perc;
				out.format(" %.1f%n", acc);
				
				if (acc >= 99.99) break;
			}
			
			out.close();
		} catch (java.io.IOException eio) {
			System.out.println("Error during handling file: " + report.getName() + ".poor_max");
			return;
		}
		
		try {
			
			File tmp = new File(report.getName() + ".poor_avg");
			tmp.createNewFile();
			PrintWriter out = new PrintWriter(new FileWriter(tmp));
			
			double acc = 0;
			for (int i = 0; i < poor_avg_cost.length; i++) {
				out.print((int) Math.pow(2, i));
				double perc = 100 * ((double)poor_max_cost[i] / (double)count_poor);
				acc += perc;
				out.format(" %.1f%n", acc);
				
				if (acc >= 99.99) break;
			}
			
			out.close();
		} catch (java.io.IOException eio) {
			System.out.println("Error during handling file: " + report.getName() + ".poor_avg");
			return;
		}
		
		System.out.println("Rich functions are " +  count_rich + " over " + rr.size());
		double perc = 100 * ((double)count_rich / (double) rr.size());
		System.out.format("Rich functions are %.1f%%%n", perc);
		System.out.println("Poor functions are " +  count_poor);
		
		try {
			
			File tmp = new File(report.getName() + ".rich");
			tmp.createNewFile();
			PrintWriter out = new PrintWriter(new FileWriter(tmp));
			out.format("%.1f%n", perc);
			out.close();
		} catch (java.io.IOException eio) {
			System.out.println("Error during handling file: " + report.getName() + ".rich");
			return;
		}
		
	}
	
}
