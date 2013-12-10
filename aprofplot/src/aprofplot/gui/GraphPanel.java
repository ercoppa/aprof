package aprofplot.gui;

import aprofplot.*;
import aprofplot.jfreechart.SamplingXYLineAndShapeRenderer;
import java.awt.*;
import java.awt.geom.Rectangle2D;
import java.util.*;
import org.jfree.chart.*;
import org.jfree.chart.axis.*;
import org.jfree.chart.plot.*;
import org.jfree.chart.renderer.xy.*;
import org.jfree.chart.title.*;
import org.jfree.data.xy.*;
import org.jfree.ui.RectangleEdge;
import org.jfree.ui.RectangleInsets;

public class GraphPanel extends javax.swing.JPanel {

	// type of graph
	public static final int COST_PLOT = 0;
	public static final int RATIO_PLOT = 1;
	public static final int FREQ_PLOT = 2;
	public static final int MMM_PLOT = 3;
	public static final int TOTALCOST_PLOT = 4;
	public static final int VAR_PLOT = 5;
	public static final int RTN_PLOT = 6;
	public static final int AMORTIZED_PLOT = 7;
    public static final int RATIO_TUPLES_PLOT = 8;
    public static final int EXTERNAL_INPUT_PLOT = 9;
    public static final int ALPHA_PLOT = 10;
	public static final int MAX_VAL_PLOT = 11; // Max value

	// Possible cost used by a graph
	public static final int MAX_COST = 0;
	public static final int AVG_COST = 1;
	public static final int MIN_COST = 2;
	
	// Status of graph
	public static final int MAXIMIZED = 0;
	public static final int ZOOMED = 1;

	// Performance monitor
	PerformanceMonitor perf = null;
	
	private int graph_type;
	private int cost_type;
	private MainWindow main_window;
	private boolean x_log_scale = false;
	private boolean y_log_scale = false;
	private int status = MAXIMIZED;
	private boolean zoomed = false;
	private boolean dragging = false;
	private int group_threshold_base = 1;
	private int group_threshold = 1;
	private int smooth_threshold_base = 1;
	private int smooth_threshold = 1;
	private double min_x = 0, min_y = 0;
	private double max_x = 10, max_y = 10;
	private String[] filters;
	private Routine rtn_info = null;
    
	// Temp global vars
	private int elem_slot = 0, slot_start = 0;
	private double sum_y = 0, sum_y2 = 0, sum_y3 = 0, sum_x = 0;
	private long sum_occ = 0;
	
	// Graphics
	private XYPlot plot;
	private JFreeChart chart;
	private NumberAxis rangeAxis, domainAxis;
	private XYItemRenderer renderer;
	private ChartPanel chartPanel;
	private XYSeriesCollection data;
	private XYSeries[] series;
	// Some colors: from blue to red
	private java.awt.Color[] colors = new java.awt.Color[] { new java.awt.Color(0, 85, 255),
															 new java.awt.Color(0, 170, 255),
															 new java.awt.Color(0, 213, 255),
															 new java.awt.Color(0, 255, 234),
															 new java.awt.Color(0, 255, 161),
															 new java.awt.Color(190, 235, 62),
															 new java.awt.Color(232, 253, 2),
															 new java.awt.Color(255, 242, 0),
															 new java.awt.Color(255, 255, 0),
															 new java.awt.Color(255, 191, 0),
															 new java.awt.Color(255, 145, 0),
															 new java.awt.Color(255, 64, 0)
															};

	public GraphPanel(int graph_type, MainWindow mw) {
		
		this.graph_type = graph_type;
		this.main_window = mw;
		this.filters = main_window.getRmsTableFilter();

		Dimension d = new Dimension(370, 300);
		this.setPreferredSize(d);
		// we will use 12 series of points, one for each color we have choosen 
		data = new XYSeriesCollection();
		series = new XYSeries[12];
		for (int i = 0; i < series.length; i++) {
			series[i] = new XYSeries(i + "");
		}
		for (int i = 0; i < series.length; i++) data.addSeries(series[i]);

		boolean legend = false;
		switch(graph_type) {
			case RTN_PLOT:
			case MMM_PLOT:
				legend = true;
				break;
		}
		
		chart = ChartFactory.createScatterPlot(
												null, // title
												null, // x axis label
												null, // y axis label
												data, // data
												PlotOrientation.VERTICAL,
												legend, // legend ?
												false, // tooltips?
												false // urls?
												);
		
		chart.setAntiAlias(false);
		plot = (XYPlot) chart.getPlot();
		plot.setBackgroundPaint(Color.WHITE);
		plot.setAxisOffset(new RectangleInsets(0.0, 0.0, 0.0, 0.0));
		plot.setDomainGridlinePaint(Color.LIGHT_GRAY);
		plot.setRangeGridlinePaint(Color.LIGHT_GRAY);
		if (legend) plot.setFixedLegendItems(buildLegend());
		
		// For routine plot we use the standard renderer, instead for all other
		// graph we use a custom sampling renderer
		if (graph_type == RTN_PLOT || graph_type == RATIO_TUPLES_PLOT
                || graph_type == EXTERNAL_INPUT_PLOT) {
		
			renderer = plot.getRenderer();
		
		} else {
		
			renderer = new SamplingXYLineAndShapeRenderer(false, true);
			plot.setRenderer(renderer);
		
		}
		
		for (int i = 0; i < series.length; i++) {
			
			// Associate each series with a color, shape, line, etc
			
			renderer.setSeriesOutlinePaint(i, colors[i]);
			if (graph_type == RTN_PLOT) {

				renderer.setSeriesShape(i, new Rectangle2D.Double(-2.5, -2.5, 3.0, 3.0));
				XYLineAndShapeRenderer r = (XYLineAndShapeRenderer) renderer;
				r.setSeriesLinesVisible(i, true);

				r.setSeriesStroke(i, new BasicStroke(2f, BasicStroke.JOIN_ROUND, BasicStroke.JOIN_BEVEL));

				switch(i) {

					case 0:
						renderer.setSeriesPaint(i, Color.BLACK);
						r.setSeriesOutlinePaint(i, Color.BLACK);
						break;
					case 1:
						renderer.setSeriesPaint(i, Color.RED);
						r.setSeriesOutlinePaint(i, Color.RED);
						break;
					case 2:
						renderer.setSeriesPaint(i, Color.BLUE);
						r.setSeriesOutlinePaint(i, Color.BLUE);
						break;
					case 3:
						renderer.setSeriesPaint(i, Color.GREEN);
						r.setSeriesOutlinePaint(i, Color.GREEN);
						break;
					case 4:
						renderer.setSeriesPaint(i, Color.ORANGE);
						r.setSeriesOutlinePaint(i, Color.ORANGE);
						break;
					default:
						renderer.setSeriesPaint(i, colors[i]);
						r.setSeriesOutlinePaint(i, colors[i]);
						break;
				}
				r.setUseOutlinePaint(true);
				
			} else if(graph_type == RATIO_TUPLES_PLOT || 
                        graph_type == EXTERNAL_INPUT_PLOT) {
            
                renderer.setSeriesShape(i, new Rectangle2D.Double(-2.5, -2.5, 3.0, 3.0));
				XYLineAndShapeRenderer r = (XYLineAndShapeRenderer) renderer;
				r.setSeriesLinesVisible(i, true);

				r.setSeriesStroke(i, new BasicStroke(2f, BasicStroke.JOIN_ROUND,
                        BasicStroke.JOIN_BEVEL));

                switch(i) {

					case 0:
						renderer.setSeriesPaint(i, Color.RED);
						r.setSeriesOutlinePaint(i, Color.BLACK);
						break;
					case 1:
						renderer.setSeriesPaint(i, Color.RED);
						r.setSeriesOutlinePaint(i, Color.RED);
						break;
					case 2:
						renderer.setSeriesPaint(i, Color.BLUE);
						r.setSeriesOutlinePaint(i, Color.BLUE);
						break;
					case 3:
						renderer.setSeriesPaint(i, Color.GREEN);
						r.setSeriesOutlinePaint(i, Color.GREEN);
						break;
					case 4:
						renderer.setSeriesPaint(i, Color.ORANGE);
						r.setSeriesOutlinePaint(i, Color.ORANGE);
						break;
					default:
						renderer.setSeriesPaint(i, colors[i]);
						r.setSeriesOutlinePaint(i, colors[i]);
						break;
				}
				r.setUseOutlinePaint(true);
            
            } else {
			
				renderer.setSeriesShape(i, new Rectangle2D.Double(-2.5, -2.5, 3.0, 3.0));
				renderer.setSeriesPaint(i, colors[i]);
				((XYLineAndShapeRenderer)renderer).setUseOutlinePaint(true);
			
			}
		}
		((XYLineAndShapeRenderer)renderer).setDrawOutlines(true);

		chartPanel = new ChartPanel(chart, true);
		chartPanel.getChartRenderingInfo().setEntityCollection(null);
		chartPanel.setMouseZoomable(true, true);
		chartPanel.setPopupMenu(null);
		chartPanel.addMouseListener(new java.awt.event.MouseAdapter() {
			@Override
			public void mousePressed(java.awt.event.MouseEvent evt) {}
			@Override
			public void mouseReleased(java.awt.event.MouseEvent evt) {
				chartMouseReleased();
			}
		});
		chartPanel.addMouseMotionListener(new java.awt.event.MouseMotionAdapter() {
			@Override
			public void mouseDragged(java.awt.event.MouseEvent evt) {
				//System.out.println("Mouse dragged");
				chartMouseDragged();
			}
		});

		initComponents();
		this.add(chartPanel);

		updateGraphTitle();
		updateXAxis(false);
		updateYAxis(false);
		
	}
	
	private LegendItemCollection buildLegend() {
		
		LegendItemCollection legenditemcollection = new LegendItemCollection();
		if (graph_type == MMM_PLOT) {
			
			LegendItem legenditem = new LegendItem("Maximum cost", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, colors[11]);
			LegendItem legenditem1 = new LegendItem("Average cost", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, colors[5]);
			LegendItem legenditem2 = new LegendItem("Minimum cost", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, colors[0]);
			legenditemcollection.add(legenditem);
			legenditemcollection.add(legenditem1);
			legenditemcollection.add(legenditem2);

		} else if (graph_type == RTN_PLOT) {

			LegendItem legenditem = new LegendItem("% calls", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.BLACK);
			LegendItem legenditem1 = new LegendItem("% avg calls", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.RED);
			LegendItem legenditem2 = new LegendItem("% routines (distinct)", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.BLUE);
			LegendItem legenditem3 = new LegendItem("% max calls", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.GREEN);
			LegendItem legenditem4 = new LegendItem("% routines (at least)", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.ORANGE);

			legenditemcollection.add(legenditem);
			legenditemcollection.add(legenditem1);
			legenditemcollection.add(legenditem3);
			legenditemcollection.add(legenditem2);
			legenditemcollection.add(legenditem4);
		
		} else return null;
		
		return legenditemcollection;
	}

