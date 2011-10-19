/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * TimeGraphPanel.java
 *
 * Created on 22-apr-2010, 19.15.12
 */

package aprofplot.gui;

import aprofplot.*;
import java.awt.BasicStroke;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Polygon;
import java.awt.Shape;
import java.awt.geom.Rectangle2D;
import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.LegendItem;
import org.jfree.chart.LegendItemCollection;
import org.jfree.chart.axis.LogarithmicAxis;
import org.jfree.chart.axis.NumberAxis;
import org.jfree.chart.axis.ValueAxis;
import org.jfree.chart.plot.DefaultDrawingSupplier;
import org.jfree.chart.plot.DrawingSupplier;
import org.jfree.chart.plot.Plot;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYItemRenderer;
import org.jfree.chart.renderer.xy.XYLine3DRenderer;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.chart.renderer.xy.XYSplineRenderer;
import org.jfree.chart.title.TextTitle;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;
import org.jfree.ui.RectangleInsets;

/**
 *
 * @author bruno
 */
public class GraphPanel extends javax.swing.JPanel {

    public static final int TIME_PLOT = 0;
    public static final int RATIO_PLOT = 1;
    public static final int FREQ_PLOT = 2;
    public static final int MMM_PLOT = 3;
    public static final int SUM_PLOT = 4;
    public static final int VAR_PLOT = 5;
    public static final int RTN_PLOT = 6;

    public static final int MAX_COST = 0;
    public static final int AVG_COST = 1;
    public static final int MIN_COST = 2;

    private int graph_type;
    private int cost_type;
    private MainWindow main_window;
    private boolean x_log_scale;
    private boolean y_log_scale;
    private boolean maximized;
    private boolean autoscaled;
    private boolean magnified;
    private boolean dragging;
    private int group_threshold;
    private double min_x, min_y, max_x, max_y;
    private String[] filters;
    private RoutineInfo rtn_info;
    private AprofReport report = null;

    private XYPlot plot;
    private JFreeChart chart;
    private ValueAxis domainAxis, rangeAxis;
    private XYItemRenderer renderer;
    private ChartPanel chartPanel;
    private XYSeriesCollection data;
    private XYSeries[] series;
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

    /** Creates new form TimeGraphPanel */
    public GraphPanel(int graph_type, MainWindow mw) {
        this.graph_type = graph_type;
        this.x_log_scale = false;
        this.y_log_scale = false;
        if (graph_type == RTN_PLOT) {
            this.maximized = false;
            this.autoscaled = true;
        } else {
            this.maximized = true;
            this.autoscaled = false;
        }
        this.magnified = false;
        this.dragging = false;
        this.group_threshold = 1;
        this.main_window = mw;
        this.filters = main_window.getSmsTableFilter();
        if (graph_type == RTN_PLOT) {
            this.min_y = -10;
            this.min_x = -10;
        } else {
            this.min_y = 0;
            this.min_x = 0;
        }
        this.max_x = this.max_y = 10;

        Dimension d = new Dimension(370, 300);
        this.setPreferredSize(d);

        data = new XYSeriesCollection();
        series = new XYSeries[12];
        for (int i = 0; i < series.length; i++) {
            series[i] = new XYSeries(i + "");
        }
        for (int i = 0; i < series.length; i++) data.addSeries(series[i]);

        boolean legend = false;
        if (graph_type == RTN_PLOT || graph_type == MMM_PLOT)
            legend = true;
        chart = ChartFactory.createScatterPlot(null,
                                                  null,
                                                  null,
                                                  data,
                                                  PlotOrientation.VERTICAL,
                                                  legend,
                                                  false,
                                                  false);

        chart.setAntiAlias(false);
        plot = (XYPlot) chart.getPlot();
        /*
        if (graph_type == RTN_PLOT) {
            XYSplineRenderer rr = new XYSplineRenderer(100);
            renderer = (XYSplineRenderer) rr;
            plot.setRenderer(rr);
        } else {
            renderer = plot.getRenderer();
        }
         */
        renderer = plot.getRenderer();
        plot.setBackgroundPaint(Color.WHITE);
        plot.setAxisOffset(new RectangleInsets(0.0, 0.0, 0.0, 0.0));
        plot.setDomainGridlinePaint(Color.LIGHT_GRAY);
        plot.setRangeGridlinePaint(Color.LIGHT_GRAY);

        if (legend) {
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
                LegendItem legenditem2 = new LegendItem("% routines", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.BLUE);
                LegendItem legenditem3 = new LegendItem("% max calls", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, Color.GREEN);

                legenditemcollection.add(legenditem);
                legenditemcollection.add(legenditem1);
                legenditemcollection.add(legenditem3);
                legenditemcollection.add(legenditem2);
            }
            plot.setFixedLegendItems(legenditemcollection);
        }

