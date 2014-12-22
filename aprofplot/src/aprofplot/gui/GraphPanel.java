package aprofplot.gui;

import aprofplot.*;
import static aprofplot.gui.GraphPanel.Type.*;
import static aprofplot.gui.GraphPanel.Status.*;
import aprofplot.jfreechart.SamplingXYLineAndShapeRenderer;
import java.awt.*;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;
import java.io.File;
import java.util.*;
import javax.imageio.ImageIO;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JRadioButtonMenuItem;
import javax.swing.MenuElement;
import javax.swing.filechooser.FileNameExtensionFilter;
import org.jfree.chart.*;
import org.jfree.chart.axis.*;
import org.jfree.chart.plot.*;
import org.jfree.chart.renderer.xy.*;
import org.jfree.chart.title.*;
import org.jfree.data.xy.*;
import org.jfree.ui.RectangleInsets;

public class GraphPanel extends javax.swing.JPanel {

    // type of graph
    public enum Type {

        COST_PLOT, RATIO_PLOT, FREQ_PLOT, MMM_PLOT, TOTALCOST_PLOT,
        VAR_PLOT, AMORTIZED_PLOT
    }

    // status of graph
    public enum Status {

        MAXIMIZED, ZOOMED
    }

    // Performance monitor
    private PerformanceMonitor perf = null;
    private final MainWindow main_window;

    // current state of graph
    private final Type graph_type;
    private Input.CostKind cost_type = Input.CostKind.WORST;
    private boolean x_log_scale = false;
    private boolean y_log_scale = false;
    private Status status = Status.MAXIMIZED;
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
    private final XYPlot plot;
    private final JFreeChart chart;
    private NumberAxis rangeAxis, domainAxis;
    private final XYItemRenderer renderer;
    private final ChartPanel chartPanel;
    private final XYSeriesCollection data;
    private final XYSeries[] series;

    // Some colors: from blue to red
    private final Color[] colors = new Color[]{
        new Color(0, 85, 255),
        new Color(0, 170, 255),
        new Color(0, 213, 255),
        new Color(0, 255, 234),
        new Color(0, 255, 161),
        new Color(190, 235, 62),
        new Color(232, 253, 2),
        new Color(255, 242, 0),
        new Color(255, 255, 0),
        new Color(255, 191, 0),
        new Color(255, 145, 0),
        new Color(255, 64, 0)
    };

    public GraphPanel(Type graph_type, MainWindow mw) {

        // set a minimum size of the panel
        Dimension d = new Dimension(370, 300);
        this.setPreferredSize(d);
        
        this.graph_type = graph_type;
        this.main_window = mw;
        this.filters = main_window.getRmsTableFilter();

        // we will use 12 series of points, one for each color we have choosen 
        data = new XYSeriesCollection();
        series = new XYSeries[12];
        for (int i = 0; i < series.length; i++) {
            series[i] = new XYSeries(i + "");
            data.addSeries(series[i]);
        }

        // get legend
        LegendItemCollection legend = buildLegend();

        // build chart
        chart = ChartFactory.createScatterPlot(null, // title
            null, // x axis label
            null, // y axis label
            data, // data
            PlotOrientation.VERTICAL,
            (legend != null), // legend ?
            false, // tooltips?
            false // urls?
        );

        // antialiasing is slow :(
        chart.setAntiAlias(false);

        // build plot
        plot = (XYPlot) chart.getPlot();
        plot.setBackgroundPaint(Color.WHITE);
        plot.setAxisOffset(new RectangleInsets(0.0, 0.0, 0.0, 0.0));
        plot.setDomainGridlinePaint(Color.LIGHT_GRAY);
        plot.setRangeGridlinePaint(Color.LIGHT_GRAY);
        if (legend != null) {
            plot.setFixedLegendItems(buildLegend());
        }

        // use a custom but more efficient renderer
        renderer = new SamplingXYLineAndShapeRenderer(false, true);
        plot.setRenderer(renderer);

        // Associate each series with a color, shape, line, etc
        for (int i = 0; i < series.length; i++) {

            renderer.setSeriesOutlinePaint(i, colors[colors.length - 1 - i]);
            renderer.setSeriesShape(i, new Rectangle2D.Double(-2.5, -2.5, 3.0, 3.0));
            renderer.setSeriesPaint(i, colors[colors.length - 1 - i]);
            ((XYLineAndShapeRenderer) renderer).setUseOutlinePaint(true);

        }
        ((XYLineAndShapeRenderer) renderer).setDrawOutlines(true);

        chartPanel = new ChartPanel(chart, true);
        chartPanel.getChartRenderingInfo().setEntityCollection(null);
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
        this.add(chartPanel);

        updateGraphTitle();
        updateXAxis(false);
        updateYAxis(false);
    }

    private LegendItemCollection buildLegend() {

        if (graph_type == MMM_PLOT) {

            LegendItemCollection legenditemcollection = new LegendItemCollection();
            LegendItem legenditem = new LegendItem("Worst", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, colors[11]);
            LegendItem legenditem1 = new LegendItem("Average", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, colors[5]);
            LegendItem legenditem2 = new LegendItem("Best", "-", null, null, Plot.DEFAULT_LEGEND_ITEM_BOX, colors[0]);
            legenditemcollection.add(legenditem);
            legenditemcollection.add(legenditem1);
            legenditemcollection.add(legenditem2);
            return legenditemcollection;
        }

        return null;
    }

