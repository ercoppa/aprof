package aprofplot.gui;

import java.util.Calendar;
import java.util.Hashtable;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.event.ChartProgressEvent;
import org.jfree.chart.event.ChartProgressListener;

public class PerfomanceMonitor implements ChartProgressListener {
	
	public static final int DRAW = 1;
	public static final int ELABORATE = 2;
	Hashtable<Object, Long> times_draw = new Hashtable<Object, Long>();
	Hashtable<Object, Long> times_elab = new Hashtable<Object, Long>();
	
	public void start(Object j, int op) {
		
		Calendar cal = Calendar.getInstance();
		if (op == DRAW)
			times_draw.put(j, cal.getTimeInMillis());
		else if (op == ELABORATE)
			times_elab.put(j, cal.getTimeInMillis());
		
	}
	
	public void stop(Object j, int op) {
	
		Long t = null;
		try {
			
			if (op == DRAW)
				t = (Long) times_draw.get(j);
			else if (op == ELABORATE)
				t = (Long) times_elab.get(j);
			
			if (t == null) {
			
				 System.out.println("Graph time never initialized [1]");
				 return;
				 
			}
				 
		} catch(NullPointerException e) {
			
			System.out.println("Graph time never initialized [2]");
			return;
			
		}
		
		
		Calendar cal = Calendar.getInstance();
		long f = cal.getTimeInMillis();
		
		if (op == DRAW) {
			
			System.out.println("Draw of graph required: " + (f-t) + "ms");
			times_draw.remove(j);
			
		} else if (op == ELABORATE) {
		
			System.out.println("Elaboration of graph required: " + (f-t) + "ms");
			times_elab.remove(j);
		
		}
		
	}
	
	@Override
	public void chartProgress(ChartProgressEvent cpe) {
	
		if (cpe.getType() == ChartProgressEvent.DRAWING_FINISHED)
			stop(cpe.getChart(), DRAW);
		else if (cpe.getType() == ChartProgressEvent.DRAWING_STARTED)
			start(cpe.getChart(), DRAW);
	
	}
	
}