        //System.out.println(renderer.getClass());
        for (int i = 0; i < series.length; i++) {
            renderer.setSeriesOutlinePaint(i, colors[i]);
            if (graph_type == RTN_PLOT) {

                renderer.setSeriesShape(i, new Rectangle2D.Double(-2.0, -2.0, 3.0, 3.0));
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
                    default:
                        renderer.setSeriesPaint(i, colors[i]);
                        r.setSeriesOutlinePaint(i, colors[i]);
                        break;
                }
                r.setUseOutlinePaint(true);
            } else {
                renderer.setSeriesShape(i, new Rectangle2D.Double(-1.0, -1.0, 2.0, 2.0));
                renderer.setSeriesPaint(i, colors[i]);
                ((XYLineAndShapeRenderer)renderer).setUseOutlinePaint(true);
            }
        }
        ((XYLineAndShapeRenderer)renderer).setDrawOutlines(true);

        chartPanel = new ChartPanel(chart, true);
        chartPanel.setMouseZoomable(true, true);
        chartPanel.setPopupMenu(null);
        chartPanel.addMouseListener(new java.awt.event.MouseAdapter() {
            @Override
            public void mousePressed(java.awt.event.MouseEvent evt) {
            }
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
        this.add(chartPanel, BorderLayout.CENTER);

        if (graph_type == RTN_PLOT)
            this.setVisible(false);

        updateGraphTitle();
        updateXAxis();
        updateYAxis();
    }
    
    public void setReport(AprofReport r){
        report = r;
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
        jPanel1 = new javax.swing.JPanel();
        jPanel2 = new javax.swing.JPanel();
        jToggleButton1 = new javax.swing.JToggleButton();
        jToggleButton2 = new javax.swing.JToggleButton();
        jButton2 = new javax.swing.JButton();
        jToggleButton6 = new javax.swing.JToggleButton();
        jToggleButton7 = new javax.swing.JToggleButton();
        jButton1 = new javax.swing.JButton();
        jToggleButton3 = new javax.swing.JToggleButton();
        jPanel3 = new javax.swing.JPanel();
        jPanel4 = new javax.swing.JPanel();
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

        jRadioButtonMenuItem2.setText("5");
        jRadioButtonMenuItem2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem2ActionPerformed(evt);
            }
        });
        jPopupMenu1.add(jRadioButtonMenuItem2);

        jRadioButtonMenuItem3.setText("10");
        jRadioButtonMenuItem3.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem3ActionPerformed(evt);
            }
        });
        jPopupMenu1.add(jRadioButtonMenuItem3);

        jRadioButtonMenuItem4.setText("20");
        jRadioButtonMenuItem4.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem4ActionPerformed(evt);
            }
        });
        jPopupMenu1.add(jRadioButtonMenuItem4);

        jRadioButtonMenuItem5.setText("50");
        jRadioButtonMenuItem5.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jRadioButtonMenuItem5ActionPerformed(evt);
            }
        });
        jPopupMenu1.add(jRadioButtonMenuItem5);

        jRadioButtonMenuItem6.setText("100");
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

        setBorder(javax.swing.BorderFactory.createTitledBorder(""));
        setLayout(new java.awt.BorderLayout());

        jPanel1.setBorder(javax.swing.BorderFactory.createEmptyBorder(1, 1, 1, 1));
        jPanel1.setLayout(new java.awt.BorderLayout());

        jPanel2.setLayout(new javax.swing.BoxLayout(jPanel2, javax.swing.BoxLayout.LINE_AXIS));

        jToggleButton1.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Xlog-icon.png"))); // NOI18N
        jToggleButton1.setToolTipText("toggle x axis logarithmic scale");
        jToggleButton1.setBorderPainted(false);
        jToggleButton1.setFocusTraversalPolicyProvider(true);
        jToggleButton1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton1ActionPerformed(evt);
            }
        });
        jPanel2.add(jToggleButton1);

        jToggleButton2.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Ylog-icon.png"))); // NOI18N
        jToggleButton2.setToolTipText("toggle y axis logarithmic scale");
        jToggleButton2.setBorderPainted(false);
        jToggleButton2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton2ActionPerformed(evt);
            }
        });
        jPanel2.add(jToggleButton2);

        jButton2.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Maximize-icon.png"))); // NOI18N
        jButton2.setBorderPainted(false);
        jButton2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jButton2ActionPerformed(evt);
            }
        });
        jPanel2.add(jButton2);

        jToggleButton6.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Incorporate-icon.png"))); // NOI18N
        jToggleButton6.setToolTipText("group graph points");
        jToggleButton6.setBorderPainted(false);
        jToggleButton6.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton6ActionPerformed(evt);
            }
        });
        //if (graph_type == FREQ_PLOT) jToggleButton6.setVisible(false);
        jPanel2.add(jToggleButton6);

        jToggleButton7.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/RatioType-icon.png"))); // NOI18N
        jToggleButton7.setToolTipText("select ratio type");
        jToggleButton7.setBorderPainted(false);
        jToggleButton7.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton7ActionPerformed(evt);
            }
        });
        if (graph_type != RATIO_PLOT) jToggleButton7.setVisible(false);
        jPanel2.add(jToggleButton7);

        jButton1.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/SavePlot-icon.png"))); // NOI18N
        jButton1.setToolTipText("export as PNG image");
        jButton1.setBorderPainted(false);
        jButton1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jButton1ActionPerformed(evt);
            }
        });
        jPanel2.add(jButton1);

        jToggleButton3.setFont(new java.awt.Font("Ubuntu", 1, 13));
        jToggleButton3.setText("T");
        jToggleButton3.setToolTipText("Select cost type");
        jToggleButton3.setBorderPainted(false);
        jToggleButton3.setMargin(new java.awt.Insets(0, 4, 0, 4));
        jToggleButton3.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jToggleButton3ActionPerformed(evt);
            }
        });
        if (graph_type == TIME_PLOT || graph_type == RATIO_PLOT)
        jPanel2.add(jToggleButton3);

        jPanel1.add(jPanel2, java.awt.BorderLayout.WEST);
        jPanel1.add(jPanel3, java.awt.BorderLayout.CENTER);

        if (graph_type == FREQ_PLOT || graph_type == MMM_PLOT || graph_type == RTN_PLOT)
        jLabel2.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Dummy.png"))); // NOI18N
        else {
            jLabel2.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Color-scale.png"))); // NOI18N
            //jLabel2.setBorder(new javax.swing.border.LineBorder(java.awt.Color.BLACK));
        }
        jPanel4.add(jLabel2);

        jPanel1.add(jPanel4, java.awt.BorderLayout.EAST);

        add(jPanel1, java.awt.BorderLayout.NORTH);
    }// </editor-fold>//GEN-END:initComponents

    private void chartMouseReleased() {
        if (this.dragging) {
            double new_min_x = domainAxis.getLowerBound();
            double new_max_x = domainAxis.getUpperBound();
            double new_min_y = domainAxis.getLowerBound();
            double new_max_y = domainAxis.getUpperBound();
            if (min_x != new_min_x || max_x != new_max_x || min_y != new_min_y || max_y != new_max_y) {
                //System.out.println("Chart zoomed");
                setViewBounds(new_min_x, new_max_x, new_min_y, new_max_y);
                if (this.main_window.getLinkPlots()) {
                    
                }
            }
            this.dragging = false;
        }
    }

    private void chartMouseDragged() {
        this.dragging = true;
    }

    private void resetXAxis() {
        String label = null;
        if (this.graph_type == RTN_PLOT) label = "Number of distinct SMS";
        else label = "seen memory size";
        if (x_log_scale) {
            LogarithmicAxis newDomainAxis = new LogarithmicAxis(label);
            newDomainAxis.setAllowNegativesFlag(false);
            //NumberFormat nf = NumberFormat.getNumberInstance();
            newDomainAxis.setStrictValuesFlag(false);
            this.domainAxis = newDomainAxis;
        }
        else {
            NumberAxis newDomainAxis = new NumberAxis(label);
            this.domainAxis = newDomainAxis;
        }
        this.domainAxis.setStandardTickUnits(NumberAxis.createIntegerTickUnits());
        plot.setDomainAxis(domainAxis);
    }

    private void resetYAxis() {

        String label = null;
        if (graph_type == FREQ_PLOT) label = "occurrences";
        else if (graph_type == RTN_PLOT) label = "Percentage";
        else label = "cost"; 
        if (y_log_scale) {
            LogarithmicAxis newRangeAxis = new LogarithmicAxis(label);
            newRangeAxis.setAllowNegativesFlag(false);
            //NumberFormat nf = NumberFormat.getNumberInstance();
            newRangeAxis.setStrictValuesFlag(false);
            this.rangeAxis = newRangeAxis;
        }
        else {
            NumberAxis newRangeAxis = new NumberAxis(label);
            this.rangeAxis = newRangeAxis;
        }
        if (graph_type == GraphPanel.FREQ_PLOT)
                this.rangeAxis.setStandardTickUnits(NumberAxis.createIntegerTickUnits());
        plot.setRangeAxis(rangeAxis);
    }

    private void updateXAxis() {
        String label = null;
        if (this.graph_type == RTN_PLOT) {
            label = "Number of distinct SMS";
        }
        else label = "seen memory size";

        if (x_log_scale) {
            LogarithmicAxis newDomainAxis = new LogarithmicAxis(label);
            newDomainAxis.setAllowNegativesFlag(false);
            //NumberFormat nf = NumberFormat.getNumberInstance();
            newDomainAxis.setStrictValuesFlag(false);
            this.domainAxis = newDomainAxis;
        }
        else {
            NumberAxis newDomainAxis = new NumberAxis(label);
            this.domainAxis = newDomainAxis;
        }
        this.domainAxis.setLowerBound(min_x);
        this.domainAxis.setUpperBound(max_x);
        this.domainAxis.setStandardTickUnits(NumberAxis.createIntegerTickUnits());
        plot.setDomainAxis(domainAxis);
    }

    private void updateYAxis() {

        String label = null;
        if (graph_type == FREQ_PLOT) label = "occurrences";
        else if (graph_type == RTN_PLOT) label = "Percentage";
        else label = "cost";

        if (y_log_scale) {
            LogarithmicAxis newRangeAxis = new LogarithmicAxis(label);
            newRangeAxis.setAllowNegativesFlag(false);
            //NumberFormat nf = NumberFormat.getNumberInstance();
            newRangeAxis.setStrictValuesFlag(false);
            this.rangeAxis = newRangeAxis;
        }
        else {
            NumberAxis newRangeAxis = new NumberAxis(label);
            this.rangeAxis = newRangeAxis;
        }
        this.rangeAxis.setLowerBound(min_y);
        this.rangeAxis.setUpperBound(max_y);
        if (graph_type == GraphPanel.FREQ_PLOT)
                this.rangeAxis.setStandardTickUnits(NumberAxis.createIntegerTickUnits());
        plot.setRangeAxis(rangeAxis);
    }
    
    private void updateGraphTitle() {
        String s = "";
        switch (this.graph_type) {
            case TIME_PLOT: s = "Cost plot"; break;
            case FREQ_PLOT: s = "Frequency plot"; break;
            case MMM_PLOT: s = "Min/Avg/Max cost plot"; break;
            case SUM_PLOT: s = "Total cost plot"; break;
            case VAR_PLOT: s = "Variance plot"; break;
            case RTN_PLOT: s = "Routine plot"; break;
            case RATIO_PLOT: s = "Ratio plot - T(n) / ";
                            double[] rc = SmsEntry.getRatioConfig();
                            int n = 0;
                            for (int i =  0; i < rc.length; i++) {
                                if (rc[i] != 0) n++;
                            }
                            if (n > 1) s += "(";
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
                            if (n > 1) s += ")";
                            break;
        }
//        chart.setTitle(s);
        chart.setTitle(new TextTitle(s, new Font(Font.SERIF, Font.BOLD, 14)));
    }

    public void setViewBounds(double min_x, double max_x, double min_y, double max_y) {
        maximized = autoscaled = false;
        this.min_x = min_x;
        this.min_y = min_y;
        this.min_y = min_y;
        this.max_y = max_y;
        magnified = true;
        updateXAxis();
        updateYAxis();
    }

    public void setViewBounsFromTimePlotBBox(double min_x, double max_x, double min_y, double max_y) {

    }

    public void refreshFilter() {
        filters = main_window.getSmsTableFilter();
        populateChart();
    }

    public void setXLogScale(boolean logscale) {
        x_log_scale = logscale;
        jToggleButton1.setSelected(logscale);
        if (maximized) resetXAxis();
        else {
            if (graph_type == RTN_PLOT) autoscale();
            updateXAxis();
        }
    }

    public void setYLogScale(boolean logscale) {
        y_log_scale = logscale;
        jToggleButton2.setSelected(logscale);
        if (maximized) resetYAxis();
        else {
            if (graph_type == RTN_PLOT) autoscale();
            updateYAxis();
        }
    }

    public void setGroupCost(int cost_type) {
        switch(cost_type) {
            case MAX_COST:  jRadioButtonMenuItem7.setSelected(true);
                            break;
            case AVG_COST:  jRadioButtonMenuItem8.setSelected(true);
                            break;
            case MIN_COST:  jRadioButtonMenuItem9.setSelected(true);
                            break;
        }
        this.cost_type = cost_type;
        refresh();
    }

    public void setGroupThreshold(int t) {
        switch (t) {
            case 1: jRadioButtonMenuItem1.setSelected(true);
                    break;
            case 5: jRadioButtonMenuItem2.setSelected(true);
                    break;
            case 10: jRadioButtonMenuItem3.setSelected(true);
                    break;
            case 20: jRadioButtonMenuItem4.setSelected(true);
                    break;
            case 50: jRadioButtonMenuItem5.setSelected(true);
                    break;
            case 100: jRadioButtonMenuItem6.setSelected(true);
                    break;
        }
        group_threshold = t;
        populateChart();
        if (group_threshold == 1) {
            if (autoscaled) autoscale();
            else if (maximized) maximize();
        }
        updateXAxis();
        updateYAxis();
    }

    public void maximize() {
        autoscaled = false;
        maximized = true;
        magnified = false;
        resetXAxis();
        resetYAxis();
        
        max_x = domainAxis.getUpperBound();
        if (graph_type == RTN_PLOT) {
            min_x = -1;
            min_y = -10;
        } else {
            min_x = domainAxis.getLowerBound();
            min_y = rangeAxis.getLowerBound();
        }
        max_y = rangeAxis.getUpperBound();
    }

    private int sumOccurrences(int first, int last) {
        int sum = 0;
        for (int i = first; i <= last; i++) sum += rtn_info.getTimeEntries().get(i).getOcc();
        return sum;
    }

    public void autoscale() {
        
        autoscaled = true;
        maximized = false;
        magnified = false;
        
        if (graph_type == RTN_PLOT && report != null) {
            
            for (int q = 0; q < report.num_class_sms.length; q++) {
                if (report.num_class_sms[q] > 0) 
                    max_x = Math.pow(2, q);
            }
            max_x += max_x / 10;
            
            if (x_log_scale) min_x = 0;
            else min_x = - (max_x / 10);
            if (y_log_scale) min_y = 0;
            else min_y = -10;
            
            max_y = 110;
            updateXAxis();
            updateYAxis();
            return;
        }
        
        if (rtn_info != null && rtn_info.getTimeEntries().size() > 0) {
            int first, last, middle;
            int totalCalls = rtn_info.getTotalCalls();
            switch (this.graph_type) {
                case TIME_PLOT:
                case MMM_PLOT:
                                rtn_info.sortTimeEntriesByTime();
                                break;
                case SUM_PLOT:
                                rtn_info.sortTimeEntriesBySum();
                                break;
                case VAR_PLOT:
                                rtn_info.sortTimeEntriesByVar();
                                break;
                case RATIO_PLOT: rtn_info.sortTimeEntriesByRatio();
                                 break;
                case FREQ_PLOT: rtn_info.sortTimeEntriesByOccurrences();
                                break;
            }
            first = 0;
            last = rtn_info.getTimeEntries().size() - 1;
            middle = (int)Math.ceil(((double)last - (double)first) / 2);
            int c = sumOccurrences(0, middle);
            while (c != 0.9*totalCalls && (last - middle > 0 && middle - first > 0)) {
                if (c < 0.9*totalCalls) {
                    first =  middle;
                    middle += (int)Math.ceil(((double)last - (double)middle) / 2);
                    c = sumOccurrences(0, middle);
                }
                else {
                    last =  middle;
                    middle -= (int)Math.ceil(((double)middle - (double)first) / 2);
                    c = sumOccurrences(0, middle);
                }
            }
            switch (this.graph_type) {
                case TIME_PLOT: max_y = rtn_info.getTimeEntries().get(middle).getCost();
//                                int i = 0;
//                                while ((min_y = rtn_info.getTimeEntries().get(i).getCost()) == 0 && i < rtn_info.getTimeEntries().size()) i++;
                                break;
                case MMM_PLOT:  max_y = rtn_info.getTimeEntries().get(middle).getMaxCost();
                                break;
                case RATIO_PLOT: max_y = rtn_info.getTimeEntries().get(middle).getRatio();
//                                i = 0;
//                                while ((min_y = rtn_info.getTimeEntries().get(i).getRatio()) == 0 && i < rtn_info.getTimeEntries().size()) i++;
                                break;
                case FREQ_PLOT: max_y = rtn_info.getTimeEntries().get(middle).getOcc();
//                                i = 0;
//                                while ((min_y = rtn_info.getTimeEntries().get(i).getOcc()) == 0 && i < rtn_info.getTimeEntries().size()) i++;
                                break;
                case SUM_PLOT:
                                max_y = rtn_info.getTimeEntries().get(last).getSumCost();
                                break;
                case VAR_PLOT:
                                max_y = rtn_info.getTimeEntries().get(last).getVar();
                                break;
                case RTN_PLOT:
                                max_y = 110;
                                break;
            }

            rtn_info.sortTimeEntriesByAccesses();
//            int i = 0;
//            while ((min_x = rtn_info.getTimeEntries().get(i).getSms()) == 0 && i < rtn_info.getTimeEntries().size()) i++;
            first = 0;
            last = rtn_info.getTimeEntries().size() - 1;
            middle = (int)Math.ceil(((double)last - (double)first) / 2);
            c = sumOccurrences(0, middle);
            while (c != 0.9*totalCalls && (last - middle > 0 && middle - first > 0)) {
                if (c < 0.9*totalCalls) {
                    first =  middle;
                    middle += (int)Math.ceil(((double)last - (double)middle) / 2);
                    c = sumOccurrences(0, middle);
                }
                else {
                    last =  middle;
                    middle -= (int)Math.ceil(((double)middle - (double)first) / 2);
                    c = sumOccurrences(0, middle);
                }
            }
            if (graph_type == RTN_PLOT) {
                for (int q = 0; q < report.num_class_sms.length; q++) {
                    if (report.num_class_sms[q] > 0) 
                        max_x = Math.pow(2, q) + 2;
                }
            } else
                max_x = rtn_info.getTimeEntries().get(middle).getSms();
            
            if (graph_type == RTN_PLOT) {
                min_y = -10;
                min_x = -1;
            } else {
                min_y = 0;
                min_x = 0;
            }
            updateXAxis();
            updateYAxis();
        }
    }

    private void jToggleButton1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton1ActionPerformed

        boolean logscale;
        if (jToggleButton1.isSelected()) logscale = true;
        else logscale = false;
        setXLogScale(logscale);
        if (this.main_window.getLinkPlots()) main_window.setXLogScaleAll(graph_type, logscale);
    }//GEN-LAST:event_jToggleButton1ActionPerformed

    private void jToggleButton2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton2ActionPerformed
       
        boolean logscale;
        if (jToggleButton2.isSelected()) logscale = true;
        else logscale = false;
        setYLogScale(logscale);
        if (this.main_window.getLinkPlots()) main_window.setYLogScaleAll(graph_type, logscale);
    }//GEN-LAST:event_jToggleButton2ActionPerformed

    private void jToggleButton6ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton6ActionPerformed
        
        if (jToggleButton6.isSelected()) {
            jPopupMenu1.show(jToggleButton6, 0, jToggleButton6.getHeight());
        }
        else {
            jPopupMenu1.setVisible(false);
        }
    }//GEN-LAST:event_jToggleButton6ActionPerformed

    private void jPopupMenu1PopupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {//GEN-FIRST:event_jPopupMenu1PopupMenuWillBecomeInvisible
        
        jToggleButton6.setSelected(false);
    }//GEN-LAST:event_jPopupMenu1PopupMenuWillBecomeInvisible

    private void jRadioButtonMenuItem1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem1ActionPerformed
        
        setGroupThreshold(1);
        if (this.main_window.getLinkPlots()) main_window.setGroupThresholdAll(graph_type, 1);
    }//GEN-LAST:event_jRadioButtonMenuItem1ActionPerformed

    private void jRadioButtonMenuItem2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem2ActionPerformed
        
        setGroupThreshold(5);
        if (this.main_window.getLinkPlots()) main_window.setGroupThresholdAll(graph_type, 5);
    }//GEN-LAST:event_jRadioButtonMenuItem2ActionPerformed

    private void jRadioButtonMenuItem3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem3ActionPerformed
        
        setGroupThreshold(10);
        if (this.main_window.getLinkPlots()) main_window.setGroupThresholdAll(graph_type, 10);
    }//GEN-LAST:event_jRadioButtonMenuItem3ActionPerformed

    private void jRadioButtonMenuItem4ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem4ActionPerformed
        
        setGroupThreshold(20);
        if (this.main_window.getLinkPlots()) main_window.setGroupThresholdAll(graph_type, 20);
    }//GEN-LAST:event_jRadioButtonMenuItem4ActionPerformed

    private void jRadioButtonMenuItem5ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem5ActionPerformed
       
        setGroupThreshold(50);
        if (this.main_window.getLinkPlots()) main_window.setGroupThresholdAll(graph_type, 50);
    }//GEN-LAST:event_jRadioButtonMenuItem5ActionPerformed

    private void jRadioButtonMenuItem6ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem6ActionPerformed
       
        setGroupThreshold(100);
        if (this.main_window.getLinkPlots()) main_window.setGroupThresholdAll(graph_type, 100);
    }//GEN-LAST:event_jRadioButtonMenuItem6ActionPerformed

    private void jButton1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButton1ActionPerformed
        
        if (this.rtn_info == null) return;
        String filename = this.rtn_info.getName();
        if (this.graph_type == TIME_PLOT) filename += "_time_plot.png";
        else if (this.graph_type == MMM_PLOT) filename += "_mmm_plot.png";
        else if (this.graph_type == SUM_PLOT) filename += "_sum_plot.png";
        else if (this.graph_type == VAR_PLOT) filename += "_var_plot.png";
        else if (this.graph_type == RATIO_PLOT) filename += "_ratio_plot.png";
        else if (this.graph_type == RTN_PLOT) filename += "_rtn_plot.png";
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
            }
            catch (Exception e) {

            }
        }
    }//GEN-LAST:event_jButton1ActionPerformed

    private void jToggleButton7ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton7ActionPerformed
        
        if (jToggleButton7.isSelected()) {
            jPopupMenu2.show(jToggleButton7, 0, jToggleButton7.getHeight());
        }
        else {
            jPopupMenu2.setVisible(false);
        }
    }//GEN-LAST:event_jToggleButton7ActionPerformed

    private void jPopupMenu2PopupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {//GEN-FIRST:event_jPopupMenu2PopupMenuWillBecomeInvisible
        // TODO add your handling code here:
        jToggleButton7.setSelected(false);
    }//GEN-LAST:event_jPopupMenu2PopupMenuWillBecomeInvisible

    private void jMenuItem1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem1ActionPerformed
        // TODO add your handling code here:
        double[] rc = {1, 0, 0};
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem1ActionPerformed

    private void jMenuItem2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem2ActionPerformed
        // TODO add your handling code here:
        double[] rc = {1, 0, 1};
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem2ActionPerformed

    private void jMenuItem3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem3ActionPerformed
        // TODO add your handling code here:
        double[] rc = {1, 1, 0};
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem3ActionPerformed

    private void jMenuItem4ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem4ActionPerformed
        // TODO add your handling code here:
        double[] rc = {1.5, 0, 0};
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem4ActionPerformed

    private void jMenuItem5ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem5ActionPerformed
        // TODO add your handling code here:
        double[] rc = {2, 0, 0};
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem5ActionPerformed

    private void jMenuItem6ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem6ActionPerformed
        // TODO add your handling code here:
        double[] rc = {2.5, 0, 0};
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem6ActionPerformed

    private void jMenuItem7ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem7ActionPerformed
        // TODO add your handling code here:
        double[] rc = {3, 0, 0};
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem7ActionPerformed

    private void jMenuItem8ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem8ActionPerformed
        // TODO add your handling code here:
        double[] rc = SmsEntry.getRatioConfig();
        rc[2]--;
        if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
            return;
        }
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem8ActionPerformed

    private void jMenuItem9ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem9ActionPerformed
        // TODO add your handling code here:
        double[] rc = SmsEntry.getRatioConfig();
        rc[2]++;
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem9ActionPerformed

    private void jMenuItem10ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem10ActionPerformed
        // TODO add your handling code here:
        double[] rc = SmsEntry.getRatioConfig();
        rc[1]--;
        if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
            return;
        }
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem10ActionPerformed

    private void jMenuItem11ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem11ActionPerformed
        // TODO add your handling code here:
        double[] rc = SmsEntry.getRatioConfig();
        rc[1]++;
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem11ActionPerformed

    private void jMenuItem12ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem12ActionPerformed
        // TODO add your handling code here:
        double exp = 0;
        try {
            exp = Double.parseDouble(javax.swing.JOptionPane.showInputDialog(main_window, "Enter exponent"));
        }
        catch (NumberFormatException e) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "exponent must be a decimal value");
            return;
        }
        catch (Exception e) {
            return;
        }
        double[] rc = SmsEntry.getRatioConfig();
        rc[2] -= exp;
        if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
            return;
        }
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem12ActionPerformed

    private void jMenuItem13ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem13ActionPerformed
        // TODO add your handling code here:
        double exp = 0;
        try {
            exp = Double.parseDouble(javax.swing.JOptionPane.showInputDialog(main_window, "Enter exponent"));
        }
        catch (NumberFormatException e) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "exponent must be a decimal value");
            return;
        }
        catch (Exception e) {
            return;
        }
        double[] rc = SmsEntry.getRatioConfig();
        rc[1] -= exp;
        if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
            return;
        }
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem13ActionPerformed

    private void jMenuItem14ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem14ActionPerformed
        // TODO add your handling code here:
        double exp = 0;
        try {
            exp = Double.parseDouble(javax.swing.JOptionPane.showInputDialog(main_window, "Enter exponent"));
        }
        catch (NumberFormatException e) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "exponent must be a decimal value");
            return;
        }
        catch (Exception e) {
            return;
        }
        double[] rc = SmsEntry.getRatioConfig();
        rc[0] -= exp;
        if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
            return;
        }
        SmsEntry.setRatioConfig(rc);
        updateGraphTitle();
        refresh();
        main_window.refreshTables();
    }//GEN-LAST:event_jMenuItem14ActionPerformed

    private void jButton2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButton2ActionPerformed
        
        if (!maximized) {
            jButton2.setToolTipText("maximize graph");
            jButton2.setIcon(new javax.swing.
                    ImageIcon(getClass()
                    .getResource("/aprofplot/gui/resources/Maximize-icon.png"))); // NOI18N
            maximize();
        } else {
            autoscale();
            jButton2.setToolTipText("autoscale graph");
            jButton2.setIcon(new javax.swing.
                    ImageIcon(getClass()
                    .getResource("/aprofplot/gui/resources/Minimize-icon.png"))); // NOI18N

        }