    /**
     * This method is called from within the constructor to initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is always
     * regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        GroupMenu = new javax.swing.JPopupMenu();
        groupMenuButtonGroup = new javax.swing.ButtonGroup();
        GroupThresholdButton1 = new javax.swing.JRadioButtonMenuItem();
        GroupThresholdButton2 = new javax.swing.JRadioButtonMenuItem();
        GroupThresholdButton3 = new javax.swing.JRadioButtonMenuItem();
        GroupThresholdButton4 = new javax.swing.JRadioButtonMenuItem();
        GroupThresholdButton5 = new javax.swing.JRadioButtonMenuItem();
        GroupThresholdButton6 = new javax.swing.JRadioButtonMenuItem();
        RatioMenu = new javax.swing.JPopupMenu();
        RatioFunction1 = new javax.swing.JMenuItem();
        RatioFunction2 = new javax.swing.JMenuItem();
        RatioFunction3 = new javax.swing.JMenuItem();
        RatioFunction4 = new javax.swing.JMenuItem();
        RatioFunction5 = new javax.swing.JMenuItem();
        RatioFunction6 = new javax.swing.JMenuItem();
        RatioFunction7 = new javax.swing.JMenuItem();
        RatioFunction8 = new javax.swing.JMenuItem();
        RatioFunction9 = new javax.swing.JMenuItem();
        RatioFunction10 = new javax.swing.JMenuItem();
        RatioFunction11 = new javax.swing.JMenuItem();
        RatioFunction12 = new javax.swing.JMenuItem();
        RationFunction13 = new javax.swing.JMenuItem();
        RatioFunction14 = new javax.swing.JMenuItem();
        CostMenu = new javax.swing.JPopupMenu();
        WorstCostButton = new javax.swing.JRadioButtonMenuItem();
        AverageCostButton = new javax.swing.JRadioButtonMenuItem();
        BestCostButton = new javax.swing.JRadioButtonMenuItem();
        SmoothMenu = new javax.swing.JPopupMenu();
        SmoothThresholdButton1 = new javax.swing.JRadioButtonMenuItem();
        SmoothThresholdButton2 = new javax.swing.JRadioButtonMenuItem();
        SmoothThresholdButton3 = new javax.swing.JRadioButtonMenuItem();
        SmoothThresholdButton4 = new javax.swing.JRadioButtonMenuItem();
        SmoothThresholdButton5 = new javax.swing.JRadioButtonMenuItem();
        SmoothThresholdButton6 = new javax.swing.JRadioButtonMenuItem();
        jToolBar1 = new javax.swing.JToolBar();
        XLogToggle = new javax.swing.JToggleButton();
        YLogToggle = new javax.swing.JToggleButton();
        pointAggregationMenu = new javax.swing.JToggleButton();
        ratioMenu = new javax.swing.JToggleButton();
        exportButton = new javax.swing.JButton();
        costTypeMenu = new javax.swing.JToggleButton();
        smoothMenu = new javax.swing.JToggleButton();
        alphaChooser = new javax.swing.JButton();
        zoomOutButton = new javax.swing.JButton();
        filler1 = new javax.swing.Box.Filler(new java.awt.Dimension(0, 0), new java.awt.Dimension(0, 0), new java.awt.Dimension(0, 0));
        colorLegend1 = new aprofplot.gui.ColorLegend();

        GroupMenu.addPopupMenuListener(new javax.swing.event.PopupMenuListener() {
            public void popupMenuCanceled(javax.swing.event.PopupMenuEvent evt) {
            }
            public void popupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {
                GroupMenuPopupMenuWillBecomeInvisible(evt);
            }
            public void popupMenuWillBecomeVisible(javax.swing.event.PopupMenuEvent evt) {
            }
        });

        GroupThresholdButton1.setSelected(true);
        GroupThresholdButton1.setText("1");
        GroupThresholdButton1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                GroupThresholdButton1ActionPerformed(evt);
            }
        });
        GroupMenu.add(GroupThresholdButton1);

        GroupThresholdButton2.setText("2");
        GroupThresholdButton2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                GroupThresholdButton2ActionPerformed(evt);
            }
        });
        GroupMenu.add(GroupThresholdButton2);

        GroupThresholdButton3.setText("3");
        GroupThresholdButton3.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                GroupThresholdButton3ActionPerformed(evt);
            }
        });
        GroupMenu.add(GroupThresholdButton3);

        GroupThresholdButton4.setText("4");
        GroupThresholdButton4.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                GroupThresholdButton4ActionPerformed(evt);
            }
        });
        GroupMenu.add(GroupThresholdButton4);

        GroupThresholdButton5.setText("5");
        GroupThresholdButton5.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                GroupThresholdButton5ActionPerformed(evt);
            }
        });
        GroupMenu.add(GroupThresholdButton5);

        GroupThresholdButton6.setText("6");
        GroupThresholdButton6.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                GroupThresholdButton6ActionPerformed(evt);
            }
        });
        GroupMenu.add(GroupThresholdButton6);
        groupMenuButtonGroup.add(GroupThresholdButton1);
        groupMenuButtonGroup.add(GroupThresholdButton2);
        groupMenuButtonGroup.add(GroupThresholdButton3);
        groupMenuButtonGroup.add(GroupThresholdButton4);
        groupMenuButtonGroup.add(GroupThresholdButton5);
        groupMenuButtonGroup.add(GroupThresholdButton6);

        RatioMenu.addPopupMenuListener(new javax.swing.event.PopupMenuListener() {
            public void popupMenuCanceled(javax.swing.event.PopupMenuEvent evt) {
            }
            public void popupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {
                RatioMenuPopupMenuWillBecomeInvisible(evt);
            }
            public void popupMenuWillBecomeVisible(javax.swing.event.PopupMenuEvent evt) {
            }
        });

        RatioFunction1.setText("T(n)/n ");
        RatioFunction1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction1ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction1);

        RatioFunction2.setText("T(n)/(n log log n) ");
        RatioFunction2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction2ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction2);

        RatioFunction3.setText("T(n)/(n log n) ");
        RatioFunction3.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction3ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction3);

        RatioFunction4.setText("T(n)/n^1.5 ");
        RatioFunction4.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction4ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction4);

        RatioFunction5.setText("T(n)/n^2 ");
        RatioFunction5.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction5ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction5);

        RatioFunction6.setText("T(n)/n^2.5 ");
        RatioFunction6.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction6ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction6);

        RatioFunction7.setText("T(n)/n^3 ");
        RatioFunction7.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction7ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction7);

        RatioFunction8.setText("*= log log n");
        RatioFunction8.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction8ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction8);

        RatioFunction9.setText("/= log log n");
        RatioFunction9.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction9ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction9);

        RatioFunction10.setText("*= log n");
        RatioFunction10.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction10ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction10);

        RatioFunction11.setText("/= log n");
        RatioFunction11.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction11ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction11);

        RatioFunction12.setText("*= (log log n)^eps ...");
        RatioFunction12.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction12ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction12);

        RationFunction13.setText("*= (log n)^eps ...");
        RationFunction13.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RationFunction13ActionPerformed(evt);
            }
        });
        RatioMenu.add(RationFunction13);

        RatioFunction14.setText("*= n^eps ...");
        RatioFunction14.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                RatioFunction14ActionPerformed(evt);
            }
        });
        RatioMenu.add(RatioFunction14);

        CostMenu.addPopupMenuListener(new javax.swing.event.PopupMenuListener() {
            public void popupMenuCanceled(javax.swing.event.PopupMenuEvent evt) {
            }
            public void popupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {
                CostMenuPopupMenuWillBecomeInvisible(evt);
            }
            public void popupMenuWillBecomeVisible(javax.swing.event.PopupMenuEvent evt) {
            }
        });
        groupMenuButtonGroup2 = new javax.swing.ButtonGroup();

        WorstCostButton.setSelected(true);
        WorstCostButton.setText("Worst");
        WorstCostButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                WorstCostButtonActionPerformed(evt);
            }
        });
        CostMenu.add(WorstCostButton);

        AverageCostButton.setSelected(true);
        AverageCostButton.setText("Average");
        AverageCostButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                AverageCostButtonActionPerformed(evt);
            }
        });
        CostMenu.add(AverageCostButton);

        BestCostButton.setSelected(true);
        BestCostButton.setText("Best");
        BestCostButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                BestCostButtonActionPerformed(evt);
            }
        });
        CostMenu.add(BestCostButton);

        groupMenuButtonGroup2.add(WorstCostButton);
        groupMenuButtonGroup2.add(AverageCostButton);
        groupMenuButtonGroup2.add(BestCostButton);

        SmoothMenu.addPopupMenuListener(new javax.swing.event.PopupMenuListener() {
            public void popupMenuCanceled(javax.swing.event.PopupMenuEvent evt) {
            }
            public void popupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {
                SmoothMenuPopupMenuWillBecomeInvisible(evt);
            }
            public void popupMenuWillBecomeVisible(javax.swing.event.PopupMenuEvent evt) {
            }
        });

        SmoothThresholdButton1.setSelected(true);
        SmoothThresholdButton1.setText("1");
        SmoothThresholdButton1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                SmoothThresholdButton1ActionPerformed(evt);
            }
        });
        SmoothMenu.add(SmoothThresholdButton1);

        SmoothThresholdButton2.setSelected(true);
        SmoothThresholdButton2.setText("2");
        SmoothThresholdButton2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                SmoothThresholdButton2ActionPerformed(evt);
            }
        });
        SmoothMenu.add(SmoothThresholdButton2);

        SmoothThresholdButton3.setSelected(true);
        SmoothThresholdButton3.setText("3");
        SmoothThresholdButton3.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                SmoothThresholdButton3ActionPerformed(evt);
            }
        });
        SmoothMenu.add(SmoothThresholdButton3);

        SmoothThresholdButton4.setSelected(true);
        SmoothThresholdButton4.setText("4");
        SmoothThresholdButton4.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                SmoothThresholdButton4ActionPerformed(evt);
            }
        });
        SmoothMenu.add(SmoothThresholdButton4);

        SmoothThresholdButton5.setSelected(true);
        SmoothThresholdButton5.setText("5");
        SmoothThresholdButton5.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                SmoothThresholdButton5ActionPerformed(evt);
            }
        });
        SmoothMenu.add(SmoothThresholdButton5);

        SmoothThresholdButton6.setSelected(true);
        SmoothThresholdButton6.setText("6");
        SmoothThresholdButton6.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                SmoothThresholdButton6ActionPerformed(evt);
            }
        });
        SmoothMenu.add(SmoothThresholdButton6);

        groupMenuButtonGroup3 = new javax.swing.ButtonGroup();
        groupMenuButtonGroup3.add(SmoothThresholdButton1);
        groupMenuButtonGroup3.add(SmoothThresholdButton2);
        groupMenuButtonGroup3.add(SmoothThresholdButton3);
        groupMenuButtonGroup3.add(SmoothThresholdButton4);
        groupMenuButtonGroup3.add(SmoothThresholdButton5);
        groupMenuButtonGroup3.add(SmoothThresholdButton6);

        setBorder(javax.swing.BorderFactory.createEtchedBorder());
        setLayout(new javax.swing.BoxLayout(this, javax.swing.BoxLayout.Y_AXIS));

        jToolBar1.setBorder(null);
        jToolBar1.setFloatable(false);
        jToolBar1.setRollover(true);
        jToolBar1.setMargin(new java.awt.Insets(0, 3, 3, 3));

        XLogToggle.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Xlog-icon.png"))); // NOI18N
        XLogToggle.setToolTipText("toggle x axis logarithmic scale");
        XLogToggle.setFocusTraversalPolicyProvider(true);
        XLogToggle.setFocusable(false);
        XLogToggle.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        XLogToggle.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        XLogToggle.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                XLogToggleActionPerformed(evt);
            }
        });
        jToolBar1.add(XLogToggle);

        YLogToggle.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Ylog-icon.png"))); // NOI18N
        YLogToggle.setToolTipText("toggle y axis logarithmic scale");
        YLogToggle.setFocusable(false);
        YLogToggle.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        YLogToggle.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        YLogToggle.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                YLogToggleActionPerformed(evt);
            }
        });
        jToolBar1.add(YLogToggle);

        pointAggregationMenu.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/Incorporate-icon.png"))); // NOI18N
        pointAggregationMenu.setToolTipText("group graph points");
        pointAggregationMenu.setFocusable(false);
        pointAggregationMenu.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        pointAggregationMenu.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        pointAggregationMenu.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                pointAggregationMenuActionPerformed(evt);
            }
        });
        if (graph_type == AMORTIZED_PLOT)
        pointAggregationMenu.setVisible(false);
        jToolBar1.add(pointAggregationMenu);

        ratioMenu.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/RatioType-icon.png"))); // NOI18N
        ratioMenu.setToolTipText("select ratio type");
        ratioMenu.setFocusable(false);
        ratioMenu.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ratioMenu.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        ratioMenu.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                ratioMenuActionPerformed(evt);
            }
        });
        if (graph_type != RATIO_PLOT) ratioMenu.setVisible(false);
        jToolBar1.add(ratioMenu);

        exportButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/SavePlot-icon.png"))); // NOI18N
        exportButton.setToolTipText("export as PNG image");
        exportButton.setBorder(null);
        exportButton.setFocusable(false);
        exportButton.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        exportButton.setMaximumSize(new java.awt.Dimension(28, 28));
        exportButton.setMinimumSize(new java.awt.Dimension(28, 28));
        exportButton.setPreferredSize(new java.awt.Dimension(28, 28));
        exportButton.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        exportButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                exportButtonActionPerformed(evt);
            }
        });
        jToolBar1.add(exportButton);

        costTypeMenu.setFont(new java.awt.Font("Ubuntu", 1, 13)); // NOI18N
        costTypeMenu.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/type.png"))); // NOI18N
        costTypeMenu.setToolTipText("Select cost type");
        costTypeMenu.setFocusable(false);
        costTypeMenu.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        costTypeMenu.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        costTypeMenu.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                costTypeMenuActionPerformed(evt);
            }
        });
        if (graph_type != COST_PLOT && graph_type != RATIO_PLOT)
        costTypeMenu.setVisible(false);
        jToolBar1.add(costTypeMenu);

        smoothMenu.setFont(new java.awt.Font("Ubuntu", 1, 13)); // NOI18N
        smoothMenu.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/tool_curve.png"))); // NOI18N
        smoothMenu.setToolTipText("Smooth point window");
        smoothMenu.setFocusable(false);
        smoothMenu.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        smoothMenu.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        smoothMenu.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                smoothMenuActionPerformed(evt);
            }
        });
        if (graph_type == AMORTIZED_PLOT)
        smoothMenu.setVisible(false);
        jToolBar1.add(smoothMenu);

        alphaChooser.setFont(new java.awt.Font("Arial", 1, 20)); // NOI18N
        alphaChooser.setText("α");
        alphaChooser.setToolTipText("Amortized costant");
        alphaChooser.setBorder(null);
        alphaChooser.setEnabled(false);
        alphaChooser.setFocusable(false);
        alphaChooser.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        alphaChooser.setMaximumSize(new java.awt.Dimension(28, 28));
        alphaChooser.setMinimumSize(new java.awt.Dimension(28, 28));
        alphaChooser.setPreferredSize(new java.awt.Dimension(28, 28));
        alphaChooser.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        alphaChooser.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                alphaChooserActionPerformed(evt);
            }
        });
        alphaChooser.setVisible(false);
        jToolBar1.add(alphaChooser);

        zoomOutButton.setIcon(new javax.swing.ImageIcon(getClass().getResource("/aprofplot/gui/resources/zoom_out_16.png"))); // NOI18N
        zoomOutButton.setToolTipText("Zoom out");
        zoomOutButton.setBorder(null);
        zoomOutButton.setEnabled(false);
        zoomOutButton.setVisible(false);
        zoomOutButton.setFocusable(false);
        zoomOutButton.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        zoomOutButton.setMaximumSize(new java.awt.Dimension(28, 28));
        zoomOutButton.setMinimumSize(new java.awt.Dimension(28, 28));
        zoomOutButton.setPreferredSize(new java.awt.Dimension(28, 28));
        zoomOutButton.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        zoomOutButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                zoomOutButtonActionPerformed(evt);
            }
        });
        jToolBar1.add(zoomOutButton);
        jToolBar1.add(filler1);

        colorLegend1.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        if (graph_type == FREQ_PLOT
            || graph_type == MMM_PLOT)
        colorLegend1.setVisible(false);
        jToolBar1.add(colorLegend1);

        add(jToolBar1);
    }// </editor-fold>//GEN-END:initComponents

    private void chartMouseReleased() {

        double cmin_x = domainAxis.getLowerBound();
        double cmax_x = domainAxis.getUpperBound();
        double cmin_y = rangeAxis.getLowerBound();
        double cmax_y = rangeAxis.getUpperBound();

        // Have we zoomed?
        boolean changed_bounds = (min_x != cmin_x) || (max_x != cmax_x)
            || (cmin_y != min_y) || (cmax_y != max_y);
        if (this.dragging && changed_bounds) {

            zoomResetVisible(true);
            status = ZOOMED;
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

    public Type getGraphType() {
        return graph_type;
    }

    public int getGraphPriority() {

        switch (graph_type) {

            case COST_PLOT:
                return 0;
            case AMORTIZED_PLOT:
                return 1;
            case FREQ_PLOT:
                return 2;
            case MMM_PLOT:
                return 3;
            case TOTALCOST_PLOT:
                return 4;
            case VAR_PLOT:
                return 5;
            case RATIO_PLOT:
                return 6;

            default:
                throw new RuntimeException("Invalid chart");
        }
    }

    private String getXLabel() {
        switch (graph_type) {
            default:
                if (main_window.isInputMetricRms()) {
                    return "read memory size";
                } else {
                    return "dynamic read memory size";
                }
        }
    }

    private String getYLabel() {
        switch (graph_type) {

            case RATIO_PLOT:
                return "ratio";

            case FREQ_PLOT:
                return "frequency";

            case AMORTIZED_PLOT:
                return "amortized cost";

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

        if (!reset && status == ZOOMED) {
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

        if (!reset && status == ZOOMED) {
            rangeAxis.setRange(min_y, max_y);
            if (graph_type == FREQ_PLOT) {
                this.rangeAxis.setStandardTickUnits(NumberAxis.createIntegerTickUnits());
            }
        } else {
            rangeAxis.setAutoRangeIncludesZero(true);
        }

        plot.setRangeAxis(rangeAxis);
    }

    public void updateGraphTitle() {

        String s;
        switch (this.graph_type) {

            case COST_PLOT:
                s = "Cost plot";
                break;
            case FREQ_PLOT:
                if (main_window.isInputMetricRms()) {
                    s = "RMS frequency plot";
                    break;
                } else {
                    s = "DRMS frequency plot";
                    break;
                }
            case MMM_PLOT:
                s = "Best/Avg/Worst cost plot";
                break;
            case TOTALCOST_PLOT:
                s = "Total cost plot";
                break;
            case VAR_PLOT:
                s = "Cost variance plot";
                break;

            case AMORTIZED_PLOT:
                s = "Amortized cost plot";
                if (rtn_info != null) {
                    double alpha = ((int) (rtn_info.getAmortizedConstant() * 1000));
                    alpha /= 1000;
                    s += " (α = " + alpha + ")";
                }
                break;

            case RATIO_PLOT:
                s = "Curve bounding plot - T(n) / ";
                double[] rc = Input.getRatioConfig();
                int k = 0;
                for (int i = 0; i < rc.length; i++) {
                    if (rc[i] != 0) {
                        k++;
                    }
                }
                if (k > 1) {
                    s += "(";
                }
                if (rc[0] != 0) {
                    if (rc[0] == 1) {
                        s += "n";
                    } else {
                        s += "n^" + rc[0];
                    }
                }
                if (rc[1] != 0) {
                    if (rc[0] != 0) {
                        s += " ";
                    }
                    if (rc[1] == 1) {
                        s += "log(n)";
                    } else {
                        s += "log(n)^" + rc[1];
                    }
                }
                if (rc[2] != 0) {
                    if (rc[0] != 0 || rc[1] != 0) {
                        s += " * ";
                    }
                    if (rc[2] == 1) {
                        s += "log(log(n))";
                    } else {
                        s += "log(log(n))^" + rc[2];
                    }
                }
                if (k > 1) {
                    s += ")";
                }
                break;
            default:
                s = "";
        }

        chart.setTitle(new TextTitle(s, new Font(Font.SERIF, Font.BOLD, 14)));
    }

    public void zoomResetVisible(boolean visible) {
        if (visible) {
            zoomOutButton.setEnabled(true);
            zoomOutButton.setVisible(true);
        } else {
            zoomOutButton.setEnabled(false);
            zoomOutButton.setVisible(false);
        }
    }

    public void refreshFilter() {
        filters = main_window.getRmsTableFilter();
        refresh(true);
    }

    public void setXLogScale(boolean logscale) {
        x_log_scale = logscale;
        XLogToggle.setSelected(logscale);
        updateXAxis(false);
        updateMinimalTicks();
    }

    public void setYLogScale(boolean logscale) {
        y_log_scale = logscale;
        YLogToggle.setSelected(logscale);
        updateYAxis(false);
        updateMinimalTicks();
    }

    public void setGroupCost(Input.CostKind cost_type) {

        if (cost_type == this.cost_type) {
            return;
        }

        switch (cost_type) {
            case WORST:
                WorstCostButton.setSelected(true);
                break;
            case AVG:
                AverageCostButton.setSelected(true);
                break;
            case BEST:
                BestCostButton.setSelected(true);
                break;
        }

        this.cost_type = cost_type;
        refresh(false);
    }

    public void setSmoothThreshold(int t) {

        if (rtn_info == null || graph_type == AMORTIZED_PLOT) {
            return;
        }

        // reset grouping
        group_threshold = 1;
        GroupThresholdButton1.setSelected(true);

        switch (t) {
            case 1:
                SmoothThresholdButton1.setSelected(true);
                break;
            case 2:
                SmoothThresholdButton2.setSelected(true);
                break;
            case 3:
                SmoothThresholdButton3.setSelected(true);
                break;
            case 4:
                SmoothThresholdButton4.setSelected(true);
                break;
            case 5:
                SmoothThresholdButton5.setSelected(true);
                break;
            case 6:
                SmoothThresholdButton6.setSelected(true);
                break;
        }

        smooth_threshold = t;
        refresh(false);
    }

    public void setGroupThreshold(int t) {

        if (rtn_info == null || graph_type == AMORTIZED_PLOT) {
            return;
        }

        // Reset smoothing
        smooth_threshold = 1;
        SmoothThresholdButton1.setSelected(true);

        switch (t) {
            case 1:
                GroupThresholdButton1.setSelected(true);
                break;
            case 2:
                GroupThresholdButton2.setSelected(true);
                break;
            case 3:
                GroupThresholdButton3.setSelected(true);
                break;
            case 4:
                GroupThresholdButton4.setSelected(true);
                break;
            case 5:
                GroupThresholdButton5.setSelected(true);
                break;
            case 6:
                GroupThresholdButton6.setSelected(true);
                break;
        }

        group_threshold = (int) Math.pow(group_threshold_base, t - 1);
        refresh(false);
    }

    public void maximize() {

        status = MAXIMIZED;

        updateXAxis(true);
        updateYAxis(true);

        chartPanel.restoreAutoBounds();
        max_x = domainAxis.getUpperBound();
        max_y = rangeAxis.getUpperBound();
        min_x = domainAxis.getLowerBound();
        min_y = rangeAxis.getLowerBound();

        updateMinimalTicks();
    }

    private void updateMinimalTicks() {

        if (!x_log_scale && max_x < 2)
            domainAxis.setTickUnit(new NumberTickUnit(max_x / 10));
        else if (domainAxis.getTickUnit().getSize() != 1.0)
            domainAxis.setTickUnit(new NumberTickUnit(1));

        if (!y_log_scale && max_y < 2) 
            rangeAxis.setTickUnit(new NumberTickUnit(max_y / 10));
        else if (rangeAxis.getTickUnit().getSize() != 1.0)
            rangeAxis.setTickUnit(new NumberTickUnit(1));
    }

	private void XLogToggleActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_XLogToggleActionPerformed
        // X logscale button
        boolean logscale = XLogToggle.isSelected();
        setXLogScale(logscale);
        if (this.main_window.arePlotsLinked()) {
            main_window.setXLogScaleAll(graph_type, logscale);
        }
	}//GEN-LAST:event_XLogToggleActionPerformed

	private void YLogToggleActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_YLogToggleActionPerformed
        // Y logscale button
        boolean logscale = YLogToggle.isSelected();
        setYLogScale(logscale);
        if (this.main_window.arePlotsLinked()) {
            main_window.setYLogScaleAll(graph_type, logscale);
        }
	}//GEN-LAST:event_YLogToggleActionPerformed

	private void pointAggregationMenuActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_pointAggregationMenuActionPerformed
        // Grouping button
        if (pointAggregationMenu.isSelected()) {
            GroupMenu.show(pointAggregationMenu, 0, pointAggregationMenu.getHeight());
        } else {
            GroupMenu.setVisible(false);
        }
	}//GEN-LAST:event_pointAggregationMenuActionPerformed

	private void GroupMenuPopupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {//GEN-FIRST:event_GroupMenuPopupMenuWillBecomeInvisible
        pointAggregationMenu.setSelected(false);
	}//GEN-LAST:event_GroupMenuPopupMenuWillBecomeInvisible

	private void GroupThresholdButton1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_GroupThresholdButton1ActionPerformed
        // Grouping menu > first entry
        setGroupThreshold(1);
        if (this.main_window.arePlotsLinked()) {
            main_window.setGroupThresholdAll(graph_type, 1);
        }
	}//GEN-LAST:event_GroupThresholdButton1ActionPerformed

	private void GroupThresholdButton2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_GroupThresholdButton2ActionPerformed
        // Grouping menu > second entry
        setGroupThreshold(2);
        if (this.main_window.arePlotsLinked()) {
            main_window.setGroupThresholdAll(graph_type, 2);
        }
	}//GEN-LAST:event_GroupThresholdButton2ActionPerformed

	private void GroupThresholdButton3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_GroupThresholdButton3ActionPerformed
        // Grouping menu > third entry
        setGroupThreshold(3);
        if (this.main_window.arePlotsLinked()) {
            main_window.setGroupThresholdAll(graph_type, 3);
        }
	}//GEN-LAST:event_GroupThresholdButton3ActionPerformed

	private void GroupThresholdButton4ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_GroupThresholdButton4ActionPerformed
        // Grouping menu > fourth entry
        setGroupThreshold(4);
        if (this.main_window.arePlotsLinked()) {
            main_window.setGroupThresholdAll(graph_type, 4);
        }
	}//GEN-LAST:event_GroupThresholdButton4ActionPerformed

	private void GroupThresholdButton5ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_GroupThresholdButton5ActionPerformed
        // Grouping menu > fifth entry
        setGroupThreshold(5);
        if (this.main_window.arePlotsLinked()) {
            main_window.setGroupThresholdAll(graph_type, 5);
        }
	}//GEN-LAST:event_GroupThresholdButton5ActionPerformed

	private void GroupThresholdButton6ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_GroupThresholdButton6ActionPerformed
        // Grouping menu > sixth entry
        setGroupThreshold(6);
        if (this.main_window.arePlotsLinked()) {
            main_window.setGroupThresholdAll(graph_type, 6);
        }
	}//GEN-LAST:event_GroupThresholdButton6ActionPerformed

	private void exportButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_exportButtonActionPerformed

        // Save graph image button
        if (this.rtn_info == null) {
            return;
        }

        String filename = this.rtn_info.getName();
        switch (graph_type) {
            case COST_PLOT:
                filename += "_cost_plot.png";
                break;
            case MMM_PLOT:
                filename += "_mmm_plot.png";
                break;
            case TOTALCOST_PLOT:
                filename += "_total_plot.png";
                break;
            case VAR_PLOT:
                filename += "_var_plot.png";
                break;
            case RATIO_PLOT:
                filename += "_ratio_plot.png";
                break;
            case AMORTIZED_PLOT:
                filename += "_amm_plot.png";
                break;
            case FREQ_PLOT:
                filename += "_freq_plot.png";
                break;
            default:
                throw new RuntimeException("Invalid chart");
        }

        File f = new File(filename);
        JFileChooser chooser = new JFileChooser();
        String lastReportPath = Main.getLastReportPath();
        if (!lastReportPath.equals("")) {
            chooser.setCurrentDirectory(new java.io.File(lastReportPath));
        }
        FileNameExtensionFilter filter = new FileNameExtensionFilter("PNG images", "png");
        chooser.setFileFilter(filter);
        chooser.setAcceptAllFileFilterUsed(false);
        chooser.setSelectedFile(f);
        int choice = chooser.showSaveDialog(this.getParent());
        if (choice == JFileChooser.APPROVE_OPTION) {
            f = chooser.getSelectedFile();
            BufferedImage img = new BufferedImage(this.chartPanel.getWidth(),
                                    this.chartPanel.getHeight(),
                                    BufferedImage.TYPE_INT_ARGB);
            Graphics2D g = img.createGraphics();
            this.chartPanel.paint(g);
            g.dispose();
            try {
                ImageIO.write(img, "png", f);
            } catch (Exception e) {
                JOptionPane.showMessageDialog(this,
                    "Couldn't save the selected graph image", "Error",
                    JOptionPane.ERROR_MESSAGE);
            }
        }
	}//GEN-LAST:event_exportButtonActionPerformed

	private void ratioMenuActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_ratioMenuActionPerformed
        // Ration menu button
        if (ratioMenu.isSelected()) {
            RatioMenu.show(ratioMenu, 0, ratioMenu.getHeight());
        } else {
            RatioMenu.setVisible(false);
        }
	}//GEN-LAST:event_ratioMenuActionPerformed

	private void RatioMenuPopupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {//GEN-FIRST:event_RatioMenuPopupMenuWillBecomeInvisible
        ratioMenu.setSelected(false);
	}//GEN-LAST:event_RatioMenuPopupMenuWillBecomeInvisible

	private void RatioFunction1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction1ActionPerformed
        // ratio menu > first entry
        double[] rc = {1, 0, 0};
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction1ActionPerformed

	private void RatioFunction2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction2ActionPerformed
        // ratio menu > second entry
        double[] rc = {1, 0, 1};
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction2ActionPerformed

	private void RatioFunction3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction3ActionPerformed
        // ratio menu > third entry
        double[] rc = {1, 1, 0};
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction3ActionPerformed

	private void RatioFunction4ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction4ActionPerformed
        // ratio menu > fourth entry
        double[] rc = {1.5, 0, 0};
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction4ActionPerformed

	private void RatioFunction5ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction5ActionPerformed
        // ratio menu > fifth entry
        double[] rc = {2, 0, 0};
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction5ActionPerformed

	private void RatioFunction6ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction6ActionPerformed
        // ratio menu > sixth entry
        double[] rc = {2.5, 0, 0};
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction6ActionPerformed

	private void RatioFunction7ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction7ActionPerformed
        // ratio menu > seventh entry
        double[] rc = {3, 0, 0};
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction7ActionPerformed

	private void RatioFunction8ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction8ActionPerformed
        // ratio menu > eighth entry
        double[] rc = Input.getRatioConfig();
        rc[2]--;
        if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
            return;
        }
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction8ActionPerformed

	private void RatioFunction9ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction9ActionPerformed
        // ratio menu > ninth entry
        double[] rc = Input.getRatioConfig();
        rc[2]++;
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction9ActionPerformed

	private void RatioFunction10ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction10ActionPerformed
        // ratio menu > 10-th entry
        double[] rc = Input.getRatioConfig();
        rc[1]--;
        if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
            return;
        }
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction10ActionPerformed

	private void RatioFunction11ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction11ActionPerformed
        // ratio menu > 11-th entry
        double[] rc = Input.getRatioConfig();
        rc[1]++;
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction11ActionPerformed

	private void RatioFunction12ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction12ActionPerformed
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
        double[] rc = Input.getRatioConfig();
        rc[2] -= exp;
        if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
            return;
        }
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction12ActionPerformed

	private void RationFunction13ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RationFunction13ActionPerformed
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
        double[] rc = Input.getRatioConfig();
        rc[1] -= exp;
        if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
            return;
        }
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RationFunction13ActionPerformed

	private void RatioFunction14ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_RatioFunction14ActionPerformed
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
        double[] rc = Input.getRatioConfig();
        rc[0] -= exp;
        if (rc[0] == 0 && rc[1] == 0 && rc[2] == 0) {
            javax.swing.JOptionPane.showMessageDialog(main_window, "can't apply setting: division by zero");
            return;
        }
        Input.setRatioConfig(rc);
        updateGraphTitle();
        refresh(true);
	}//GEN-LAST:event_RatioFunction14ActionPerformed

	private void costTypeMenuActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_costTypeMenuActionPerformed
        // Cost type menu
        if (costTypeMenu.isSelected()) {
            CostMenu.show(costTypeMenu, 0, costTypeMenu.getHeight());
        } else {
            CostMenu.setVisible(false);
        }
	}//GEN-LAST:event_costTypeMenuActionPerformed

	private void CostMenuPopupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {//GEN-FIRST:event_CostMenuPopupMenuWillBecomeInvisible
        costTypeMenu.setSelected(false);
	}//GEN-LAST:event_CostMenuPopupMenuWillBecomeInvisible

	private void WorstCostButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_WorstCostButtonActionPerformed
        cost_type = Input.CostKind.WORST;
        if (main_window.arePlotsLinked()) {
            main_window.setGroupCostAll(graph_type, cost_type);
        }
        refresh(true);
	}//GEN-LAST:event_WorstCostButtonActionPerformed

	private void AverageCostButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_AverageCostButtonActionPerformed
        cost_type = Input.CostKind.AVG;
        if (main_window.arePlotsLinked()) {
            main_window.setGroupCostAll(graph_type, cost_type);
        }
        refresh(true);
	}//GEN-LAST:event_AverageCostButtonActionPerformed

	private void BestCostButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_BestCostButtonActionPerformed
        cost_type = Input.CostKind.BEST;
        if (main_window.arePlotsLinked()) {
            main_window.setGroupCostAll(graph_type, cost_type);
        }
        refresh(true);
	}//GEN-LAST:event_BestCostButtonActionPerformed

	private void smoothMenuActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_smoothMenuActionPerformed
        // Smooth point window button
        if (smoothMenu.isSelected()) {
            SmoothMenu.show(smoothMenu, 0, smoothMenu.getHeight());
        } else {
            SmoothMenu.setVisible(false);
        }
	}//GEN-LAST:event_smoothMenuActionPerformed

	private void SmoothMenuPopupMenuWillBecomeInvisible(javax.swing.event.PopupMenuEvent evt) {//GEN-FIRST:event_SmoothMenuPopupMenuWillBecomeInvisible
        smoothMenu.setSelected(false);
	}//GEN-LAST:event_SmoothMenuPopupMenuWillBecomeInvisible

	private void SmoothThresholdButton1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_SmoothThresholdButton1ActionPerformed
        // Smooth window menu: 1-th entry
        setSmoothThreshold(1);
        if (this.main_window.arePlotsLinked()) {
            main_window.setSmoothThresholdAll(graph_type, 1);
        }
	}//GEN-LAST:event_SmoothThresholdButton1ActionPerformed

	private void SmoothThresholdButton2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_SmoothThresholdButton2ActionPerformed
        // Smooth window menu: 2-th entry
        setSmoothThreshold(2);
        if (this.main_window.arePlotsLinked()) {
            main_window.setSmoothThresholdAll(graph_type, 2);
        }
	}//GEN-LAST:event_SmoothThresholdButton2ActionPerformed

	private void SmoothThresholdButton3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_SmoothThresholdButton3ActionPerformed
        // Smooth window menu: 3-th entry
        setSmoothThreshold(3);
        if (this.main_window.arePlotsLinked()) {
            main_window.setSmoothThresholdAll(graph_type, 3);
        }
	}//GEN-LAST:event_SmoothThresholdButton3ActionPerformed

	private void SmoothThresholdButton4ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_SmoothThresholdButton4ActionPerformed
        // Smooth window menu: 4-th entry
        setSmoothThreshold(4);
        if (this.main_window.arePlotsLinked()) {
            main_window.setSmoothThresholdAll(graph_type, 4);
       }
	}//GEN-LAST:event_SmoothThresholdButton4ActionPerformed

	private void SmoothThresholdButton5ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_SmoothThresholdButton5ActionPerformed
        // Smooth window menu: 5-th entry
        setSmoothThreshold(5);
        if (this.main_window.arePlotsLinked()) {
            main_window.setSmoothThresholdAll(graph_type, 5);
        }
	}//GEN-LAST:event_SmoothThresholdButton5ActionPerformed

	private void SmoothThresholdButton6ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_SmoothThresholdButton6ActionPerformed
        // Smooth window menu: 6-th entry
        setSmoothThreshold(6);
        if (this.main_window.arePlotsLinked()) {
            main_window.setSmoothThresholdAll(graph_type, 6);
        }
	}//GEN-LAST:event_SmoothThresholdButton6ActionPerformed

	private void zoomOutButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_zoomOutButtonActionPerformed
        // Zoom out button pressed
        maximize();
        zoomResetVisible(false);
        //chartPanel.repaint();
	}//GEN-LAST:event_zoomOutButtonActionPerformed

    private void alphaChooserActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_alphaChooserActionPerformed
        /*
        double alpha = 1.0;
        if (rtn_info != null) {
            alpha = rtn_info.getAmortizedConstant();
        }
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

        rtn_info.invalidAmortizedCache();
        rtn_info.setAmortizedConstant(alpha);
        refresh(true);
        */
    }//GEN-LAST:event_alphaChooserActionPerformed

    private boolean filterRms(Input t) {

        if (filters == null) {
            return true;
        }

        if (filters[0] != null) {
            try {
                double time = Double.parseDouble(filters[0]);
                if (t.getMaxCost() < time) {
                    return false;
                }
            } catch (Exception e) {
                return true;
            }
        }

        if (filters[1] != null) {
            try {
                int rms = Integer.parseInt(filters[1]);
                if (t.getSize() < rms) {
                    return false;
                }
            } catch (Exception e) {
                return true;
            }
        }

        if (filters[2] != null) {
            try {
                int freq = Integer.parseInt(filters[2]);
                if (t.getCalls() < freq) {
                    return false;
                }
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
        for (int i = 0; i < series.length; i++) {
            series[i].clear();
        }
        plot.clearAnnotations();
        disableNotification(false);

        if (this.rtn_info != null) {

            long max = rtn_info.getMaxInput();
            group_threshold_base = (int) Math.log10(max) + 1;
            smooth_threshold_base = (int) Math.log10(max) + 1;

            // reset temp global vars
            group_threshold = 1;
            GroupThresholdButton1.setSelected(true);
            smooth_threshold = 1;
            SmoothThresholdButton1.setSelected(true);

            elem_slot = 0;
            slot_start = 0;
            sum_y = 0;
            sum_y2 = 0;
            sum_y3 = 0;
            sum_x = 0;
            sum_occ = 0;

            if (rtn_info.getInputTuplesCount() <= 0) {
                GroupThresholdButton1.setText("0");
                GroupThresholdButton2.setText("0");
                GroupThresholdButton3.setText("0");
                GroupThresholdButton4.setText("0");
                GroupThresholdButton5.setText("0");
                GroupThresholdButton6.setText("0");
                SmoothThresholdButton1.setText("0");
                SmoothThresholdButton2.setText("0");
                SmoothThresholdButton3.setText("0");
                SmoothThresholdButton4.setText("0");
                SmoothThresholdButton5.setText("0");
                SmoothThresholdButton6.setText("0");
                return;
            }

            MenuElement[] me = GroupMenu.getSubElements();
            for (int i = 0; i < me.length; i++) {
                int val = (int) Math.pow(group_threshold_base, i);
                ((JRadioButtonMenuItem) me[i]).setText(Integer.toString(val));
            }
            
            me = SmoothMenu.getSubElements();
            for (int i = 0; i < me.length; i++) {
                int val = (int) Math.pow(smooth_threshold_base, i);
                if (val >= rtn_info.getInputTuplesCount())
                    val = rtn_info.getInputTuplesCount() - 1;
                if (val % 2 == 0)
                    val++; // we want an odd number...
               ((JRadioButtonMenuItem) me[i]).setText(Integer.toString(val));
            }            
        }
    }

    public void addFittedLine(double a, double b, double c) {

        if (graph_type != COST_PLOT) {
            return;
        }
        if (rtn_info == null) {
            return;
        }

        series[11].clear();
        plot.clearAnnotations();

        double max = rtn_info.getMaxInput() * 1.05;
        long count = (long) (max / 300);
        if (count <= 1) {
            count = 1;
        }
        for (int i = 0; i < max; i += count) {
            double fy = a + b * Math.pow(i, c);
            series[0].add(i, fy, false);
        }

        String f = String.format("fit = %.2f + %.2f * x^%.2f", a, b, c);

        LegendItemCollection legenditemcollection = new LegendItemCollection();
        LegendItem legenditem2 = new LegendItem(f, "-", null, null,
            Plot.DEFAULT_LEGEND_ITEM_BOX, Color.RED);

        legenditemcollection.add(legenditem2);
        plot.setFixedLegendItems(legenditemcollection);

        if (chart.getLegend() == null) {
            chart.addLegend(new LegendTitle(plot));
        }
    }

    private double getY(Input te, int slot) {

        //System.out.println("Set getY");
        double y = 0;
        if (te == null) {
            return y;
        }

        if (this.graph_type == COST_PLOT) {
            y = te.getCost(cost_type);
        } else if (this.graph_type == RATIO_PLOT) {
            y = te.getRatioCost(cost_type);
        } else if (this.graph_type == TOTALCOST_PLOT) {
            y = te.getSumCost();
        } else if (this.graph_type == VAR_PLOT) {
            y = te.getVar();
        } else if (this.graph_type == MMM_PLOT) {

            if (slot == 0) {
                y = te.getMinCost();
            } else if (slot == 1) {
                y = te.getAvgCost();
            } else if (slot == 2) {
                y = te.getMaxCost();
            }

        } else if (graph_type == AMORTIZED_PLOT) {
            y = rtn_info.getAmortizedValue(te);
        } else if (this.graph_type == FREQ_PLOT) {
            y = te.getCalls();
        } else {
            throw new RuntimeException("Not defined getY behavior: " + graph_type);
        }

        return y;
    }

    public void addPoint(Input te) {

        double x = te.getSize();
        double y = 0;

        if (group_threshold == 1 && smooth_threshold == 1) {

            if (filterRms(te)) {

                if (graph_type == MMM_PLOT) {

                    series[11].add(x, getY(te, 0), false);
                    series[5].add(x, getY(te, 1), false);
                    series[0].add(x, getY(te, 2), false);

                } else if (graph_type == FREQ_PLOT) {

                    int index = 11;
                    series[index].add(x, getY(te, 0), false);

                } else if (graph_type == AMORTIZED_PLOT || graph_type == RATIO_PLOT) {

                    series[11].add(x, getY(te, 0), false);

                } else {

                    y = getY(te, 0);
                    double index = 11 - Math.round(Math.log10(te.getCalls()) / Math.log10(2));
                    if (index > 11) index = 11;
                    if (index < 0) index = 0;
                    series[(int) index].add(x, y, false);
                }
            }

        } else if (group_threshold > 1 && smooth_threshold == 1) {

            int current_slot = (int) ((int) x - (x % group_threshold));
            if (current_slot != slot_start) {

                // Ok, this rms is for a new slot, so we need to create
                // a point in the graph for the old slot, and reset temp
                // global counters and then add current value 
                if (elem_slot > 0) {
                    add_group_point();
                }
                slot_start = current_slot;

            }

            // same slot, add value to current slot
            if (filterRms(te)) {

                sum_x += x;
                sum_y += getY(te, 0);
                sum_occ += te.getCalls(); // need for choosing color

                if (graph_type == MMM_PLOT) {
                    sum_y2 += getY(te, 1);
                    sum_y3 += getY(te, 2);
                }

                elem_slot++;

            }

            // Is this the last point?
            if (x == rtn_info.getMaxInput()) {
                add_group_point();
            }

        } else if (group_threshold == 1 && smooth_threshold > 1) {

            if (x == rtn_info.getMaxInput()) {
                this.setData(rtn_info);
            }

            //throw new RuntimeException("Not yet implemented :(");
        } else {
            throw new RuntimeException("Group and smooth threshold cannot be both greater than one");
        }
    }

    private void add_group_point() {

        double mean_x = sum_x / elem_slot;

        if (graph_type == FREQ_PLOT) {

            series[11].add(mean_x, sum_y, false);

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
            if (index > 11) {
                index = 11;
            }
            if (index < 0) {
                index = 0;
            }
            series[(int) index].add(mean_x, mean_y, false);

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

        for (int i = 0; i < series.length; i++) {
            series[i].clear();
        }

        if (graph_type == AMORTIZED_PLOT) {
            updateGraphTitle();
        }

        if (rtn_info == null || rtn_info.getInputTuplesCount() <= 0) {
            return;
        }

        if (group_threshold == 1 && smooth_threshold == 1) {

            //System.out.println("START################");
            //System.out.println("GroupThreshold == 1");
            Iterator rms_list = this.rtn_info.getInputTuplesIterator();
            while (rms_list.hasNext()) {

                Input te = (Input) rms_list.next();
                addPoint(te);

            }

            //System.out.println("Terminated populate");
        } else if (group_threshold > 1 && smooth_threshold == 1) {

            int n_y = 1;
            if (graph_type == MMM_PLOT) {
                n_y = 3;
            }

            double slot_start = 0;
            double sum_x = 0;
            double sum_y = 0, sum_y2 = 0, sum_y3 = 0;
            long sum_occurrences = 0;
            int t = group_threshold;
            int k = 0;
            Iterator rms_list = this.rtn_info.getInputTuplesIterator();
            while (rms_list.hasNext()) {

                Input te = (Input) rms_list.next();
                if (filterRms(te)) {
                    double x = te.getSize();
                    double y = 0, y2 = 0, y3 = 0;
                    if (graph_type == COST_PLOT) {
                        y = te.getCost(cost_type);
                    } else if (graph_type == RATIO_PLOT) {
                        y = te.getRatioCost(cost_type);
                    } else if (graph_type == TOTALCOST_PLOT) {
                        y = te.getSumCost();
                    } else if (graph_type == VAR_PLOT) {
                        y = te.getVar();
                    } else if (graph_type == FREQ_PLOT) {
                        y = te.getCalls();
                    } else if (graph_type == MMM_PLOT) {
                        y = te.getMinCost();
                        y2 = te.getAvgCost();
                        y3 = te.getMaxCost();
                    }

                    double current_slot = x - (x % t);

                    if (current_slot == slot_start) {
                        sum_x += x;
                        sum_y += y;
                        if (n_y >= 2) {
                            sum_y2 += y2;
                        }
                        if (n_y >= 3) {
                            sum_y3 += y3;
                        }
                        sum_occurrences += te.getCalls();
                        k++;
                    } else {
                        if (k > 0) {
                            double mean_x = (k == 0) ? sum_x : sum_x / k;
                            if (graph_type == FREQ_PLOT) {
                                series[11].add(mean_x, sum_y);
                                if (sum_y > max_y) {
                                    max_y = sum_y;
                                }
                            } else if (graph_type == MMM_PLOT) {
                                double mean_y = (k == 0) ? sum_y : sum_y / k;
                                double mean_y2 = (k == 0) ? sum_y2 : sum_y2 / k;
                                double mean_y3 = (k == 0) ? sum_y3 : sum_y3 / k;
                                series[11].add(mean_x, mean_y);
                                series[5].add(mean_x, mean_y2);
                                series[0].add(mean_x, mean_y3);
                            } else if (graph_type == AMORTIZED_PLOT) {
                                double mean_y = (k == 0) ? sum_y : sum_y / k;
                                series[11].add(mean_x, mean_y);
                            } else {
                                double mean_y = (k == 0) ? sum_y : sum_y / k;
                                double index = 11 - Math.round(Math.log10(sum_occurrences) / Math.log10(2));
                                if (index > 11) index = 11;
                                if (index < 0) index = 0;
                                series[(int) index].add(mean_x, mean_y);
                            }
                            sum_x = x;
                            sum_y = y;
                            if (n_y >= 2) {
                                sum_y2 = y2;
                            }
                            if (n_y >= 3) {
                                sum_y3 = y3;
                            }
                            sum_occurrences = te.getCalls();
                            k = 1;
                        }
                        slot_start = current_slot;
                    }
                }
            }
            if (k > 0) {
                double mean_x = (k == 0) ? sum_x : sum_x / k;
                if (graph_type == FREQ_PLOT) {
                    series[11].add(mean_x, sum_y, false);
                    if (sum_y > max_y) {
                        max_y = sum_y;
                    }
                } else if (graph_type == MMM_PLOT) {
                    double mean_y = (k == 0) ? sum_y : sum_y / k;
                    double mean_y2 = (k == 0) ? sum_y2 : sum_y2 / k;
                    double mean_y3 = (k == 0) ? sum_y3 : sum_y3 / k;
                    series[11].add(mean_x, mean_y, false);
                    series[5].add(mean_x, mean_y2, false);
                    series[0].add(mean_x, mean_y3, false);
                } else {
                    double mean_y = (k == 0) ? sum_y : sum_y / k;
                    double index = 11 - Math.round(Math.log10(sum_occurrences) / Math.log10(2));
                    if (index > 11) index = 11;
                    if (index < 0) index = 0;
                    series[(int) index].add(mean_x, mean_y, false);
                }
            }

        } else if (smooth_threshold > 1 && group_threshold == 1) {

            int t = (int) Math.pow(smooth_threshold_base, smooth_threshold - 1);
            if (t >= rtn_info.getInputTuplesCount()) {
                t = rtn_info.getInputTuplesCount() - 1;
            }
            if (t % 2 == 0) {
                t++;
            }
            // rms items within window; circular array; +2 avoid immediate overwrite
            Input[] l = new Input[t + 2];
            // amortized cache
            double[] lm = new double[t + 2];
            double sum = 0, sum2 = 0, sum3 = 0;
            rtn_info.sortInputTuplesByInput();
            Iterator rms_list = rtn_info.getInputTuplesIterator();

            int tail = 0;
            int current = 0;
            int head = 0;
            double n = 0;

            while (true) {

                int add = 0;
                Input removed = null;

                if (n == t || n == 0) {
                    add = 1;
                } else {
                    add = 2;
                }

                int c = 0;
                while (add > 0) {

                    Input te = null;

                    // extract a valid rms
                    while (rms_list.hasNext()) {

                        te = (Input) rms_list.next();
                        if (filterRms(te)) {
                            break;
                        }

                    }

                    if (te == null) {
                        break;
                    }

                    double y = getY(te, 0);

                    // add to the list
                    l[head % l.length] = te;

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
                    //System.out.println("Rimuovo elemento  - tail: " + tail);
                    removed = l[tail % l.length];
                    tail++;
                    //System.out.println("tail incremented... " +  tail);

                    sum -= getY(removed, 0);
                    if (graph_type == MMM_PLOT) {

                        sum2 -= getY(removed, 1);
                        sum3 -= getY(removed, 2);

                    }
                    
                    //System.out.println("Sottratto: " +  getY(removed, 0));
                    n--;

                    if (add > 0) {

                        //System.out.println("Rimuovo elemento - tail: " + tail);
                        removed = l[tail % l.length];
                        tail++;
                        //System.out.println("tail incremented... " +  tail);

                        sum -= getY(removed, 0);
                        if (graph_type == MMM_PLOT) {
                            sum2 -= getY(removed, 1);
                            sum3 -= getY(removed, 2);
                        }
                        
                        n--;
                    }
                }

                n += c;

                //System.out.println("Aggiungo punto - current: " +  current + " head: " + head);
                double x = l[current % l.length].getSize();

                if (graph_type == MMM_PLOT) {

                    series[11].add(x, sum / n, false);
                    series[5].add(x, sum2 / n, false);
                    series[0].add(x, sum3 / n, false);

                } else {

                    double index = 11 - Math.round(Math.log10(l[current % l.length].getCalls()) / Math.log10(2));
                    if (index > 11) index = 11;
                    if (index < 0) index = 0;
                    series[(int) index].add(x, sum / n, false);
                }

                if (current++ != 0 && n == 1) {
                    break;
                }
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
     * When possible avoid use of this method, better share elaboration
     * between all active graphs (e.g. iteration over rms list)
     */
    public void setData(Routine r) {

        if (r == null) {
            clearData();
            return;
        }

        setRoutine(r);
        refresh(true);
    }

    public void clearData() {

        disableNotification(true);

        rtn_info = null;

        for (int i = 0; i < series.length; i++) {
            series[i].clear();
        }

        if (graph_type == COST_PLOT && chart.getLegend() != null) {
            chart.removeLegend();
        }

        max_x = domainAxis.getUpperBound();
        max_y = rangeAxis.getUpperBound();
        min_x = domainAxis.getLowerBound();
        min_y = rangeAxis.getLowerBound();

        updateXAxis(true);
        updateYAxis(true);

        disableNotification(false);
        System.gc();
    }

    private void refresh(boolean resetAxis) {

        System.out.println("Refreshing chart " + graph_type);
        
        if (perf != null) {
            perf.start(this, PerformanceMonitor.ELABORATE);
        }

        disableNotification(true);
        populateChart();
        disableNotification(false);

        if (resetAxis) {
            maximize();
        }

        if (perf != null) {
            perf.stop(this, PerformanceMonitor.ELABORATE);
        }

        System.gc();
    }

    private void disableNotification(boolean disable) {

        if (disable) {

            chart.setNotify(false);
            for (int i = 0; i < series.length; i++) {
                series[i].setNotify(false);
            }

        } else {

            for (int i = 0; i < series.length; i++) {
                series[i].setNotify(true);
            }
            chart.setNotify(true);
        }
    }

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JRadioButtonMenuItem AverageCostButton;
    private javax.swing.JRadioButtonMenuItem BestCostButton;
    private javax.swing.JPopupMenu CostMenu;
    private javax.swing.JPopupMenu GroupMenu;
    private javax.swing.JRadioButtonMenuItem GroupThresholdButton1;
    private javax.swing.JRadioButtonMenuItem GroupThresholdButton2;
    private javax.swing.JRadioButtonMenuItem GroupThresholdButton3;
    private javax.swing.JRadioButtonMenuItem GroupThresholdButton4;
    private javax.swing.JRadioButtonMenuItem GroupThresholdButton5;
    private javax.swing.JRadioButtonMenuItem GroupThresholdButton6;
    private javax.swing.JMenuItem RatioFunction1;
    private javax.swing.JMenuItem RatioFunction10;
    private javax.swing.JMenuItem RatioFunction11;
    private javax.swing.JMenuItem RatioFunction12;
    private javax.swing.JMenuItem RatioFunction14;
    private javax.swing.JMenuItem RatioFunction2;
    private javax.swing.JMenuItem RatioFunction3;
    private javax.swing.JMenuItem RatioFunction4;
    private javax.swing.JMenuItem RatioFunction5;
    private javax.swing.JMenuItem RatioFunction6;
    private javax.swing.JMenuItem RatioFunction7;
    private javax.swing.JMenuItem RatioFunction8;
    private javax.swing.JMenuItem RatioFunction9;
    private javax.swing.JPopupMenu RatioMenu;
    private javax.swing.JMenuItem RationFunction13;
    private javax.swing.JPopupMenu SmoothMenu;
    private javax.swing.JRadioButtonMenuItem SmoothThresholdButton1;
    private javax.swing.JRadioButtonMenuItem SmoothThresholdButton2;
    private javax.swing.JRadioButtonMenuItem SmoothThresholdButton3;
    private javax.swing.JRadioButtonMenuItem SmoothThresholdButton4;
    private javax.swing.JRadioButtonMenuItem SmoothThresholdButton5;
    private javax.swing.JRadioButtonMenuItem SmoothThresholdButton6;
    private javax.swing.JRadioButtonMenuItem WorstCostButton;
    private javax.swing.JToggleButton XLogToggle;
    private javax.swing.JToggleButton YLogToggle;
    private javax.swing.JButton alphaChooser;
    private aprofplot.gui.ColorLegend colorLegend1;
    private javax.swing.JToggleButton costTypeMenu;
    private javax.swing.JButton exportButton;
    private javax.swing.Box.Filler filler1;
    private javax.swing.JToolBar jToolBar1;
    private javax.swing.JToggleButton pointAggregationMenu;
    private javax.swing.JToggleButton ratioMenu;
    private javax.swing.JToggleButton smoothMenu;
    private javax.swing.JButton zoomOutButton;
    // End of variables declaration//GEN-END:variables
	private javax.swing.ButtonGroup groupMenuButtonGroup;
    private javax.swing.ButtonGroup groupMenuButtonGroup2;
    private javax.swing.ButtonGroup groupMenuButtonGroup3;
    private javax.swing.ButtonGroup toggleButtonGroup;
}