	/** This method is called from within the constructor to
	 * initialize the form.
	 * WARNING: Do NOT modify this code. The content of this method is
	 * always regenerated by the Form Editor.
	 */
	@SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        jPopupMenu1 = new javax.swing.JPopupMenu();
        groupMenuButtonGroup = new javax.swing.ButtonGroup();
        jRadioButtonMenuItem1 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem2 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem3 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem4 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem5 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem6 = new javax.swing.JRadioButtonMenuItem();
        jPopupMenu2 = new javax.swing.JPopupMenu();
        jMenuItem1 = new javax.swing.JMenuItem();
        jMenuItem2 = new javax.swing.JMenuItem();
        jMenuItem3 = new javax.swing.JMenuItem();
        jMenuItem4 = new javax.swing.JMenuItem();
        jMenuItem5 = new javax.swing.JMenuItem();
        jMenuItem6 = new javax.swing.JMenuItem();
        jMenuItem7 = new javax.swing.JMenuItem();
        jMenuItem8 = new javax.swing.JMenuItem();
        jMenuItem9 = new javax.swing.JMenuItem();
        jMenuItem10 = new javax.swing.JMenuItem();
        jMenuItem11 = new javax.swing.JMenuItem();
        jMenuItem12 = new javax.swing.JMenuItem();
        jMenuItem13 = new javax.swing.JMenuItem();
        jMenuItem14 = new javax.swing.JMenuItem();
        jPopupMenu3 = new javax.swing.JPopupMenu();
        jRadioButtonMenuItem7 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem8 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem9 = new javax.swing.JRadioButtonMenuItem();
        jPopupMenu4 = new javax.swing.JPopupMenu();
        jRadioButtonMenuItem10 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem11 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem12 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem13 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem14 = new javax.swing.JRadioButtonMenuItem();
        jRadioButtonMenuItem15 = new javax.swing.JRadioButtonMenuItem();
        jToolBar1 = new javax.swing.JToolBar();
        jToggleButton1 = new javax.swing.JToggleButton();
        jToggleButton2 = new javax.swing.JToggleButton();
        jButton2 = new javax.swing.JButton();
        jToggleButton6 = new javax.swing.JToggleButton();
        jToggleButton7 = new javax.swing.JToggleButton();
        jButton1 = new javax.swing.JButton();
        jToggleButton3 = new javax.swing.JToggleButton();
        jToggleButton4 = new javax.swing.JToggleButton();
        jButton3 = new javax.swing.JButton();
        jButton4 = new javax.swing.JButton();
        jPanel1 = new javax.swing.JPanel();
        jLabel2 = new javax.swing.JLabel();

        jPopupMenu1.addPopupMenuListener(new javax.swing.event.PopupMenuListener() {
            public void popupMenuCanceled(javax.swing.event.PopupMenuEvent evt) {
            }
            public void popupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {
                jPopupMenu1PopupMenuWillBecomeInvisible(evt);
            }
            public void popupMenuWillBecomeVisible(javax.swing.event.PopupMenuEvent evt) {
            }
        });

