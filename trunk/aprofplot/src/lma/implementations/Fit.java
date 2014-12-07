package lma.implementations;

import lma.LMA;
import lma.LMAFunction;

import java.awt.Color;
import java.awt.geom.Rectangle2D;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.lang.Math;
import java.util.Scanner;

import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartFrame;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.ChartUtilities;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYItemRenderer;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;
import org.jfree.ui.RectangleInsets;

public class Fit {
    
	/** Function: y = a0 + a1 * x ^ a2 */
	public static class Function extends LMAFunction {
		@Override
		public double getY(double x, double[] a) {
			return a[0] + a[1] * Math.pow(x, a[2]);
		}
		@Override
		public double getPartialDerivate(double x, double[] a, int parameterIndex) {
			switch (parameterIndex) {
                case 0: return 1;
				case 1: return Math.pow(x, a[2]);
				case 2: return a[1] * Math.pow(x, a[2]) * Math.log(x);
			}
			throw new RuntimeException("No such parameter index: " + parameterIndex);
		}
	}
	
	 public static class DoubleDynamicArray {
		 
		 double[] data = new double[64];
		 int used = 0;
		 
		 boolean contains(double e) {
			 for (int i = 0; i < used; i++)
				 if (data[i] == e) return true;
			 return false;
		 }
		 
		 int size() {
			 return used;
		 }
		 
		 double get(int index) {
			 return data[index];
		 }
		 
		 void add(double e) {
			 
			 if (used >= data.length) {
				 double [] data2 = new double[data.length * 2];
				 System.arraycopy(data, 0, data2, 0, data.length);
				 data = data2;
			 }
			 
			data[used++] = e; 
		 }
	 
		 double[] toArray() {
			 double[] d = new double[used];
			 System.arraycopy(data, 0, d, 0, used);
			 return d;
		 }
	 
	 }
	
	private static void printUsage() {
		System.out.println("Usage: lma <data file>");
	}
	
	public static double[][] parseData(String path) {
		
		DoubleDynamicArray X = new DoubleDynamicArray();
		DoubleDynamicArray Y = new DoubleDynamicArray();
		
		FileInputStream inputStream = null;
		Scanner sc = null;
		try {
		    inputStream = new FileInputStream(path);
		    sc = new Scanner(inputStream);
		    while (sc.hasNextLine()) {
		        String line = sc.nextLine();
		        String[] xy = line.split("\t");
		        X.add(Double.parseDouble(xy[0]));
		        Y.add(Double.parseDouble(xy[1]));
		    }
		    // note that Scanner suppresses exceptions
		    if (sc.ioException() != null) {
		        throw sc.ioException();
		    }
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
		    if (inputStream != null) {
		        try {
					inputStream.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
		    }
		    if (sc != null) {
		        sc.close();
		    }
		}
		
		double[] dx = X.toArray();
		double[] dy = Y.toArray();
		
		return new double[][] { dx, dy };
	}
	
	/** Does the actual fitting by using the above ExampleFunction (a line) */
	public static void main(String[] args) {
        
		if (args.length < 1) {
			printUsage();
			return;
		}
		
		double[][] data = parseData(args[0]);
		
		LMA lma = new LMA(
			new Function(),
            new double[] {1, 1, 1},
			data
		);
		lma.fit();
		//System.out.println("iterations: " + lma.iterationCount);
		//System.out.format("%.2f + %.2f * x ^ %.2f\n%.2f\n", lma.parameters[0],
        //    lma.parameters[1], lma.parameters[2], lma.computeR2()); // "chi2: " + lma.chi2 
		System.out.println(lma.parameters[0] + ", " + lma.parameters[1] + ", " + lma.parameters[2]);
		System.out.println(lma.computeR2());
		System.out.println(lma.iterationCount);
		
		XYSeriesCollection result = new XYSeriesCollection();
	    XYSeries series = new XYSeries("Random");
	    for (int i = 0; i < data[0].length; i++) {
	        series.add(data[0][i], data[1][i]);
	    }
	    result.addSeries(series);
		
		JFreeChart chart = ChartFactory.createScatterPlot(
	            "Scatter Plot", // chart title
	            "X", // x axis label
	            "Y", // y axis label
	            result,
	            PlotOrientation.VERTICAL,
	            false, // include legend
	            false, // tooltips
	            false // urls
	            );

		XYPlot plot = (XYPlot) chart.getPlot();
		plot.setBackgroundPaint(Color.WHITE);
		plot.setAxisOffset(new RectangleInsets(0.0, 0.0, 0.0, 0.0));
		plot.setDomainGridlinePaint(Color.LIGHT_GRAY);
		plot.setRangeGridlinePaint(Color.LIGHT_GRAY);
		
		XYLineAndShapeRenderer renderer = (XYLineAndShapeRenderer) plot.getRenderer();
		renderer.setSeriesShape(0, new Rectangle2D.Double(-2.5, -2.5, 3.0, 3.0));
		renderer.setUseOutlinePaint(false);
		renderer.setDrawOutlines(false);
		
		chart.setAntiAlias(true);
		try {
			ChartUtilities.saveChartAsPNG(new File("soft3d.png"), chart, 600, 500, null, false, 0);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