//        graphArea.removeMouseListeners();
        if (this.main_window.getLinkPlots()) main_window.maximizeAll(graph_type);
    }//GEN-LAST:event_jButton2ActionPerformed

    private void jToggleButton3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jToggleButton3ActionPerformed
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
        if (this.main_window.getLinkPlots()) main_window.setGroupCostAll(graph_type, cost_type);
        refresh();
    }//GEN-LAST:event_jRadioButtonMenuItem7ActionPerformed

    private void jRadioButtonMenuItem8ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem8ActionPerformed
        cost_type = AVG_COST;
        if (this.main_window.getLinkPlots()) main_window.setGroupCostAll(graph_type, cost_type);
        refresh();
    }//GEN-LAST:event_jRadioButtonMenuItem8ActionPerformed

    private void jRadioButtonMenuItem9ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jRadioButtonMenuItem9ActionPerformed
        cost_type = MIN_COST;
        if (this.main_window.getLinkPlots()) main_window.setGroupCostAll(graph_type, cost_type);
        refresh();
    }//GEN-LAST:event_jRadioButtonMenuItem9ActionPerformed

    private boolean filterTimeEntry(SmsEntry t) {
        if (filters == null) return true;
        if (filters[0] != null) {
            try {
                double time = Double.parseDouble(filters[0]);
                if (t.getCost() < time) return false;
            }
            catch (Exception e) {
                return true;
            }
        }
        if (filters[1] != null) {
            try {
                double ratio = Double.parseDouble(filters[1]);
                if (t.getRatio() < ratio) return false;
            }
            catch (Exception e) {
                return true;
            }
        }
        if (filters[2] != null) {
            try {
                int freq = Integer.parseInt(filters[2]);
                if (t.getOcc() < freq) return false;
            }
            catch (Exception e) {
                return true;
            }
        }
        return true;
    }

    private void populateChart() {
        for (int i = 0; i < series.length; i++) series[i].setNotify(false);
        for (int i = 0; i < series.length; i++) series[i].clear();
        
        if (graph_type == RTN_PLOT) {
            double x = 0;
            double y = 0;
            long[] num_class_sms = report.num_class_sms;
            
            for (int k = 0; k < report.num_class_sms.length; k++) {
                
                if (report.num_class_sms[k] == 0) continue;
                x = Math.pow(2, k);
                
                y = 100 * ((double) report.tot_cost_class_sms[k] / (double) report.getTotalCalls());
                series[0].add(x, y);

                y = 100 * ((double)((double) report.tot_cost_class_sms[k] / (double) report.num_class_sms[k])
                                        / (double)report.most_called);
                series[1].add(x, y);
                
                y = 100 * ((double) report.num_class_sms[k] / (double)report.getRoutineCount());
                series[2].add(x, y);
               
                y = 100 * ((double) report.max_cost_class_sms[k] / (double)report.most_called);
                series[3].add(x, y);
               
            }
            for (int i = 0; i < series.length; i++) series[i].setNotify(true);
            return;
        }
        
        if (group_threshold == 1) {
            for (int i = 0; i < this.rtn_info.getTimeEntries().size(); i++) {
                SmsEntry te = this.rtn_info.getTimeEntries().get(i);
                if (filterTimeEntry(te)) {
                    double x = te.getSms();
                    double y = 0;
                    if (this.graph_type == GraphPanel.TIME_PLOT) {
                        y = te.getCost(cost_type); 
                    } else if (this.graph_type == GraphPanel.RATIO_PLOT) 
                        y = te.getRatioCost(cost_type);
                    else if (this.graph_type == GraphPanel.SUM_PLOT) 
                        y = te.getSumCost();
                    else if (this.graph_type == GraphPanel.VAR_PLOT) 
                        y = te.getVar();
                    
                    if (this.graph_type == GraphPanel.MMM_PLOT) {
                    
                        y = te.getMinCost();
                        series[0].add(x, y);
                        y = te.getAvgCost();
                        series[5].add(x, y);
                        y = te.getMaxCost();
                        series[11].add(x, y);
                    
                    } else if (this.graph_type == GraphPanel.FREQ_PLOT) {
                        y = te.getOcc();
                        series[0].add(x, y);
                    } else {
                        double index = Math.round(Math.log10(te.getOcc()) / Math.log10(2));
                        if (index > 11) index = 11;
                        if (index < 0) index = 0;
                        series[(int)index].add(x, y);
                    }
                }
            }
        }
        else {
            double slot_start = 0;
            double sum_x = 0;
            double sum_y = 0;
            int sum_occurrences = 0;
            int n = 0;
            for (int i = 0; i < this.rtn_info.getTimeEntries().size(); i++) {
                SmsEntry te = this.rtn_info.getTimeEntries().get(i);
                if (filterTimeEntry(te)) {
                    double x = te.getSms();
                    double y;
                    if (graph_type == GraphPanel.TIME_PLOT) y = te.getCost();
                    else if(graph_type == GraphPanel.MMM_PLOT) y = te.getCost();
                    else if (graph_type == GraphPanel.RATIO_PLOT) y = te.getRatio();
                    else y = te.getOcc();
                    double current_slot = x - (x % group_threshold);
                    if (current_slot == slot_start) {
                        sum_x += x;
                        sum_y += y;
                        sum_occurrences += te.getOcc();
                        n++;
                    }
                    else {
                        if (n > 0) {
                            double mean_x = (n == 0) ? sum_x : sum_x / n;
                            if (graph_type == GraphPanel.FREQ_PLOT) {
                                series[0].add(mean_x, sum_y);
                                if (sum_y > max_y) max_y = sum_y;
                            }
                            else {
                                double mean_y = (n == 0) ? sum_y : sum_y / n;
                                double index = Math.round(Math.log10(sum_occurrences) / Math.log10(2));
                                if (index > 11) index = 11;
                                if (index < 0) index = 0;
                                series[(int)index].add(mean_x, mean_y);
                            }
                            sum_x = 0;
                            sum_y = 0;
                            sum_occurrences = 0;
                            n = 0;
                            sum_x += x;
                            sum_y += y;
                            sum_occurrences += te.getOcc();
                            n++;
                        }
                        slot_start = current_slot;
                    }
                }
            }
            if (n > 0) {
                double mean_x = (n == 0) ? sum_x : sum_x / n;
                if (graph_type == GraphPanel.FREQ_PLOT) {
                    series[0].add(mean_x, sum_y);
                    if (sum_y > max_y) max_y = sum_y;
                }
                else {
                    double mean_y = (n == 0) ? sum_y : sum_y / n;
                    double index = Math.round(Math.log10(sum_occurrences) / Math.log10(2));
                    if (index > 11) index = 11;
                    if (index < 0) index = 0;
                    series[(int)index].add(mean_x, mean_y);
                }
            }
        }
        for (int i = 0; i < series.length; i++) series[i].setNotify(true);
    }

    public void setData(RoutineInfo r) {
        chart.setNotify(false);
        
        if (r == null && (graph_type != RTN_PLOT || report == null)) {
            clearData();
            return;
        }
        this.rtn_info = r;
        populateChart();
        if (autoscaled) autoscale();
        else maximize();
        chart.setNotify(true);
    }

    public void clearData() {
        for (int i = 0; i < series.length; i++) series[i].clear();
        if (graph_type == RTN_PLOT) {
            this.min_x = -1; 
            this.min_y = -10;
        } else {
            this.min_x = this.min_y = 0;
        }
        this.max_x = this.max_y = 10;
        updateXAxis();
        updateYAxis();
    }

    private void refresh() {
        if (rtn_info == null) return;
        //if (magnify) updateGraph();
        populateChart();
        if (autoscaled) autoscale();
        else if (maximized) maximize();
        else {
            updateXAxis();
            updateYAxis();
        }
    }

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JButton jButton1;
    private javax.swing.JButton jButton2;
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
    private javax.swing.JPanel jPanel2;
    private javax.swing.JPanel jPanel3;
    private javax.swing.JPanel jPanel4;
    private javax.swing.JPopupMenu jPopupMenu1;
    private javax.swing.JPopupMenu jPopupMenu2;
    private javax.swing.JPopupMenu jPopupMenu3;
    private javax.swing.JRadioButtonMenuItem jRadioButtonMenuItem1;
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
    private javax.swing.JToggleButton jToggleButton6;
    private javax.swing.JToggleButton jToggleButton7;
    // End of variables declaration//GEN-END:variables
    private javax.swing.ButtonGroup groupMenuButtonGroup;
    private javax.swing.ButtonGroup groupMenuButtonGroup2;
    private javax.swing.ButtonGroup toggleButtonGroup;
}