        jRadioButtonMenuItem1.setSelected(true);
        jRadioButtonMenuItem1.setText("1");
        jRadioButtonMenuItem1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem1ActionPerformed(evt);
            }
        });
        jPopupMenu1.add(jRadioButtonMenuItem1);

        jRadioButtonMenuItem2.setText("2");
        jRadioButtonMenuItem2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem2ActionPerformed(evt);
            }
        });
        jPopupMenu1.add(jRadioButtonMenuItem2);

        jRadioButtonMenuItem3.setText("3");
        jRadioButtonMenuItem3.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem3ActionPerformed(evt);
            }
        });
        jPopupMenu1.add(jRadioButtonMenuItem3);

        jRadioButtonMenuItem4.setText("4");
        jRadioButtonMenuItem4.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem4ActionPerformed(evt);
            }
        });
        jPopupMenu1.add(jRadioButtonMenuItem4);

        jRadioButtonMenuItem5.setText("5");
        jRadioButtonMenuItem5.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem5ActionPerformed(evt);
            }
        });
        jPopupMenu1.add(jRadioButtonMenuItem5);

        jRadioButtonMenuItem6.setText("6");
        jRadioButtonMenuItem6.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem6ActionPerformed(evt);
            }
        });
        jPopupMenu1.add(jRadioButtonMenuItem6);
        groupMenuButtonGroup.add(jRadioButtonMenuItem1);
        groupMenuButtonGroup.add(jRadioButtonMenuItem2);
        groupMenuButtonGroup.add(jRadioButtonMenuItem3);
        groupMenuButtonGroup.add(jRadioButtonMenuItem4);
        groupMenuButtonGroup.add(jRadioButtonMenuItem5);
        groupMenuButtonGroup.add(jRadioButtonMenuItem6);

        jPopupMenu2.addPopupMenuListener(new javax.swing.event.PopupMenuListener() {
            public void popupMenuCanceled(javax.swing.event.PopupMenuEvent evt) {
            }
            public void popupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {
                jPopupMenu2PopupMenuWillBecomeInvisible(evt);
            }
            public void popupMenuWillBecomeVisible(javax.swing.event.PopupMenuEvent evt) {
            }
        });

        jMenuItem1.setText("T(n)/n ");
        jMenuItem1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem1ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem1);

        jMenuItem2.setText("T(n)/(n log log n) ");
        jMenuItem2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem2ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem2);

        jMenuItem3.setText("T(n)/(n log n) ");
        jMenuItem3.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem3ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem3);

        jMenuItem4.setText("T(n)/n^1.5 ");
        jMenuItem4.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem4ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem4);

        jMenuItem5.setText("T(n)/n^2 ");
        jMenuItem5.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem5ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem5);

        jMenuItem6.setText("T(n)/n^2.5 ");
        jMenuItem6.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem6ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem6);

        jMenuItem7.setText("T(n)/n^3 ");
        jMenuItem7.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem7ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem7);

        jMenuItem8.setText("*= log log n");
        jMenuItem8.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem8ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem8);

        jMenuItem9.setText("/= log log n");
        jMenuItem9.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem9ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem9);

        jMenuItem10.setText("*= log n");
        jMenuItem10.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem10ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem10);

        jMenuItem11.setText("/= log n");
        jMenuItem11.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem11ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem11);

        jMenuItem12.setText("*= (log log n)^eps ...");
        jMenuItem12.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem12ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem12);

        jMenuItem13.setText("*= (log n)^eps ...");
        jMenuItem13.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem13ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem13);

        jMenuItem14.setText("*= n^eps ...");
        jMenuItem14.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem14ActionPerformed(evt);
            }
        });
        jPopupMenu2.add(jMenuItem14);

        jPopupMenu3.addPopupMenuListener(new javax.swing.event.PopupMenuListener() {
            public void popupMenuCanceled(javax.swing.event.PopupMenuEvent evt) {
            }
            public void popupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {
                jPopupMenu3PopupMenuWillBecomeInvisible(evt);
            }
            public void popupMenuWillBecomeVisible(javax.swing.event.PopupMenuEvent evt) {
            }
        });
        groupMenuButtonGroup2 = new javax.swing.ButtonGroup();

        jRadioButtonMenuItem7.setSelected(true);
        jRadioButtonMenuItem7.setText("Maximum");
        jRadioButtonMenuItem7.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem7ActionPerformed(evt);
            }
        });
        jPopupMenu3.add(jRadioButtonMenuItem7);

        jRadioButtonMenuItem8.setSelected(true);
        jRadioButtonMenuItem8.setText("Average");
        jRadioButtonMenuItem8.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem8ActionPerformed(evt);
            }
        });
        jPopupMenu3.add(jRadioButtonMenuItem8);

        jRadioButtonMenuItem9.setSelected(true);
        jRadioButtonMenuItem9.setText("Minimum");
        jRadioButtonMenuItem9.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem9ActionPerformed(evt);
            }
        });
        jPopupMenu3.add(jRadioButtonMenuItem9);

        groupMenuButtonGroup2.add(jRadioButtonMenuItem7);
        groupMenuButtonGroup2.add(jRadioButtonMenuItem8);
        groupMenuButtonGroup2.add(jRadioButtonMenuItem9);

        jPopupMenu4.addPopupMenuListener(new javax.swing.event.PopupMenuListener() {
            public void popupMenuCanceled(javax.swing.event.PopupMenuEvent evt) {
            }
            public void popupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {
                jPopupMenu4PopupMenuWillBecomeInvisible(evt);
            }
            public void popupMenuWillBecomeVisible(javax.swing.event.PopupMenuEvent evt) {
            }
        });

        jRadioButtonMenuItem10.setSelected(true);
        jRadioButtonMenuItem10.setText("1");
        jRadioButtonMenuItem10.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem10ActionPerformed(evt);
            }
        });
        jPopupMenu4.add(jRadioButtonMenuItem10);

        jRadioButtonMenuItem11.setSelected(true);
        jRadioButtonMenuItem11.setText("2");
        jRadioButtonMenuItem11.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem11ActionPerformed(evt);
            }
        });
        jPopupMenu4.add(jRadioButtonMenuItem11);

        jRadioButtonMenuItem12.setSelected(true);
        jRadioButtonMenuItem12.setText("3");
        jRadioButtonMenuItem12.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem12ActionPerformed(evt);
            }
        });
        jPopupMenu4.add(jRadioButtonMenuItem12);

        jRadioButtonMenuItem13.setSelected(true);
        jRadioButtonMenuItem13.setText("4");
        jRadioButtonMenuItem13.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem13ActionPerformed(evt);
            }
        });
        jPopupMenu4.add(jRadioButtonMenuItem13);

        jRadioButtonMenuItem14.setSelected(true);
        jRadioButtonMenuItem14.setText("5");
        jRadioButtonMenuItem14.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem14ActionPerformed(evt);
            }
        });
        jPopupMenu4.add(jRadioButtonMenuItem14);

        jRadioButtonMenuItem15.setSelected(true);
        jRadioButtonMenuItem15.setText("6");
        jRadioButtonMenuItem15.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem15ActionPerformed(evt);
            }
        });
        jPopupMenu4.add(jRadioButtonMenuItem15);

        groupMenuButtonGroup3 = new javax.swing.ButtonGroup();
        groupMenuButtonGroup3.add(jRadioButtonMenuItem10);
        groupMenuButtonGroup3.add(jRadioButtonMenuItem11);
        groupMenuButtonGroup3.add(jRadioButtonMenuItem12);
        groupMenuButtonGroup3.add(jRadioButtonMenuItem13);
        groupMenuButtonGroup3.add(jRadioButtonMenuItem14);
        groupMenuButtonGroup3.add(jRadioButtonMenuItem15);

        setBorder(javax.swing.BorderFactory.createEtchedBorder());
        setLayout(new javax.swing.BoxLayout(this, javax.swing.BoxLayout.Y_AXIS));

        jToolBar1.setBorder(null);
        jToolBar1.setFloatable(false);
        jToolBar1.setRollover(true);
        jToolBar1.setMargin(new java.awt.Insets(4, 3, 3, 3));

        jToggleButton1.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Xlog-icon.png"))); // NOI18N
        jToggleButton1.setToolTipText("toggle x axis logarithmic scale");
        jToggleButton1.setFocusTraversalPolicyProvider(true);
        jToggleButton1.setFocusable(false);
        jToggleButton1.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        jToggleButton1.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        jToggleButton1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton1ActionPerformed(evt);
            }
        });
        jToolBar1.add(jToggleButton1);

        jToggleButton2.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Ylog-icon.png"))); // NOI18N
        jToggleButton2.setToolTipText("toggle y axis logarithmic scale");
        jToggleButton2.setFocusable(false);
        jToggleButton2.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        jToggleButton2.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        jToggleButton2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton2ActionPerformed(evt);
            }
        });
        jToolBar1.add(jToggleButton2);

        jButton2.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Maximize-icon.png"))); // NOI18N
        jButton2.setEnabled(false);
        jButton2.setFocusable(false);
        jButton2.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        jButton2.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        jButton2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jButton2ActionPerformed(evt);
            }
        });
        jButton2.setVisible(false);
        jToolBar1.add(jButton2);

        jToggleButton6.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Incorporate-icon.png"))); // NOI18N
        jToggleButton6.setToolTipText("group graph points");
        jToggleButton6.setFocusable(false);
        jToggleButton6.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        jToggleButton6.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        jToggleButton6.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton6ActionPerformed(evt);
            }
        });
        if (graph_type == RTN_PLOT
            || graph_type == EXTERNAL_INPUT_PLOT
            || graph_type == RATIO_TUPLES_PLOT
            || graph_type == ALPHA_PLOT)
        jToggleButton6.setVisible(false);
        jToolBar1.add(jToggleButton6);

        jToggleButton7.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/RatioType-icon.png"))); // NOI18N
        jToggleButton7.setToolTipText("select ratio type");
        jToggleButton7.setFocusable(false);
        jToggleButton7.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        jToggleButton7.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        jToggleButton7.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton7ActionPerformed(evt);
            }
        });
        if (graph_type != RATIO_PLOT) jToggleButton7.setVisible(false);
        jToolBar1.add(jToggleButton7);

        jButton1.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/SavePlot-icon.png"))); // NOI18N
        jButton1.setToolTipText("export as PNG image");
        jButton1.setBorder(null);
        jButton1.setFocusable(false);
        jButton1.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        jButton1.setMaximumSize(new java.awt.Dimension(28, 28));
        jButton1.setMinimumSize(new java.awt.Dimension(28, 28));
        jButton1.setPreferredSize(new java.awt.Dimension(28, 28));
        jButton1.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        jButton1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jButton1ActionPerformed(evt);
            }
        });
        jToolBar1.add(jButton1);

        jToggleButton3.setFont(new java.awt.Font("Ubuntu", 1, 13)); // NOI18N
        jToggleButton3.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/type.png"))); // NOI18N
        jToggleButton3.setToolTipText("Select cost type");
        jToggleButton3.setFocusable(false);
        jToggleButton3.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        jToggleButton3.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        jToggleButton3.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton3ActionPerformed(evt);
            }
        });
        if (graph_type != COST_PLOT && graph_type != RATIO_PLOT)
        jToggleButton3.setVisible(false);
        jToolBar1.add(jToggleButton3);

        jToggleButton4.setFont(new java.awt.Font("Ubuntu", 1, 13)); // NOI18N
        jToggleButton4.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/tool_curve.png"))); // NOI18N
        jToggleButton4.setToolTipText("Smooth point window");
        jToggleButton4.setFocusable(false);
        jToggleButton4.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        jToggleButton4.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        jToggleButton4.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton4ActionPerformed(evt);
            }
        });
        if (graph_type == RTN_PLOT
            || graph_type == EXTERNAL_INPUT_PLOT
            || graph_type == RATIO_TUPLES_PLOT
            || graph_type == ALPHA_PLOT)
        jToggleButton4.setVisible(false);
        jToolBar1.add(jToggleButton4);

        jButton3.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/zoom_out_16.png"))); // NOI18N
        jButton3.setToolTipText("Zoom out");
        jButton3.setBorder(null);
        jButton3.setEnabled(false);
        jButton3.setVisible(false);
        jButton3.setFocusable(false);
        jButton3.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        jButton3.setMaximumSize(new java.awt.Dimension(28, 28));
        jButton3.setMinimumSize(new java.awt.Dimension(28, 28));
        jButton3.setPreferredSize(new java.awt.Dimension(28, 28));
        jButton3.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        jButton3.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jButton3ActionPerformed(evt);
            }
        });
        jToolBar1.add(jButton3);

        jButton4.setFont(new java.awt.Font("Arial", 1, 20)); // NOI18N
        jButton4.setText("α");
        jButton4.setToolTipText("Amortized costant");
        jButton4.setBorder(null);
        jButton4.setFocusable(false);
        jButton4.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        jButton4.setMaximumSize(new java.awt.Dimension(28, 28));
        jButton4.setMinimumSize(new java.awt.Dimension(28, 28));
        jButton4.setPreferredSize(new java.awt.Dimension(28, 28));
        jButton4.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        jButton4.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jButton4ActionPerformed(evt);
            }
        });
        if (graph_type != AMORTIZED_PLOT) jButton4.setVisible(false);
        jToolBar1.add(jButton4);

        jPanel1.setBorder(null);
        jPanel1.setMinimumSize(new java.awt.Dimension(30, 10));
        jPanel1.setPreferredSize(new java.awt.Dimension(30, 10));
        jToolBar1.add(jPanel1);

        if (graph_type == FREQ_PLOT
            || graph_type == MMM_PLOT
            || graph_type == RTN_PLOT
            || graph_type == EXTERNAL_INPUT_PLOT
            || graph_type == RATIO_TUPLES_PLOT)
        jLabel2.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Dummy.png"))); // NOI18N
        else {
            jLabel2.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Color-scale.png"))); // NOI18N
            //jLabel2.setBorder(new javax.swing.border.LineBorder(java.awt.Color.BLACK));
        }
        jToolBar1.add(jLabel2);

        add(jToolBar1);
    }// </editor-fold>//GEN-END:initComponents

	private void chartMouseReleased() {
		
		//System.out.println(this.getSize(null));
		
		double cmin_x = domainAxis.getLowerBound();
		double cmax_x = domainAxis.getUpperBound();
		double cmin_y = rangeAxis.getLowerBound();
		double cmax_y = rangeAxis.getUpperBound();
		
		// Have we zoomed?
		boolean changed_bounds = (min_x != cmin_x) || (max_x != cmax_x)
									|| (cmin_y != min_y) || (cmax_y != max_y);
		if (this.dragging && changed_bounds) {
			
			zoomResetVisible(true);
			this.status = ZOOMED;
			min_x = cmin_x;
			max_x = cmax_x;
			min_y = cmin_y;
			max_y = cmax_y;
			
			//System.out.println("X range: " + new_min_x + ":" + new_max_x);
			//System.out.println("Y range: " + new_min_y + ":" + new_max_y);
			
		}
		
		this.dragging = false;
	}

	private void chartMouseDragged() {
		this.dragging = true;
	}
	
	public int getGraphType() {
		return graph_type;
	}

	public int getGraphPriority() {
		
		switch(graph_type) {
		
			case COST_PLOT: return 0;
			case AMORTIZED_PLOT: return 1;
			case FREQ_PLOT: return 2;
			case MMM_PLOT: return 3;
			case TOTALCOST_PLOT: return 4;
			case VAR_PLOT: return 5;
			case RATIO_PLOT: return 6;
			case RTN_PLOT: return 7;
            case RATIO_TUPLES_PLOT: return 8;
            case EXTERNAL_INPUT_PLOT: return 9;
            case ALPHA_PLOT: return 10;
				
			default: return 11;
		
		}
		
	}
	
	private String getXLabel() {
		
		switch(graph_type) {
			
			case RTN_PLOT: 
				return "number of collected tuples";
			
            case RATIO_TUPLES_PLOT:
            case EXTERNAL_INPUT_PLOT:
                return "percentage of routines";
            
            case ALPHA_PLOT:
                return "α";
                
			default:
                
                if (main_window.isInputMetricRms())
                    return "read memory size";
                else
                    return "dynamic read memory size";
                
		}
	
	} 
	
	private String getYLabel() {
		
		switch(graph_type) {
			
			case RATIO_PLOT:
				return "ratio";
			
			case FREQ_PLOT: 
				return "frequency";
			
			case AMORTIZED_PLOT:
				return "mean cumulative cost";
				
			case RTN_PLOT: 
				return "percentage of routines";
			
            case RATIO_TUPLES_PLOT:
                return "ratio";
            case EXTERNAL_INPUT_PLOT:
                return "percentage (ratio)";
            
            case ALPHA_PLOT:
                return "number of points";
                
			default: 
				return "cost";
		
		}
	
	} 

	private void updateXAxis(boolean reset) {
		
		String label = getXLabel();

		if (x_log_scale) {

			LogarithmicAxis newDomainAxis = new LogarithmicAxis(label);
			newDomainAxis.setAllowNegativesFlag(false);
			newDomainAxis.setStrictValuesFlag(false);
			newDomainAxis.setStandardTickUnits(LogarithmicAxis.createIntegerTickUnits());
			this.domainAxis = newDomainAxis;
		
		} else {
		
			NumberAxis newDomainAxis = new NumberAxis(label);
			this.domainAxis = newDomainAxis;
			this.domainAxis.setStandardTickUnits(NumberAxis.createIntegerTickUnits());
		
		}
		
        if (graph_type == this.RATIO_TUPLES_PLOT ||
                graph_type == this.EXTERNAL_INPUT_PLOT) {
            
            domainAxis.setRange(min_x, max_x);
        
        } else if (!reset && status == ZOOMED) {
			
			domainAxis.setRange(min_x, max_x);
		
		} else {
			
			domainAxis.setAutoRangeIncludesZero(true);
		
		}
		
		plot.setDomainAxis(domainAxis);
	}

	private void updateYAxis(boolean reset) {

		String label = getYLabel();

		if (y_log_scale) {
		
			LogarithmicAxis newRangeAxis = new LogarithmicAxis(label);
			newRangeAxis.setAllowNegativesFlag(false);
			newRangeAxis.setStrictValuesFlag(false);
			this.rangeAxis = newRangeAxis;
		
		} else {
			
			NumberAxis newRangeAxis = new NumberAxis(label);
			this.rangeAxis = newRangeAxis;
		
		}
        
		 if (graph_type == this.RATIO_TUPLES_PLOT ||
                graph_type == this.EXTERNAL_INPUT_PLOT) {
            
            rangeAxis.setRange(min_y, max_y);
        
        } else if (!reset && status == ZOOMED) {
			
			//System.out.println("Set range y: " + min_y + " " + max_y);
			rangeAxis.setRange(min_y, max_y);
			if (graph_type == GraphPanel.FREQ_PLOT)
				this.rangeAxis.setStandardTickUnits(NumberAxis.createIntegerTickUnits());
		
		} else {
			
			rangeAxis.setAutoRangeIncludesZero(true);
			
		}

		plot.setRangeAxis(rangeAxis);

	}
	
	public void updateGraphTitle() {
		
		String s;
		switch (this.graph_type) {
			
			case COST_PLOT: s = "Cost plot"; break;
			case FREQ_PLOT: 
                if (main_window.isInputMetricRms()) {
                    s = "RMS frequency plot"; break;
                } else {
                    s = "DRMS frequency plot"; break;
                }
            case MMM_PLOT: s = "Min/Avg/Max cost plot"; break;
			case TOTALCOST_PLOT: s = "Total cost plot"; break;
			case VAR_PLOT: s = "Cost variance plot"; break;
			case RTN_PLOT: s = "Program statistics plot"; break;
            case RATIO_TUPLES_PLOT: s = "Ratio #DRMS/#RMS plot"; break;
            case EXTERNAL_INPUT_PLOT: s = "External input plot"; break;
            case ALPHA_PLOT: s = "Alpha plot"; break;
			case AMORTIZED_PLOT: 
                if (rtn_info != null)
                    s = "Amortized cost plot (α = " + rtn_info.getAmortizedConstant() + ")" ; 
                else
                    s = "Amortized cost plot";
                break;
			case 
                    RATIO_PLOT: s = "Curve bounding plot - T(n) / ";
							double[] rc = Rms.getRatioConfig();
							int k = 0;
							for (int i =  0; i < rc.length; i++) {
								if (rc[i] != 0) k++;
							}
							if (k > 1) s += "(";
							if (rc[0] != 0) {
								if (rc[0] == 1) s += "n";
								else s += "n^" + rc[0];
							}
							if (rc[1] != 0) {
								if (rc[0] != 0) s += " * ";
								if (rc[1] == 1) s += "log(n)";
								else s += "log(n)^" + rc[1];
							}
							if (rc[2] != 0) {
								if (rc[0] != 0 || rc[1] != 0) s += " * ";
								if (rc[2] == 1) s += "log(log(n))";
								else s += "log(log(n))^" + rc[2];
							}
							if (k > 1) s += ")";
							break;
			default:
				s = "";
		}
		
		chart.setTitle(new TextTitle(s, new Font(Font.SERIF, Font.BOLD, 14)));
	}

	public void zoomResetVisible(boolean visible) {
		
		if (visible) {
		
			jButton3.setEnabled(true);
			jButton3.setVisible(true);
		
		} else {
		
			jButton3.setEnabled(false);
			jButton3.setVisible(false);
			
		}
		
	}
	
	public void refreshFilter() {
		
		filters = main_window.getRmsTableFilter();
		refresh(true);
	
	}

	public void setXLogScale(boolean logscale) {
		
		x_log_scale = logscale;
		jToggleButton1.setSelected(logscale);
		updateXAxis(false);
        updateMinimalTicks();
	
	}

	public void setYLogScale(boolean logscale) {
		
		y_log_scale = logscale;
		jToggleButton2.setSelected(logscale);
		updateYAxis(false);
        updateMinimalTicks();
        
	}

	public void setGroupCost(int cost_type) {
		
		if (cost_type == this.cost_type) return;
		switch(cost_type) {
			case MAX_COST:  jRadioButtonMenuItem7.setSelected(true);
							break;
			case AVG_COST:  jRadioButtonMenuItem8.setSelected(true);
							break;
			case MIN_COST:  jRadioButtonMenuItem9.setSelected(true);
							break;
		}
		this.cost_type = cost_type;
		refresh(false);
	
	}

	public void setSmoothThreshold(int t) {
		
        //System.out.println("Set smooth threashold");
        
		if (graph_type == RTN_PLOT || rtn_info == null
                || graph_type == RATIO_TUPLES_PLOT
                || graph_type == EXTERNAL_INPUT_PLOT
                || graph_type == ALPHA_PLOT) return;
		
		// reset grouping
		group_threshold = 1;
		jRadioButtonMenuItem1.setSelected(true);
		
		switch (t) {
			case 1: jRadioButtonMenuItem10.setSelected(true);
					break;
			case 2: jRadioButtonMenuItem11.setSelected(true);
					break;
			case 3: jRadioButtonMenuItem12.setSelected(true);
					break;
			case 4: jRadioButtonMenuItem13.setSelected(true);
					break;
			case 5: jRadioButtonMenuItem14.setSelected(true);
					break;
			case 6: jRadioButtonMenuItem15.setSelected(true);
					break;
		}
		
		smooth_threshold = t;
		refresh(false);
	
	}
	
	public void setGroupThreshold(int t) {

        //System.out.println("Set group threashold");
        
		if (graph_type == RTN_PLOT || rtn_info == null
                || graph_type == RATIO_TUPLES_PLOT
                || graph_type == EXTERNAL_INPUT_PLOT
                || graph_type == ALPHA_PLOT) return;
		
		// Reset smoothing
		smooth_threshold = 1;
		jRadioButtonMenuItem10.setSelected(true);
		
		switch (t) {
			case 1: jRadioButtonMenuItem1.setSelected(true);
					break;
			case 2: jRadioButtonMenuItem2.setSelected(true);
					break;
			case 3: jRadioButtonMenuItem3.setSelected(true);
					break;
			case 4: jRadioButtonMenuItem4.setSelected(true);
					break;
			case 5: jRadioButtonMenuItem5.setSelected(true);
					break;
			case 6: jRadioButtonMenuItem6.setSelected(true);
					break;
		}
		
		group_threshold = (int) Math.pow(group_threshold_base, t-1);
		refresh(false);
		
	}
	
	
    public void maximize() {
		
		this.status = MAXIMIZED;
        
        updateXAxis(true);
		updateYAxis(true);
        
        chartPanel.restoreAutoBounds();
        
		max_x = domainAxis.getUpperBound();
		max_y = rangeAxis.getUpperBound();
        
		if (graph_type == RTN_PLOT
            || graph_type == RATIO_TUPLES_PLOT
            || graph_type == EXTERNAL_INPUT_PLOT) {
            
			min_x = -10;
			min_y = 0;
		
        } else {
			min_x = domainAxis.getLowerBound();
			min_y = rangeAxis.getLowerBound();
		}
        
        updateMinimalTicks();
    }
    
    private void updateMinimalTicks() {
    
        if (!x_log_scale && max_x < 10) {
            //System.out.println("Set minimal ticks");
            domainAxis.setTickUnit(new NumberTickUnit(max_x / 10));
        }
            
        if (!y_log_scale && max_y < 10) 
            rangeAxis.setTickUnit(new NumberTickUnit(max_y / 10));

    }

	private void jToggleButton1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton1ActionPerformed

		// X logscale button
		boolean logscale;
		if (jToggleButton1.isSelected()) logscale = true;
		else logscale = false;
		setXLogScale(logscale);
		if (this.main_window.arePlotsLinked()) 
            main_window.setXLogScaleAll(graph_type, logscale);
		
	}//GEN-LAST:event_jToggleButton1ActionPerformed

	private void jToggleButton2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton2ActionPerformed
	   
		// Y logscale button
		boolean logscale;
		if (jToggleButton2.isSelected()) logscale = true;
		else logscale = false;
		setYLogScale(logscale);
		if (this.main_window.arePlotsLinked()) main_window.setYLogScaleAll(graph_type, logscale);
		
	}//GEN-LAST:event_jToggleButton2ActionPerformed

	private void jToggleButton6ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton6ActionPerformed
        
		// Grouing button
		if (jToggleButton6.isSelected()) {
			jPopupMenu1.show(jToggleButton6, 0, jToggleButton6.getHeight());
		} else {
			jPopupMenu1.setVisible(false);
		}
		
	}//GEN-LAST:event_jToggleButton6ActionPerformed

	private void jPopupMenu1PopupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {//GEN-FIRST:event_jPopupMenu1PopupMenuWillBecomeInvisible
		jToggleButton6.setSelected(false);
	}//GEN-LAST:event_jPopupMenu1PopupMenuWillBecomeInvisible

	private void jRadioButtonMenuItem1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem1ActionPerformed
		
		// Grouping menu > first entry
		setGroupThreshold(1);
		if (this.main_window.arePlotsLinked()) main_window.setGroupThresholdAll(graph_type, 1);
		
	}//GEN-LAST:event_jRadioButtonMenuItem1ActionPerformed

	private void jRadioButtonMenuItem2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem2ActionPerformed
		
		// Grouping menu > second entry
		setGroupThreshold(2);
		if (this.main_window.arePlotsLinked()) main_window.setGroupThresholdAll(graph_type, 2);
		
	}//GEN-LAST:event_jRadioButtonMenuItem2ActionPerformed

	private void jRadioButtonMenuItem3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem3ActionPerformed
		
		// Grouping menu > third entry
		setGroupThreshold(3);
		if (this.main_window.arePlotsLinked()) main_window.setGroupThresholdAll(graph_type, 3);
		
	}//GEN-LAST:event_jRadioButtonMenuItem3ActionPerformed

	private void jRadioButtonMenuItem4ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem4ActionPerformed
		
		// Grouping menu > fourth entry
		setGroupThreshold(4);
		if (this.main_window.arePlotsLinked()) main_window.setGroupThresholdAll(graph_type, 4);
		
	}//GEN-LAST:event_jRadioButtonMenuItem4ActionPerformed

	private void jRadioButtonMenuItem5ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem5ActionPerformed
		
		// Grouping menu > fifth entry
		setGroupThreshold(5);
		if (this.main_window.arePlotsLinked()) main_window.setGroupThresholdAll(graph_type, 5);
		
	}//GEN-LAST:event_jRadioButtonMenuItem5ActionPerformed

	private void jRadioButtonMenuItem6ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem6ActionPerformed
		
		// Grouping menu > sixth entry
		setGroupThreshold(6);
		if (this.main_window.arePlotsLinked()) main_window.setGroupThresholdAll(graph_type, 6);
		
	}//GEN-LAST:event_jRadioButtonMenuItem6ActionPerformed

	private void jButton1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButton1ActionPerformed
		
        //System.out.println("button1");
        
		// Save graph image button
		if (this.rtn_info == null) return;
		
		String filename = this.rtn_info.getName();
		if (this.graph_type == COST_PLOT) filename += "_time_plot.png";
		else if (this.graph_type == MMM_PLOT) filename += "_mmm_plot.png";
		else if (this.graph_type == TOTALCOST_PLOT) filename += "_sum_plot.png";
		else if (this.graph_type == VAR_PLOT) filename += "_var_plot.png";
		else if (this.graph_type == RATIO_PLOT) filename += "_ratio_plot.png";
		else if (this.graph_type == RTN_PLOT) filename += "_rtn_plot.png";
        else if (this.graph_type == RATIO_TUPLES_PLOT) filename += "_count_rvms_plot.png";
        else if (this.graph_type == EXTERNAL_INPUT_PLOT) filename += "_value_rvms_plot.png";
		else if (this.graph_type == AMORTIZED_PLOT) filename += "_amm_plot.png";
		else filename += "_freq_plot.png";
		
		java.io.File f = new java.io.File(filename);
		javax.swing.JFileChooser chooser = new javax.swing.JFileChooser();
		String lastReportPath = Main.getLastReportPath();
		if (!lastReportPath.equals("")) chooser.setCurrentDirectory(new java.io.File(lastReportPath));
		javax.swing.filechooser.FileNameExtensionFilter filter = new javax.swing.filechooser.FileNameExtensionFilter("PNG images", "png");
		chooser.setFileFilter(filter);
		chooser.setAcceptAllFileFilterUsed(false);
		chooser.setSelectedFile(f);
		int choice = chooser.showSaveDialog(this.getParent());
		if (choice == javax.swing.JFileChooser.APPROVE_OPTION) {
			f = chooser.getSelectedFile();
			java.awt.image.BufferedImage img =
					new java.awt.image.BufferedImage(this.chartPanel.getWidth(),
													this.chartPanel.getHeight(),
													java.awt.image.BufferedImage.TYPE_INT_RGB);
			java.awt.Graphics2D g = img.createGraphics();
			this.chartPanel.paint(g);
			g.dispose();
			try {
				javax.imageio.ImageIO.write(img, "png", f);
			} catch (Exception e) {
					javax.swing.JOptionPane.showMessageDialog(this, 
						"Couldn't save the selected graph image", "Error", 
						javax.swing.JOptionPane.ERROR_MESSAGE);
			}
		}
	}//GEN-LAST:event_jButton1ActionPerformed

	private void jToggleButton7ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton7ActionPerformed
		
		// Ration menu button
		if (jToggleButton7.isSelected()) {
			jPopupMenu2.show(jToggleButton7, 0, jToggleButton7.getHeight());
		} else {
			jPopupMenu2.setVisible(false);
		}
		
	}//GEN-LAST:event_jToggleButton7ActionPerformed

	private void jPopupMenu2PopupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {//GEN-FIRST:event_jPopupMenu2PopupMenuWillBecomeInvisible
		jToggleButton7.setSelected(false);
	}//GEN-LAST:event_jPopupMenu2PopupMenuWillBecomeInvisible

	private void jMenuItem1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem1ActionPerformed
		
		// ratio menu > first entry
		double[] rc = {1, 0, 0};
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem1ActionPerformed

	private void jMenuItem2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem2ActionPerformed
		
		// // ratio menu > second entry
		double[] rc = {1, 0, 1};
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem2ActionPerformed

	private void jMenuItem3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem3ActionPerformed
		
		// // ratio menu > third entry
		double[] rc = {1, 1, 0};
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem3ActionPerformed

	private void jMenuItem4ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem4ActionPerformed
		
		// ratio menu > fourth entry
		double[] rc = {1.5, 0, 0};
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem4ActionPerformed

	private void jMenuItem5ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem5ActionPerformed
		
		// ratio menu > fifth entry
		double[] rc = {2, 0, 0};
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem5ActionPerformed

	private void jMenuItem6ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem6ActionPerformed
		
		// ratio menu > sixth entry
		double[] rc = {2.5, 0, 0};
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem6ActionPerformed

	private void jMenuItem7ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem7ActionPerformed
		
		// ratio menu > seventh entry
		double[] rc = {3, 0, 0};
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem7ActionPerformed

	private void jMenuItem8ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem8ActionPerformed
		
		// ratio menu > eighth entry
		double[] rc = Rms.getRatioConfig();
		rc[2]--;
		if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
			javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
			return;
		}
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem8ActionPerformed

	private void jMenuItem9ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem9ActionPerformed
		
		// ratio menu > ninth entry
		double[] rc = Rms.getRatioConfig();
		rc[2]++;
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem9ActionPerformed

	private void jMenuItem10ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem10ActionPerformed
		
		// ratio menu > 10-th entry
		double[] rc = Rms.getRatioConfig();
		rc[1]--;
		if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
			javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
			return;
		}
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem10ActionPerformed

	private void jMenuItem11ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem11ActionPerformed
		
		// ratio menu > 11-th entry
		double[] rc = Rms.getRatioConfig();
		rc[1]++;
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem11ActionPerformed

	private void jMenuItem12ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem12ActionPerformed
		
		// ratio menu > 12-th entry
		double exp = 0;
		try {
			exp = Double.parseDouble(javax.swing.JOptionPane.showInputDialog(main_window, "Enter exponent"));
		} catch (NumberFormatException e) {
			javax.swing.JOptionPane.showMessageDialog(main_window, "exponent must be a decimal value");
			return;
		} catch (Exception e) {
			return;
		}
		double[] rc = Rms.getRatioConfig();
		rc[2] -= exp;
		if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
			javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
			return;
		}
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem12ActionPerformed

	private void jMenuItem13ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem13ActionPerformed
		
		// ratio menu > 13-th entry
		double exp = 0;
		try {
			exp = Double.parseDouble(javax.swing.JOptionPane.showInputDialog(main_window, "Enter exponent"));
		} catch (NumberFormatException e) {
			javax.swing.JOptionPane.showMessageDialog(main_window, "exponent must be a decimal value");
			return;
		} catch (Exception e) {
			return;
		}
		double[] rc = Rms.getRatioConfig();
		rc[1] -= exp;
		if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
			javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
			return;
		}
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
	}//GEN-LAST:event_jMenuItem13ActionPerformed

	private void jMenuItem14ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem14ActionPerformed
		
		// ratio menu > 14-th entry
		double exp = 0;
		try {
			exp = Double.parseDouble(javax.swing.JOptionPane.showInputDialog(main_window, "Enter exponent"));
		} catch (NumberFormatException e) {
			javax.swing.JOptionPane.showMessageDialog(main_window, "exponent must be a decimal value");
			return;
		} catch (Exception e) {
			return;
		}
		double[] rc = Rms.getRatioConfig();
		rc[0] -= exp;
		if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
			javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
			return;
		}
		Rms.setRatioConfig(rc);
		updateGraphTitle();
		refresh(true);
		main_window.refreshTables();
		
	}//GEN-LAST:event_jMenuItem14ActionPerformed

	private void jButton2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButton2ActionPerformed

		// Maximize/autoscle button
		if (true) return; // currently disabled
		//if (!maximized) maximize();
		//else autoscale();
		//if (this.main_window.arePlotsLinked()) main_window.maximizeAll(graph_type);
		
	}//GEN-LAST:event_jButton2ActionPerformed

	private void jToggleButton3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton3ActionPerformed
		
		// Cost type menu
		if (jToggleButton3.isSelected()) {
			jPopupMenu3.show(jToggleButton3, 0, jToggleButton3.getHeight());
		} else {
			jPopupMenu3.setVisible(false);
		}
	}//GEN-LAST:event_jToggleButton3ActionPerformed

	private void jPopupMenu3PopupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {//GEN-FIRST:event_jPopupMenu3PopupMenuWillBecomeInvisible
		jToggleButton3.setSelected(false);
	}//GEN-LAST:event_jPopupMenu3PopupMenuWillBecomeInvisible

	private void jRadioButtonMenuItem7ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem7ActionPerformed
		
		cost_type = MAX_COST;
		if (main_window.arePlotsLinked()) main_window.setGroupCostAll(graph_type, cost_type);
		refresh(true);
		
	}//GEN-LAST:event_jRadioButtonMenuItem7ActionPerformed

	private void jRadioButtonMenuItem8ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem8ActionPerformed
		
		cost_type = AVG_COST;
		if (main_window.arePlotsLinked()) main_window.setGroupCostAll(graph_type, cost_type);
		refresh(true);
		
	}//GEN-LAST:event_jRadioButtonMenuItem8ActionPerformed

	private void jRadioButtonMenuItem9ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem9ActionPerformed
		
		cost_type = MIN_COST;
		if (main_window.arePlotsLinked()) main_window.setGroupCostAll(graph_type, cost_type);
		refresh(true);
		
	}//GEN-LAST:event_jRadioButtonMenuItem9ActionPerformed

	private void jToggleButton4ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton4ActionPerformed
		
		// Smooth point window button
		if (jToggleButton4.isSelected()) {
			jPopupMenu4.show(jToggleButton4, 0, jToggleButton4.getHeight());
		} else {
			jPopupMenu4.setVisible(false);
		}
		
	}//GEN-LAST:event_jToggleButton4ActionPerformed

	private void jPopupMenu4PopupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {//GEN-FIRST:event_jPopupMenu4PopupMenuWillBecomeInvisible
		jToggleButton4.setSelected(false);
	}//GEN-LAST:event_jPopupMenu4PopupMenuWillBecomeInvisible

	private void jRadioButtonMenuItem10ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem10ActionPerformed
		
		// Smooth window menu: 1-th entry
		setSmoothThreshold(1);
		if (this.main_window.arePlotsLinked()) {
            main_window.setSmoothThresholdAll(graph_type, 1);
        }
		
	}//GEN-LAST:event_jRadioButtonMenuItem10ActionPerformed

	private void jRadioButtonMenuItem11ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem11ActionPerformed
		
		// Smooth window menu: 2-th entry
		setSmoothThreshold(2);
		if (this.main_window.arePlotsLinked()) {
            main_window.setSmoothThresholdAll(graph_type, 2);
        }
		
	}//GEN-LAST:event_jRadioButtonMenuItem11ActionPerformed

	private void jRadioButtonMenuItem12ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem12ActionPerformed
		
		// Smooth window menu: 3-th entry
		setSmoothThreshold(3);
		if (this.main_window.arePlotsLinked())
			main_window.setSmoothThresholdAll(graph_type, 3);
		
	}//GEN-LAST:event_jRadioButtonMenuItem12ActionPerformed

	private void jRadioButtonMenuItem13ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem13ActionPerformed
		
		// Smooth window menu: 4-th entry
		setSmoothThreshold(4);
		if (this.main_window.arePlotsLinked())
			main_window.setSmoothThresholdAll(graph_type, 4);
		
	}//GEN-LAST:event_jRadioButtonMenuItem13ActionPerformed

	private void jRadioButtonMenuItem14ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem14ActionPerformed
		
		// Smooth window menu: 5-th entry
		setSmoothThreshold(5);
		if (this.main_window.arePlotsLinked()) {
            main_window.setSmoothThresholdAll(graph_type, 5);
        }
		
	}//GEN-LAST:event_jRadioButtonMenuItem14ActionPerformed

	private void jRadioButtonMenuItem15ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem15ActionPerformed
		
		// Smooth window menu: 6-th entry
		setSmoothThreshold(6);
		if (this.main_window.arePlotsLinked()) {
            main_window.setSmoothThresholdAll(graph_type, 6);
        }
		
	}//GEN-LAST:event_jRadioButtonMenuItem15ActionPerformed

	private void jButton3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButton3ActionPerformed
		
		// Zoom out button pressed
		maximize();
		zoomResetVisible(false);
		
		//chartPanel.repaint();
		
	}//GEN-LAST:event_jButton3ActionPerformed

    private void jButton4ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButton4ActionPerformed
        
        // ratio menu > 14-th entry
		double alpha = 1.0;
        if (rtn_info != null) alpha = rtn_info.getAmortizedConstant();
		try {
			alpha = Double.parseDouble(javax.swing.JOptionPane.showInputDialog(main_window, "Enter alpha", alpha));
		} catch (NumberFormatException e) {
			javax.swing.JOptionPane.showMessageDialog(main_window, "alpha must be a decimal value");
			return;
		} catch (Exception e) {
			return;
		}

		if (alpha < 0) {
			javax.swing.JOptionPane.showMessageDialog(main_window, "alpha must be non negative");
            return;
		}
		
        
        rtn_info.InvalidAmortizedCache();
        rtn_info.setAmortizedConstant(alpha);
		refresh(true);
		//main_window.refreshTables();
    }//GEN-LAST:event_jButton4ActionPerformed

	private boolean filterRms(Rms t) {
		
		if (filters == null) return true;
		
		if (filters[0] != null) {
			try {
				double time = Double.parseDouble(filters[0]);
				if (t.getMaxCost() < time) return false;
			} catch (Exception e) {
				return true;
			}
		}
		
		if (filters[1] != null) {
			try {
				int rms = Integer.parseInt(filters[1]);
				if (t.getRms() < rms) return false;
			} catch (Exception e) {
				return true;
			}
		}
		
		if (filters[2] != null) {
			try {
				int freq = Integer.parseInt(filters[2]);
				if (t.getOcc() < freq) return false;
			} catch (Exception e) {
				return true;
			}
		}
		return true;
	}
	
	
    public void setRoutine(Routine r) {
		
        //System.out.println("Set routine");
        
		rtn_info = r;
		zoomResetVisible(false);
        
		disableNotification(true);
		for (int i = 0; i < series.length; i++) series[i].clear();
		disableNotification(false);
		
		if (this.rtn_info != null && graph_type != RTN_PLOT
                && graph_type != RATIO_TUPLES_PLOT
                && graph_type != EXTERNAL_INPUT_PLOT
                && graph_type != ALPHA_PLOT) {
			
			long max = rtn_info.getMaxRms();
			group_threshold_base = (int) Math.log10(max) + 1;
			smooth_threshold_base = (int) Math.log10(max) + 1;
			
			// reset temp global vars
			group_threshold = 1;
			jRadioButtonMenuItem1.setSelected(true);
			smooth_threshold = 1;
			jRadioButtonMenuItem10.setSelected(true);
			
			elem_slot = 0; slot_start = 0;
			sum_y = 0; sum_y2 = 0; sum_y3 = 0; sum_x = 0;
			sum_occ = 0;
			
            if (rtn_info.getSizeRmsList() <= 0) {
                
                jRadioButtonMenuItem1.setText("0");
                jRadioButtonMenuItem2.setText("0");
                jRadioButtonMenuItem3.setText("0");
                jRadioButtonMenuItem4.setText("0");
                jRadioButtonMenuItem5.setText("0");
                jRadioButtonMenuItem6.setText("0");
                jRadioButtonMenuItem10.setText("0");
                jRadioButtonMenuItem11.setText("0");
                jRadioButtonMenuItem12.setText("0");
                jRadioButtonMenuItem13.setText("0");
                jRadioButtonMenuItem14.setText("0");
                jRadioButtonMenuItem15.setText("0");
                
                return;
                
            }

             jRadioButtonMenuItem1.setText(Integer.toString((int)Math.pow(group_threshold_base, 0)));
             jRadioButtonMenuItem2.setText(Integer.toString((int)Math.pow(group_threshold_base, 1)));
             jRadioButtonMenuItem3.setText(Integer.toString((int)Math.pow(group_threshold_base, 2)));
             jRadioButtonMenuItem4.setText(Integer.toString((int)Math.pow(group_threshold_base, 3)));
             jRadioButtonMenuItem5.setText(Integer.toString((int)Math.pow(group_threshold_base, 4)));
             jRadioButtonMenuItem6.setText(Integer.toString((int)Math.pow(group_threshold_base, 5)));

            
			int val = (int)Math.pow(smooth_threshold_base, 0);
            if (rtn_info.getSizeRmsList() > 0 && val >= rtn_info.getSizeRmsList()) {
                val = rtn_info.getSizeRmsList() - 1;
            }
			if (rtn_info.getSizeRmsList() > 0 && val % 2 == 0) val++; // we want an odd number...
			jRadioButtonMenuItem10.setText(Integer.toString(val));

			val = (int)Math.pow(smooth_threshold_base, 1);
            if (rtn_info.getSizeRmsList() > 0 && val >= rtn_info.getSizeRmsList()) {
                val = rtn_info.getSizeRmsList() - 1;
            }
			if (rtn_info.getSizeRmsList() > 0 && val % 2 == 0) val++;
			jRadioButtonMenuItem11.setText(Integer.toString(val));

			val = (int)Math.pow(smooth_threshold_base, 2);
			if (rtn_info.getSizeRmsList() > 0 && val >= rtn_info.getSizeRmsList()) {
                val = rtn_info.getSizeRmsList() - 1;
            }
            if (rtn_info.getSizeRmsList() > 0 && val % 2 == 0) val++;
			jRadioButtonMenuItem12.setText(Integer.toString(val));

			val = (int)Math.pow(smooth_threshold_base, 3);
            if (rtn_info.getSizeRmsList() > 0 && val >= rtn_info.getSizeRmsList()) {
                val = rtn_info.getSizeRmsList() - 1;
            }
            if (rtn_info.getSizeRmsList() > 0 && val % 2 == 0) val++;
			jRadioButtonMenuItem13.setText(Integer.toString(val));

			val = (int)Math.pow(smooth_threshold_base, 4);
			if (rtn_info.getSizeRmsList() > 0 && val >= rtn_info.getSizeRmsList()) {
                val = rtn_info.getSizeRmsList() - 1;
            }
            if (rtn_info.getSizeRmsList() > 0 && val % 2 == 0) val++;
			jRadioButtonMenuItem14.setText(Integer.toString(val));

			val = (int)Math.pow(smooth_threshold_base, 5);
			if (rtn_info.getSizeRmsList() > 0 && val >= rtn_info.getSizeRmsList()) {
                val = rtn_info.getSizeRmsList() - 1;
            }
            if (rtn_info.getSizeRmsList() > 0 && val % 2 == 0) val++;
			jRadioButtonMenuItem15.setText(Integer.toString(val));
			
		}
		
	}
	
	private double getY(Rms te, int slot) {
		
        //System.out.println("Set getY");
        
		double y = 0;
		if (te == null) return y;
		
		if (this.graph_type == GraphPanel.COST_PLOT)
			y = te.getCost(cost_type); 
		
		else if (this.graph_type == GraphPanel.RATIO_PLOT) 
			y = te.getRatioCost(cost_type);
		
		else if (this.graph_type == GraphPanel.TOTALCOST_PLOT) 
			y = te.getTotalCost();
		
		else if (this.graph_type == GraphPanel.VAR_PLOT) 
			y = te.getVar();

		else if (this.graph_type == GraphPanel.MMM_PLOT) {

			if (slot == 0) 
				y = te.getMinCost();
			else if (slot == 1)
				y = te.getAvgCost();
			else if (slot == 2)
				y = te.getMaxCost();
			
		} else if (graph_type == AMORTIZED_PLOT) 
			y = rtn_info.getAmortizedValue(te.getRms());
			
		else if (this.graph_type == GraphPanel.FREQ_PLOT)
			y = te.getOcc();
		
		else
			throw new RuntimeException("Not defined getY behavior: " + graph_type);
		
		return y;
	}
	
	public void addPoint(Rms te) {
					
		double x = te.getRms();
		double y = 0;
        
		if (group_threshold == 1 && smooth_threshold == 1) { 

			if (filterRms(te)) {

				if (graph_type == MMM_PLOT) {

					series[0].add(x, getY(te, 0), false);
					series[5].add(x, getY(te, 1), false);
					series[11].add(x, getY(te, 2), false);

				} else if (graph_type == FREQ_PLOT) {

                    int index = 0;
                    /*
                    if (main_window.hasDistinctRms())
                        index = (int)(((double)1-te.getRatioSumRmsRvms())*10);
					*/
                    series[index].add(x, getY(te, 0), false);
                
                } else if (graph_type == AMORTIZED_PLOT) {
                    
                    series[0].add(x, getY(te, 0), false);
                    
				} else {

					y = getY(te, 0);
					double index = Math.round(Math.log10(te.getOcc()) / Math.log10(2));
					if (index > 11) index = 11;
					if (index < 0) index = 0;
					series[(int)index].add(x, y, false);

				}

			}

		} else if (group_threshold > 1 && smooth_threshold == 1) {

			int current_slot = (int) ((int) x - (x % group_threshold));

			if (current_slot != slot_start) {

				// Ok, this rms is for a new slot, so we need to create
				// a point in the graph for the old slot, and reset temp
				// global counters and then add current value 

				if (elem_slot > 0) add_group_point();
				slot_start = current_slot;

			}

			// same slot, add value to current slot
			if (filterRms(te)) {

				sum_x += x;
				sum_y += getY(te, 0);
				sum_occ += te.getOcc(); // need for choosing color

				if (graph_type == MMM_PLOT) {
					sum_y2 += getY(te, 1);
					sum_y3 += getY(te, 2);
				}

				elem_slot++;

			}
			
			// Is this the last point?
			if (x == rtn_info.getMaxRms()) add_group_point();

		} else if (group_threshold == 1 && smooth_threshold > 1) {
			
			if (x == rtn_info.getMaxRms()) 
				this.setData(rtn_info);
			
			//throw new RuntimeException("Not yet implemented :(");
			
		} else {

			throw new RuntimeException("Group and smooth threshold cannot be both greater than one");

		}
		
	}
	
	private void add_group_point() {
		
		double mean_x = sum_x / elem_slot;
						
		if (graph_type == GraphPanel.FREQ_PLOT) {

			series[0].add(mean_x, sum_y, false);

		} else if (graph_type == MMM_PLOT) {

			double mean_y = sum_y / elem_slot;
			series[0].add(mean_x, mean_y, false);
			mean_y = sum_y2 / elem_slot;
			series[5].add(mean_x, mean_y, false);
			mean_y = sum_y3 / elem_slot;
			series[11].add(mean_x, mean_y, false);

		} else if (graph_type == AMORTIZED_PLOT) {

			double mean_y = sum_y / elem_slot;
			series[0].add(mean_x, mean_y, false);

		} else {

			double mean_y = sum_y / elem_slot;
			double index = Math.round(Math.log10(sum_occ) / Math.log10(2));
			if (index > 11) index = 11;
			if (index < 0) index = 0;
			series[(int)index].add(mean_x, mean_y, false);

		}

		sum_x = 0;
		sum_y = 0;
		sum_occ = 0; // need for choosing color

		if (graph_type == MMM_PLOT) {
			sum_y2 = 0;
			sum_y3 = 0;
		}

		elem_slot = 0;
		
	}
	
	/*
	 * populate this chart based on current routine
	 * NOTE: avoid use of this method when possible, this method will not "share"
	 *       any work with other active graphs (e.g. iteration over rms list)
	 */ 
    public void populateChart() {
		
		//System.out.println("Inside populate chart");
        //Thread.dumpStack();
        
		for (int i = 0; i < series.length; i++) series[i].clear();
		
        if (graph_type == AMORTIZED_PLOT) updateGraphTitle();
        
		if (graph_type == RTN_PLOT) {
			
			AprofReport report = main_window.getCurrentReport();
			if (report == null) return;
			double x = 0;
			double y = 0;
			long[] num_class_sms = report.getNumCallsClassSms();
			long[] tot_calls_class_sms = report.getTotCallsClassSms();
			long[] max_calls_class_sms = report.getMaxCallsClassSms();
			long most_called = report.getCallsHottestRoutine();
			double num_at_least = 0;
			
			for (int k = num_class_sms.length-1; k >= 0; k--) {
				
				if (num_class_sms[k] == 0) continue;
				x = Math.pow(2, k);
				num_at_least += num_class_sms[k];
				
				y = 100 * ((double) tot_calls_class_sms[k] / (double) report.getTotalCalls());
				series[0].add(x, y, false);

				y = 100 * ((double)((double) tot_calls_class_sms[k] / (double) num_class_sms[k])
										/ (double) most_called);
				series[1].add(x, y, false);
				
				y = 100 * ((double) num_class_sms[k] / (double)report.getRoutineCount());
				series[2].add(x, y, false);
			   
				y = 100 * ((double) max_calls_class_sms[k] / (double) most_called);
				series[3].add(x, y, false);
				
				y = 100 * ((double) num_at_least / (double)report.getRoutineCount());
				series[4].add(x, y, false);
			   
			}
			
			return;
		}
        
        if (graph_type == RATIO_TUPLES_PLOT) {
        
            AprofReport report = main_window.getCurrentReport();
			if (report == null) return;
            report.sortRoutinesByRatioTuples();
            ArrayList<Routine> rr = report.getRoutines();
			
            double x = 0;
			double y = 0;
            
            for (int i = 0; i < rr.size(); i++) {
                
                Routine r = rr.get(i);
                x = ((double)i) / (((double)rr.size())/100);
                
                if (main_window.hasDistinctRms()) {
                    y = r.getRatioRvmsRms();
                    /*
                    y = ((double)(r.getSizeRmsList() 
                                    - r.getCountRms()) ) /
                                    r.getCountRms();*/
                } else
                    y = 0;
                
                /*
                System.out.println(r.getName() + " " + x + " " + y + " " 
                        + r.getSizeRmsList() + " " + r.getCountRms());
                */
                series[0].add(x, y, false);
                
            }
            
            return;
        }
		
        if (graph_type == EXTERNAL_INPUT_PLOT) {
        
            AprofReport report = main_window.getCurrentReport();
			if (report == null) return;
            report.sortRoutinesByExternalInput();
            ArrayList<Routine> rr = report.getRoutines();
			
            double x = 0;
			double y = 0;
            
            for (int i = 0; i < rr.size(); i++) {
                
                Routine r = rr.get(i);
                x = ((double)i) / (((double)rr.size())/100);
                
                if (main_window.hasDistinctRms())
                    y = (1 - r.getRatioSumRmsRvms()) * 100;
                else
                    y = 0;
                
                /*
                System.out.println(r.getName() + " " + x + " " + y + " " 
                        + r.sumRms() + " " + r.sumRvms());
                */
                series[0].add(x, y, false);
                
            }
            
            return;
        
        } else if (graph_type == ALPHA_PLOT) {
            
            ArrayList<Double> list = rtn_info.estimateAmortizedConstant();
            for (int i = 0; i < list.size(); i += 2) {
                series[0].add(list.get(i), list.get(i+1), false);
                //System.out.println(list.get(i) + " " + list.get(i+1));
            }
            
            return;
        } 
        
		if (rtn_info == null || rtn_info.getSizeRmsList() <= 0) return;
		
		if (group_threshold == 1 && smooth_threshold == 1) {
			
            //System.out.println("START################");
            
			//System.out.println("GroupThreshold == 1");
			Iterator rms_list = this.rtn_info.getRmsListIterator();
			while (rms_list.hasNext()) {
				
				Rms te = (Rms) rms_list.next();
				addPoint(te);
				
			}
			
			//System.out.println("Terminated populate");
			
		} else if (group_threshold > 1 && smooth_threshold == 1) {
			
			//rtn_info.sortRmsListByAccesses();
			
			
			/*
			try {
			
			File tmp = null;
			PrintWriter out = null;
			if (graph_type == COST_PLOT) {
				AprofReport report = main_winmain_dow.getCurrentReport();
				tmp = new File(report.getName() + "-" + this.rtn_info.getName() + ".rprof_" + group_threshold);
				tmp.createNewFile();
				out = new PrintWriter(new FileWriter(tmp));
			}
			 */
			
			int n_y = 1;
			if (graph_type == MMM_PLOT) n_y = 3;
			
			double slot_start = 0;
			double sum_x = 0;
			double sum_y = 0, sum_y2 = 0, sum_y3 = 0;
			long sum_occurrences = 0;
			int t = group_threshold;
			int k = 0;
			Iterator rms_list = this.rtn_info.getRmsListIterator();
			while(rms_list.hasNext()) {
				
				Rms te = (Rms) rms_list.next();
				if (filterRms(te)) {
					double x = te.getRms();
					double y = 0, y2 = 0, y3 = 0;
					if (graph_type == GraphPanel.COST_PLOT)
						y = te.getCost(cost_type); 
					else if (graph_type == GraphPanel.RATIO_PLOT) 
						y = te.getRatioCost(cost_type);
					else if (graph_type == GraphPanel.TOTALCOST_PLOT) 
						y = te.getTotalCost();
					else if (graph_type == VAR_PLOT) 
						y = te.getVar();
					else if (graph_type == AMORTIZED_PLOT)
						y = rtn_info.getAmortizedValue((int)x);
					else if (graph_type == FREQ_PLOT)
						y = te.getOcc();
					else if (graph_type == MMM_PLOT) {
						y = te.getMinCost();
						y2 = te.getAvgCost();
						y3 = te.getMaxCost();
					}
					
					double current_slot = x - (x % t);
					
					if (current_slot == slot_start) {
						sum_x += x;
						sum_y += y;
						if (n_y >= 2) sum_y2 += y2;
						if (n_y >= 3) sum_y3 += y3;
						sum_occurrences += te.getOcc();
						k++;
					} else {
						if (k > 0) {
							double mean_x = (k == 0) ? sum_x : sum_x / k;
							if (graph_type == GraphPanel.FREQ_PLOT) {
								series[0].add(mean_x, sum_y);
								if (sum_y > max_y) max_y = sum_y;
							} else if (graph_type == MMM_PLOT) {
								double mean_y = (k == 0) ? sum_y : sum_y / k;
								double mean_y2 = (k == 0) ? sum_y2 : sum_y2 / k;
								double mean_y3 = (k == 0) ? sum_y3 : sum_y3 / k;
								series[0].add(mean_x, mean_y);
								series[5].add(mean_x, mean_y2);
								series[11].add(mean_x, mean_y3);
							} else if (graph_type == AMORTIZED_PLOT) {
								double mean_y = (k == 0) ? sum_y : sum_y / k;
								series[0].add(mean_x, mean_y);
							} else {
								double mean_y = (k == 0) ? sum_y : sum_y / k;
								double index = Math.round(Math.log10(sum_occurrences) / Math.log10(2));
								if (index > 11) index = 11;
								if (index < 0) index = 0;
								series[(int)index].add(mean_x, mean_y);
								/*
								if (graph_type == COST_PLOT)
									out.println(mean_x + " " + mean_y);
								 */
							}
							sum_x = x;
							sum_y = y;
							if (n_y >= 2) sum_y2 = y2;
							if (n_y >= 3) sum_y3 = y3;
							sum_occurrences = te.getOcc();
							k = 1;
						}
						slot_start = current_slot;
					}
				}
			}
			if (k > 0) {
				double mean_x = (k == 0) ? sum_x : sum_x / k;
				if (graph_type == GraphPanel.FREQ_PLOT) {
					series[0].add(mean_x, sum_y, false);
					if (sum_y > max_y) max_y = sum_y;
				} else if (graph_type == MMM_PLOT) {
					double mean_y = (k == 0) ? sum_y : sum_y / k;
					double mean_y2 = (k == 0) ? sum_y2 : sum_y2 / k;
					double mean_y3 = (k == 0) ? sum_y3 : sum_y3 / k;
					series[0].add(mean_x, mean_y, false);
					series[5].add(mean_x, mean_y2, false);
					series[11].add(mean_x, mean_y3, false);
				} else if (graph_type == AMORTIZED_PLOT) {
					double mean_y = (k == 0) ? sum_y : sum_y / k;
					series[0].add(mean_x, mean_y, false);
				} else {
					double mean_y = (k == 0) ? sum_y : sum_y / k;
					double index = Math.round(Math.log10(sum_occurrences) / Math.log10(2));
					if (index > 11) index = 11;
					if (index < 0) index = 0;
					series[(int)index].add(mean_x, mean_y, false);
					
					/*
					if (graph_type == COST_PLOT)
						out.println(mean_x + " " + mean_y);
					 */
				}
			}
			/*
			out.close();
			
			} catch(java.io.IOException e) {
			}
			*/	
			
			
		
		} else if (smooth_threshold > 1 && group_threshold == 1) {
			
			int t = (int) Math.pow(smooth_threshold_base, smooth_threshold-1);
			if (t >= rtn_info.getSizeRmsList()) t = rtn_info.getSizeRmsList() - 1;
			if (t % 2 == 0) t++;
			// rms items within window; circular array; +2 avoid immediate overwrite
			Rms[] l = new Rms[t + 2]; 
			// amortized cache
			double[] lm = new double[t + 2];
			double sum = 0, sum2 = 0, sum3 = 0;
			rtn_info.sortRmsListByAccesses();
			Iterator rms_list = rtn_info.getRmsListIterator();
			
			int tail = 0;
			int current = 0;
			int head = 0;
			double n = 0;
			
			while(true) {	
				
				int add = 0;
				Rms removed = null;
				
				if (n == t || n == 0) add = 1;
				else add = 2;
				
				int c = 0;
				while (add > 0) {
					
					Rms te = null;
					
					// extract a valid rms
					while (rms_list.hasNext()) {
						
						te = (Rms) rms_list.next();
						if (filterRms(te)) break;
					
					}
					
					if (te == null) break;
					
					double y = getY(te, 0);
					
					// add to the list
					l[head % l.length] = te;
					if (graph_type == AMORTIZED_PLOT) 
						lm[head % lm.length] = y;
					
					head++;
					
					// update sum
					sum += y;
					if (graph_type == MMM_PLOT) {
						
						sum2 += getY(te, 1);
						sum3 += getY(te, 2);
						
					}
					
					c++;
					add--;
					//System.out.println("Aggiungo elemento");
					
				}
				
				if (n >= t || c == 0) {
					
					// update sum
					if (graph_type == AMORTIZED_PLOT) {
					
						double amortized_old = lm[tail++ % l.length];
						sum -= amortized_old;
						
					} else {
					
						//System.out.println("Rimuovo elemento  - tail: " + tail);
						removed = l[tail % l.length];
						tail++;
						//System.out.println("tail incremented... " +  tail);
						
						sum -= getY(removed, 0);
						if (graph_type == MMM_PLOT) {

							sum2 -= getY(removed, 1);
							sum3 -= getY(removed, 2);

						}
					
					}
					
					//System.out.println("Sottratto: " +  getY(removed, 0));
					n--;
					
					if (add > 0) {

						// update sum
						if (graph_type == AMORTIZED_PLOT) {

							double amortized_old = lm[tail++ % l.length];
							sum -= amortized_old;

						} else {
						
							//System.out.println("Rimuovo elemento - tail: " + tail);
							removed = l[tail % l.length];
							tail++;
							//System.out.println("tail incremented... " +  tail);

							sum -= getY(removed, 0);
							if (graph_type == MMM_PLOT) {

								sum2 -= getY(removed, 1);
								sum3 -= getY(removed, 2);

							}
						
						}
						
						n--;
						
					}
					
				}
				
				n += c;
				
				//System.out.println("Aggiungo punto - current: " +  current + " head: " + head);
				double x = l[current % l.length].getRms();
                    
				if (graph_type == MMM_PLOT) {

					series[0].add(x, sum/n, false);
					series[5].add(x, sum2/n, false);
					series[11].add(x, sum3/n, false);

				} else if (graph_type == AMORTIZED_PLOT || graph_type == FREQ_PLOT) {

					series[0].add(x, sum/n, false);

				} else {

					double index = Math.round(Math.log10(l[current % l.length].getOcc()) / Math.log10(2));
					if (index > 11) index = 11;
					if (index < 0) index = 0;
					series[(int)index].add(x, sum/n, false);

					/*
					if (graph_type == COST_PLOT)
						System.out.println("Sum is now: " + sum + " with n: " + n + 
								" current value: " + getY(l[current % l.length], 0) +
								 " removed value: " +  getY(removed, 0)
								+ " tail: " + tail);
					*/
				}
				
				if (current++ != 0 && n == 1) break;
				//System.out.println("Current incremented... " + current);
				
			}
			
		} else {
			
			throw new RuntimeException("Smooth and group threshold cannot be both greater than 1");
		
		}

	}

	public void setPerformanceMonitor(PerformanceMonitor p) {
		
		perf = p;
		chart.addProgressListener(p);
		
	}
	
	
	/*
	 * When possibile avoid use of this method, better share elaboration
	 * between all active graphs (e.g. iteration over rms list)
	 */
	public void setData(Routine r) {
		
		if (r == null && graph_type != RTN_PLOT
                && graph_type != RATIO_TUPLES_PLOT
                && graph_type != EXTERNAL_INPUT_PLOT) {
			clearData();
			return;
		}
		
		setRoutine(r);
		refresh(true);

	}

	public void clearData() {
		
		disableNotification(true);
		
        rtn_info = null;
        
		for (int i = 0; i < series.length; i++) series[i].clear();

        max_x = domainAxis.getUpperBound();
		max_y = rangeAxis.getUpperBound();
        
		if (graph_type == RTN_PLOT
            || graph_type == RATIO_TUPLES_PLOT
            || graph_type == EXTERNAL_INPUT_PLOT
            || graph_type == ALPHA_PLOT) {
            
			min_x = -1;
			min_y = -1;
		
        } else {
			min_x = domainAxis.getLowerBound();
			min_y = rangeAxis.getLowerBound();
		}
		
        updateXAxis(true);
		updateYAxis(true);

		disableNotification(false);
		
		System.gc();
		
	}

	
    private void refresh(boolean resetAxis) {
		
		if (perf != null) perf.start(this, PerformanceMonitor.ELABORATE);
		
		disableNotification(true);
		populateChart();
        disableNotification(false);
        
		if (resetAxis) maximize();
		
		if (perf != null) perf.stop(this, PerformanceMonitor.ELABORATE);
		
		System.gc();
		
	}
	
	private void disableNotification(boolean disable) {
		
		if (disable) {
		
			chart.setNotify(false);
			for (int i = 0; i < series.length; i++) series[i].setNotify(false);
			
		} else {
		
			for (int i = 0; i < series.length; i++) series[i].setNotify(true);
			chart.setNotify(true);
			
		}
		
	}

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JButton jButton1;
    private javax.swing.JButton jButton2;
    private javax.swing.JButton jButton3;
    private javax.swing.JButton jButton4;
    private javax.swing.JLabel jLabel2;
    private javax.swing.JMenuItem jMenuItem1;
    private javax.swing.JMenuItem jMenuItem10;
    private javax.swing.JMenuItem jMenuItem11;
    private javax.swing.JMenuItem jMenuItem12;
    private javax.swing.JMenuItem jMenuItem13;
    private javax.swing.JMenuItem jMenuItem14;
    private javax.swing.JMenuItem jMenuItem2;
    private javax.swing.JMenuItem jMenuItem3;
    private javax.swing.JMenuItem jMenuItem4;
    private javax.swing.JMenuItem jMenuItem5;
    private javax.swing.JMenuItem jMenuItem6;
    private javax.swing.JMenuItem jMenuItem7;
    private javax.swing.JMenuItem jMenuItem8;
    private javax.swing.JMenuItem jMenuItem9;
    private javax.swing.JPanel jPanel1;
    private javax.swing.JPopupMenu jPopupMenu1;
    private javax.swing.JPopupMenu jPopupMenu2;
    private javax.swing.JPopupMenu jPopupMenu3;
    private javax.swing.JPopupMenu jPopupMenu4;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem1;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem10;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem11;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem12;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem13;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem14;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem15;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem2;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem3;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem4;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem5;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem6;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem7;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem8;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem9;
    private javax.swing.JToggleButton jToggleButton1;
    private javax.swing.JToggleButton jToggleButton2;
    private javax.swing.JToggleButton jToggleButton3;
    private javax.swing.JToggleButton jToggleButton4;
    private javax.swing.JToggleButton jToggleButton6;
    private javax.swing.JToggleButton jToggleButton7;
    private javax.swing.JToolBar jToolBar1;
    // End of variables declaration//GEN-END:variables
	private javax.swing.ButtonGroup groupMenuButtonGroup;
	private javax.swing.ButtonGroup groupMenuButtonGroup2;
	private javax.swing.ButtonGroup groupMenuButtonGroup3;
	private javax.swing.ButtonGroup toggleButtonGroup;
}
