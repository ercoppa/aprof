/* ===========================================================
 * JFreeChart : a free chart library for the Java(tm) platform
 * ===========================================================
 *
 * (C) Copyright 2009, by Object Refinery Limited and Contributors.
 *
 * Project Info:  http://www.jfree.org/jfreechart/index.html
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * [Java is a trademark or registered trademark of Sun Microsystems, Inc.
 * in the United States and other countries.]
 *
 * ---------------------------
 * SamplingXYLineAndShapeRenderer.java
 * ---------------------------
 * (C) Copyright 2009, by Object Refinery Limited.
 *
 * Original Author:  PK;
 * Contributor(s):   -;
 *
 * Changes:
 * --------
 * 04-Jun-2009 : Version 1 ();
 *
 */
package aprofplot.jfreechart;

import java.awt.Graphics2D;
import java.awt.Paint;
import java.awt.Shape;
import java.awt.Stroke;
import java.awt.geom.GeneralPath;
import java.awt.geom.PathIterator;
import java.awt.geom.Rectangle2D;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.text.MessageFormat;
import java.util.BitSet;

import org.jfree.chart.axis.ValueAxis;
import org.jfree.chart.event.RendererChangeEvent;
import org.jfree.chart.entity.EntityCollection;
import org.jfree.chart.plot.CrosshairState;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.plot.PlotRenderingInfo;
import org.jfree.chart.plot.SeriesRenderingOrder;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.RendererUtilities;
import org.jfree.chart.renderer.xy.XYItemRendererState;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
//import org.jfree.geom.LinedShape;
import org.jfree.data.xy.XYDataset;
import org.jfree.util.PublicCloneable;
import org.jfree.util.ShapeUtilities;
import org.jfree.ui.RectangleEdge;

/**
 * A renderer for an XYPlot. Items are drawn as a combination of lines and/or shapes. Shapes that would not be visible are not rendered.
 * The drawing of lines is reduced to those lines that contribute to the visual appearance of the chart 
 * in way similar to that uses by the {@link SamplingXYLineRenderer}.
 * The rendering options shapes, strokes, paints, and flags (e. g. items filled, outlines visible) are determined 
 * ahead of the rendering of the individual items. Item-specific settings for these parameters are thus not respected.
 */
public class SamplingXYLineAndShapeRenderer extends XYLineAndShapeRenderer
        implements Cloneable, PublicCloneable, Serializable {

    /**
     * The width and height in Java2D units that a data point should occupy.
     */
    private int shapeSize;
    /**
     * The width in Java2D units that a line should occupy.
     */
    private int lineWidth = 1;
    /**
     * A flag that indicates whether data items should be sampled across series.
     *
     */
    private boolean sampleDataset = false;

    /**
     * Creates a new renderer with both lines and shapes visible and a shapeSize of 1.
     */
    public SamplingXYLineAndShapeRenderer() {
        this(true, true);
    }

    /**
     * Creates a new renderer with a shapeSize of 1 and lines / shapes visible as indicated by the flags.
     */
    public SamplingXYLineAndShapeRenderer(boolean lines, boolean shapes) {
        this(lines, shapes, 1);
    }

    /**
     * Creates a new renderer with a shapeSizeas defined and lines / shapes visible as indicated by the flags.
     */
    public SamplingXYLineAndShapeRenderer(boolean lines, boolean shapes, int shapeSize) {
        super(lines, shapes);
        this.shapeSize = shapeSize;
    }

    /**
     * Sets a flag that indicates whether data items that would be covered by data items from another series
     * should be omitted and sens a {@link org.jfree.chart.event.RendererChangeEvent} to registered listeners.
     *
     * @param flag  the flag.
     *
     */
    public void setSampleDataset(boolean flag) {
        this.sampleDataset = flag;
        fireChangeEvent();
    }

    /**
     * Returns a flag that indicates whether data items that would be covered by data items from another series
     * should be omitted.
     *
     * @return  The flag.
     *
     */
    public boolean getSampleDataset() {
        return this.sampleDataset;
    }

    /**
     * Sets the size that a shape occupies when drawn in the data area.
     *
     * @param shapeSize  the size.
     *
     */
    public void setShapeSize(int shapeSize) {
        this.shapeSize = shapeSize;
        fireChangeEvent();
    }

    /**
     * Returns the size that a shape occupies when drawn in the data area.
     * @return The shape size.
     *
     */
    public int getShapeSize() {
        return this.shapeSize;
    }

    /**
     * Sets the width in Java2D that a group of data points can span along the domain axis 
     * while still being plotted as a single part of the intervalPath.
     *
     * @param lineWidth  the width.
     *
     */
    public void setLineWidth(int lineWidth) {
        this.lineWidth = lineWidth;
        fireChangeEvent();
    }

    /**
     * Returns the width in Java2D that a group of data points can span along the domain axis 
     * while still being plotted as a single part of the intervalPath.
     *
     * @return  the line width.
     *
     */
    public int getLineWidth() {
        return this.lineWidth;
    }

    /**
     * Returns the number of passes through the data that the renderer requires
     * in order to draw the chart.  Returns 1 since all lines /shapes are drawn
     * in a single pass for performance reasons.
     *
     * @return The pass count.
     */
	@Override
    public int getPassCount() {
        return 1;
    }

    /**
     * Records the state for the renderer.  This is used to preserve state
     * information between calls to the drawItem() method for a single chart
     * drawing.
     */
    public static class State extends XYItemRendererState {

        /** The path for the current series. */
        GeneralPath seriesPath;
        /**
         * A second path that draws vertical intervals to cover any extreme
         * values.
         */
        GeneralPath intervalPath;
        /**
         * An array of BitSets to store the indices of visible data items per series.
         */
        BitSet[] visiblePoints;
        /**
         * A BitSet to indicate for which series the sampling should be performed.
         */
        BitSet series2Sample;
        /**
         * A BitSet to store the indices of points /pixels in the dataArea that are covered by data items.
         */
        BitSet renderedPixels;
        int[][] itemBounds;
        /**
         * The minimum change in the x-value needed to trigger an update to
         * the seriesPath.
         */
        //double dX = 1.0;
        /** The last x-coordinate visited by the seriesPath. */
        //double lastX = Double.NEGATIVE_INFINITY;
        //double lastY = Double.NEGATIVE_INFINITY;
        /** The initial y-coordinate for the current x-coordinate. */
        //double openY = 0.0;
        /** The highest y-coordinate for the current x-coordinate. */
        //double highY = 0.0;
        /** The lowest y-coordinate for the current x-coordinate. */
        //double lowY = 0.0;
        /** The final y-coordinate for the current x-coordinate. */
        //double closeY = 0.0;
        /**
         * The lower bound of the domain axis.
         */
        double lowDataX;
        /**
         * The upper bound of the domain axis.
         */
        double highDataX;
        /**
         * The difference between lowDataX and highDataX. Required to calculate the resolution (data units/Java2D units).
         */
        double rangeX;
        /**
         * The lower bound of the range axis.
         */
        double lowDataY;
        /**
         * The upper bound of the range axis.
         */
        double highDataY;
        /**
         * The difference between lowDataY and highDataY. Required to calculate the resolution (data units/Java2D units).
         */
        double rangeY;
        /**
         * The available number of pixels along the domain axis. Required to calculate the index of pixels occupied by data points.
         */
        int xPixels;
        /**
         * The length of the data area along the domain axis.
         */
        double java2DWidth;
        /**
         * The length of the data area along the range axis.
         */
        double java2DHeight;
        /**
         * The width and height in Java2D units that a data point should occupy.
         */
        private int shapeSize;
        /**
         * The width in Java2D units that a line should occupy.
         */
        private int lineWidth = 1;
        /**
         * A a flag that indicates whether the entire dataset should be sampled for overlapping data items.
         *
         */
        boolean sampleDataset;
        /**
         * A flag that indicates whether a series has been drawn (required to return from the drawItem method if possible).
         *
         */
        boolean seriesDrawn = false;

        /**
         * Creates a new state instance.
         *
         * @param info  the plot rendering info.
         * @param sampleDataset  a flag that indicates whether the entire dataset should be sampled for overlapping data items.
         * @param plot  the plot.
         * @param dataset  the dataset.
         * @param dataArea  the data area.
         */
        public State(PlotRenderingInfo info, boolean sampleDataset, XYPlot plot, XYDataset dataset, Rectangle2D dataArea, int shapeSize, int lineWidth) {
            super(info);
            visiblePoints = new BitSet[dataset.getSeriesCount()];
            this.sampleDataset = sampleDataset;
            this.shapeSize = shapeSize;
            this.lineWidth = lineWidth;

            renderedPixels = new BitSet();
            series2Sample = new BitSet();
            initializeBounds(plot, dataset, dataArea);
            itemBounds = new int[2][dataset.getSeriesCount()];
            setProcessVisibleItemsOnly(false);
            for (int series = 0; series < dataset.getSeriesCount(); series++) {
                int[] seriesItemBounds = RendererUtilities.findLiveItems(dataset, series, lowDataX, highDataX);
                itemBounds[0][series] = Math.max(seriesItemBounds[0] - 1, 0);
                itemBounds[1][series] = Math.min(seriesItemBounds[1] + 1, dataset.getItemCount(series) - 1);
            }
        }

        /**
         * This method is called by the {@link XYPlot} at the start of each
         * series pass.  We reset the state for the current series.
         *
         * @param dataset  the dataset.
         * @param series  the series index.
         * @param firstItem  the first item index for this pass.
         * @param lastItem  the last item index for this pass.
         * @param pass  the current pass index.
         * @param passCount  the number of passes.
         */
		@Override
        public void startSeriesPass(XYDataset dataset, int series,
                int firstItem, int lastItem, int pass, int passCount) {
            this.seriesPath.reset();
            this.intervalPath.reset();
            this.seriesDrawn = false;
            if (!sampleDataset && series2Sample.get(series)) {
                this.renderedPixels.clear();
                visiblePoints[series] = findVisiblePointsInSeries(dataset, series, this.renderedPixels);
            }
            super.startSeriesPass(dataset, series, itemBounds[0][series], itemBounds[1][series], pass,
                    passCount);
        }

        /**
         * Returns a flag that indicates whether the given data item in the given series
         * contributes to the appearance of the plot and should be rendered.
         *
         * @param series  the series index.
         * @param itemIndex  the item index.
         *
         * @return the flag..
         */
        public boolean itemVisible(int series, int itemIndex) {
            return visiblePoints[series].get(itemIndex);
        }

        /**
         * Checks the dataset for overlapping items across series.
         *
         * @param dataset  the dataset.
         * @param series  the series index.
         * @param pixels  the BitSet with "marked" "occupied" pixels.
         *
         * @return the BitSet.
         */
        void findVisiblePointsInDataset(XYPlot plot, XYDataset dataset) {
            if (!sampleDataset) {
                return;
            }
            int seriesCount = dataset.getSeriesCount();
            SeriesRenderingOrder order = plot.getSeriesRenderingOrder();
            if (order == SeriesRenderingOrder.REVERSE) {
                for (int series = 0; series < seriesCount; series++) {
                    if (series2Sample.get(series)) {
                        visiblePoints[series] = findVisiblePointsInSeries(dataset, series, this.renderedPixels);
                    } else {
                        visiblePoints[series] = new BitSet();
                    }
                }
            } else if (order == SeriesRenderingOrder.FORWARD) {
                for (int series = seriesCount - 1; series >= 0; series--) {
                    if (series2Sample.get(series)) {
                        visiblePoints[series] = findVisiblePointsInSeries(dataset, series, this.renderedPixels);
                    } else {
                        visiblePoints[series] = new BitSet();
                    }
                }
            }
        }

        /**
         * Returns a BitSet where set bits mark the indices of the visible items, 
         * i.e. items that contribute to the visual apperance of the chart.
         *
         * @param dataset  the dataset.
         * @param series  the series index.
         * @param pixels  the BitSet with "marked" "occupied" pixels.
         *
         * @return the BitSet.
         */
        private BitSet findVisiblePointsInSeries(XYDataset dataset, int series, BitSet pixels) {
            int itemCount = dataset.getItemCount(series);
            BitSet markedPoints = new BitSet(dataset.getItemCount(series));
            for (int itemIndex = itemBounds[1][series]; itemIndex >= itemBounds[0][series]; itemIndex--) {
                double x = dataset.getXValue(series, itemIndex);
                if (!Double.isNaN(x) && x >= lowDataX && x <= highDataX) { //already checked whether value is in range with RendererUtilities.findLiveItems
                    double y = dataset.getYValue(series, itemIndex);
                    if ((y >= lowDataY) && (y <= highDataY)) { //Double.NaN won�t come beyond this point
                        int transX = (int) (Math.rint((x - lowDataX) / rangeX * java2DWidth / shapeSize));
                        int transY = (int) (Math.rint((y - lowDataY) / rangeY * java2DHeight / shapeSize));
                        int itemPosition = transY * xPixels + transX;
                        if (!pixels.get(itemPosition)) {
                            pixels.set(itemPosition);
                            markedPoints.set(itemIndex);
                        }
                    }
                }
            }
            return markedPoints;
        }

        /**
         * This method is called from the contructor of the state object.
         * The following variables are initialized in this method:
         * lowDataX, highDataX, rangeX, lowDataY, highDataY, rangeY, java2DWidth, java2DHeight, xPixels.
         *
         * @param plot  the plot.
         * @param dataset  the dataset.
         * @param dataArea  the data area.
         */
        private void initializeBounds(XYPlot plot, XYDataset dataset, Rectangle2D dataArea) {
            int index = -1;
            for (int i = 0; i < plot.getDatasetCount(); i++) {
                if (plot.getDataset(i) == dataset) {
                    index = i;
                    break;
                }
            }
            PlotOrientation orientation = plot.getOrientation();
            ValueAxis domainAxis = plot.getDomainAxisForDataset(index);
            ValueAxis rangeAxis = plot.getRangeAxisForDataset(index);
            lowDataX = domainAxis.getLowerBound();
            highDataX = domainAxis.getUpperBound();
            rangeX = highDataX - lowDataX;

            lowDataY = rangeAxis.getLowerBound();
            highDataY = rangeAxis.getUpperBound();
            rangeY = highDataY - lowDataY;

            java2DWidth = dataArea.getWidth();
            java2DHeight = dataArea.getHeight();
            if (orientation == PlotOrientation.HORIZONTAL) {
                java2DWidth = dataArea.getHeight();
                java2DHeight = dataArea.getWidth();
            }
            xPixels = (int) (Math.rint(java2DWidth / shapeSize));
        }
    }

    /**
     * Initialises the renderer.
     * <P>
     * This method will be called before the first item is rendered, giving the
     * renderer an opportunity to initialise any state information it wants to
     * maintain.  The renderer can do nothing if it chooses.
     *
     * @param g2  the graphics device.
     * @param dataArea  the area inside the axes.
     * @param plot  the plot.
     * @param dataset  the dataset.
     * @param info  an optional info collection object to return data back to
     *              the caller.
     *
     * @return The renderer state.
     */
	@Override
    public XYItemRendererState initialise(Graphics2D g2,
            Rectangle2D dataArea, XYPlot plot, XYDataset dataset,
            PlotRenderingInfo info) {

        State state = new State(info, sampleDataset, plot, dataset, dataArea, shapeSize, lineWidth);
        state.series2Sample = new BitSet(dataset.getSeriesCount());
        for (int series = 0; series < dataset.getSeriesCount(); series++) {
            if ((getItemVisible(series, 0) && getItemShapeVisible(series, 0)) || getItemCreateEntity(series, 0)) {
                state.series2Sample.set(series);
            }
        }
        double dpi = 72;
        //        Integer dpiVal = (Integer) g2.getRenderingHint(HintKey.DPI);
        //        if (dpiVal != null) {
        //            dpi = dpiVal.intValue();
        //        }
        state.seriesPath = new GeneralPath();
        state.intervalPath = new GeneralPath();
        state.findVisiblePointsInDataset(plot, dataset);
        //state.dX = 72.0 / dpi;
        return state;
    }

    /**
     * Draws the visual representation of a single data item as shape and/or as a line.
     * This method immediately returns if the first item of a series has been rendered!
     *
     * @param g2  the graphics device.
     * @param state  the renderer state.
     * @param dataArea  the area within which the data is being drawn.
     * @param info  collects information about the drawing.
     * @param plot  the plot (can be used to obtain standard color
     *              information etc).
     * @param domainAxis  the domain axis.
     * @param rangeAxis  the range axis.
     * @param dataset  the dataset.
     * @param series  the series index (zero-based).
     * @param item  the item index (zero-based).
     * @param crosshairState  crosshair information for the plot
     *                        (<code>null</code> permitted).
     * @param pass  the pass index.
     */
	@Override
    public void drawItem(Graphics2D g2,
            XYItemRendererState state,
            Rectangle2D dataArea,
            PlotRenderingInfo info,
            XYPlot plot,
            ValueAxis domainAxis,
            ValueAxis rangeAxis,
            XYDataset dataset,
            int series,
            int item,
            CrosshairState crosshairState,
            int pass) {

        State s = (State) state;
        if (s.seriesDrawn) {
            return;
        }
        if (!getItemVisible(series, item)) {
            return;
        }
        s.seriesDrawn = true;
        if (getItemLineVisible(series, item)) {
            PlotOrientation orientation = plot.getOrientation();
            if (orientation.equals(PlotOrientation.VERTICAL)) {
                drawVerticalSeriesLine(g2,
                        s,
                        dataArea,
                        info,
                        plot,
                        domainAxis,
                        rangeAxis,
                        dataset,
                        series,
                        crosshairState,
                        pass);

            } else {
                drawSeriesLine(g2,
                        s,
                        dataArea,
                        info,
                        plot,
                        domainAxis,
                        rangeAxis,
                        dataset,
                        series,
                        crosshairState,
                        pass);
            }
        }
        drawSeriesShapes(g2,
                s,
                dataArea,
                info,
                plot,
                domainAxis,
                rangeAxis,
                dataset,
                series,
                crosshairState,
                pass,
                getItemShapeVisible(series, item),
                getItemCreateEntity(series, item));
    }

    /**
     * Draws the visual representation of a series as shapes.
     *
     * @param g2  the graphics device.
     * @param state  the renderer state.
     * @param dataArea  the area within which the data is being drawn.
     * @param info  collects information about the drawing.
     * @param plot  the plot (can be used to obtain standard color
     *              information etc).
     * @param domainAxis  the domain axis.
     * @param rangeAxis  the range axis.
     * @param dataset  the dataset.
     * @param series  the series index (zero-based).
     * @param crosshairState  crosshair information for the plot
     *                        (<code>null</code> permitted).
     * @param pass  the pass index.
     * @param drawShapes  a flag indicating whether shapes should be drawn
     * @param createEntities  a flag indicating whether entities should be
     * generated.
     */
    protected void drawSeriesShapes(Graphics2D g2,
            State state,
            Rectangle2D dataArea,
            PlotRenderingInfo info,
            XYPlot plot,
            ValueAxis domainAxis,
            ValueAxis rangeAxis,
            XYDataset dataset,
            int series,
            CrosshairState crosshairState,
            int pass,
            boolean drawShapes,
            boolean createEntities) {
        if (!drawShapes && !createEntities) {
            return;
        }
        PlotOrientation orientation = plot.getOrientation();
        RectangleEdge xAxisLocation = plot.getDomainAxisEdge();
        RectangleEdge yAxisLocation = plot.getRangeAxisEdge();

        double x = 0.0;
        double y = 0.0;
        double transX = 0.0;
        double transY = 0.0;
        boolean itemShapeFilled = getItemShapeFilled(series, 0);
        boolean drawOutlines = getDrawOutlines();
        Paint itemFillPaint = getUseFillPaint() ? getItemFillPaint(series, 0) : getItemPaint(series, 0);
        Paint itemOutlinePaint = getUseOutlinePaint() ? getItemOutlinePaint(series, 0) : getItemPaint(series, 0);
        Stroke itemOutlineStroke = getItemOutlineStroke(series, 0);
        Shape centeredShape = getItemShape(series, 0);
		/*
        if(centeredShape instanceof LinedShape){
            itemOutlinePaint = itemFillPaint;
            drawOutlines = true;
            itemShapeFilled = false;
        }
		*/
        //draw items
        //and create entities
        EntityCollection entities = null;
        if (info != null) {
            entities = info.getOwner().getEntityCollection();
        }
        int domainAxisIndex = plot.getDomainAxisIndex(domainAxis);
        int rangeAxisIndex = plot.getRangeAxisIndex(rangeAxis);

        for (int itemIndex = state.getFirstItemIndex(); itemIndex <= state.getLastItemIndex(); itemIndex++) {
            if (state.itemVisible(series, itemIndex)) {
                x = dataset.getXValue(series, itemIndex);
                y = dataset.getYValue(series, itemIndex);
                transX = domainAxis.valueToJava2D(x, dataArea, xAxisLocation);
                transY = rangeAxis.valueToJava2D(y, dataArea, yAxisLocation);
                if (orientation == PlotOrientation.HORIZONTAL) {
                    double temp = transX;
                    transX = transY;
                    transY = temp;
                }
                updateCrosshairValues(crosshairState, x, y, domainAxisIndex, rangeAxisIndex, transX, transY, orientation);

                Shape shape = ShapeUtilities.createTranslatedShape(centeredShape, transX, transY);

                if (drawShapes) {
                    if (itemShapeFilled) {
                        g2.setPaint(itemFillPaint);
                        g2.fill(shape);
                    }
                    if (drawOutlines) {
                        g2.setPaint(itemOutlinePaint);
                        g2.setStroke(itemOutlineStroke);
                        g2.draw(shape);
                    }
                }
                if (createEntities && entities != null) {
                    addEntity(entities, shape, dataset, series, itemIndex, transX, transY);
                }
            }
        }
    }

    /**
     * Draws the visual representation of a series as lines.
     *
     * @param g2  the graphics device.
     * @param state  the renderer state.
     * @param dataArea  the area within which the data is being drawn.
     * @param info  collects information about the drawing.
     * @param plot  the plot (can be used to obtain standard color
     *              information etc).
     * @param domainAxis  the domain axis.
     * @param rangeAxis  the range axis.
     * @param dataset  the dataset.
     * @param series  the series index (zero-based).
     * @param crosshairState  crosshair information for the plot
     *                        (<code>null</code> permitted).
     * @param pass  the pass index.
     */
    protected void drawVerticalSeriesLine(Graphics2D g2,
            XYItemRendererState state,
            Rectangle2D dataArea,
            PlotRenderingInfo info,
            XYPlot plot,
            ValueAxis domainAxis,
            ValueAxis rangeAxis,
            XYDataset dataset,
            int series,
            CrosshairState crosshairState,
            int pass) {

        State s = (State) state;
        RectangleEdge xAxisLocation = plot.getDomainAxisEdge();
        RectangleEdge yAxisLocation = plot.getRangeAxisEdge();
        double lowestVisibleX = domainAxis.getLowerBound();
        double highestVisibleX = domainAxis.getUpperBound();
        double width = dataArea.getWidth();
        double dX = (highestVisibleX - lowestVisibleX) / width * lineWidth;
        double lowestVisibleY = rangeAxis.getLowerBound();
        double highestVisibleY = rangeAxis.getUpperBound();

        double lastX = Double.NEGATIVE_INFINITY;
        double lastY = 0.0;
        double highY = 0.0;
        double lowY = 0.0;
        double openY = 0.0;
        double closeY = 0.0;
        boolean lastIntervalDone = false;
        boolean currentPointVisible = false;
        boolean lastPointVisible = false;
        boolean lastPointGood = false;
        boolean lastPointInInterval = false;
        boolean pathStarted = false;
        for (int itemIndex = state.getFirstItemIndex(); itemIndex <= state.getLastItemIndex(); itemIndex++) {
            double x = dataset.getXValue(series, itemIndex);
            double y = dataset.getYValue(series, itemIndex);
            if (!Double.isNaN(x) && !Double.isNaN(y)) {
                //System.out.println ("Breakpoint 736: pathStarted " + pathStarted);
                if (!pathStarted) {
                    float startX = (float) domainAxis.valueToJava2D(x, dataArea, xAxisLocation);
                    float startY = (float) rangeAxis.valueToJava2D(y, dataArea, yAxisLocation);
                    s.seriesPath.moveTo(startX, startY);
                    pathStarted = true;
                }
                if ((Math.abs(x - lastX) > dX)) {
                    //System.out.println ("Breakpoint 744: leaving interval ");
                    //in any case, add the interval that we are about to leave to the intervalPath
                    float intervalPathX = 0.0f;
                    float intervalPathStartY = 0.0f;
                    float intervalPathEndY = 0.0f;
                    float seriesPathCurrentX = 0.0f;
                    float seriesPathCurrentY = 0.0f;
                    float seriesPathLastX = 0.0f;
                    float seriesPathLastY = 0.0f;

                    //first set some variables
                    intervalPathX = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                    intervalPathStartY = (float) rangeAxis.valueToJava2D(lowY, dataArea, yAxisLocation);
                    intervalPathEndY = (float) rangeAxis.valueToJava2D(highY, dataArea, yAxisLocation);
                    seriesPathCurrentX = (float) domainAxis.valueToJava2D(x, dataArea, xAxisLocation);
                    seriesPathLastX = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                    seriesPathCurrentY = (float) rangeAxis.valueToJava2D(y, dataArea, yAxisLocation);
                    seriesPathLastY = (float) rangeAxis.valueToJava2D(closeY, dataArea, yAxisLocation);
                    /*if((lowY - highY) < 1){
                    lastPointInInterval = false;
                    }*/
                    //Double[] values = new Double[]{new Double(openY), new Double(closeY), new Double(highY), new Double(lowY), new Double(lastX)};
                    /*MessageFormat mf = new MessageFormat("openY {0,number, integer},"
                            + " closeY {1,number, integer}"
                            + " highY {2,number, integer}"
                            + " lowY {3,number, integer}"
                            + "lastX {4,number, integer}");*/
                    //String text = mf.format(values);
                    //System.out.println("Breakpoint 772"  + text);
                    boolean drawInterval = false;
                    if (openY >= closeY) {
                        if (highY > openY || lowY < closeY) {
                            drawInterval = true;
                        }
                    }
                    if (openY < closeY) {
                        if (lowY < openY || highY > closeY) {
                            drawInterval = true;
                        }
                    }
                    //System.out.println("Breakpoint 784, drawInterval "  + drawInterval);
                    if (drawInterval) {
                        s.intervalPath.moveTo(intervalPathX, intervalPathStartY);
                        s.intervalPath.lineTo(intervalPathX, intervalPathEndY);
                    }
                    lastIntervalDone = true;

                    //now the series path
                    currentPointVisible = ((x >= lowestVisibleX) && (x <= highestVisibleX) && (y >= lowestVisibleY) && (y <= highestVisibleY));
                    if (!lastPointGood && !lastPointInInterval) {//last point not valid --
                        if (currentPointVisible) {//--> if the current position is visible move seriesPath cursor to the current position
                            s.seriesPath.moveTo(seriesPathCurrentX, seriesPathCurrentY);
                        }
                    } else {//last point valid
                        //if the last point was visible and not part of an interval,
                        //we have already moved the seriesPath cursor to the last point, either with or without drawingh a line
                        //thus we only need to draw a line to the current position
                        if (lastPointInInterval && lastPointGood) {
                            s.seriesPath.lineTo(seriesPathLastX, seriesPathLastY);
                            s.seriesPath.lineTo(seriesPathCurrentX, seriesPathCurrentY);
                        } //if the last point was not visible or part of an interval, we have just stored the y values of the last point
                        //and not yet moved the seriesPath cursor. Thus, we need to move the cursor to the last point without drawing
                        //and draw a line to the current position.
                        else {
                            s.seriesPath.lineTo(seriesPathCurrentX, seriesPathCurrentY);
                        }
                    }
                    lastPointVisible = currentPointVisible;
                    lastX = x;
                    lastY = y;
                    highY = y;
                    lowY = y;
                    openY = y;
                    closeY = y;
                    lastPointInInterval = false;
                } else {
                    lastIntervalDone = false;
                    lastPointInInterval = true;
                    highY = Math.max(highY, y);
                    lowY = Math.min(lowY, y);
                    closeY = y;
                }
                lastPointGood = true;
            } else {
                lastPointGood = false;
            }
        }
        // if this is the last item, draw the path ...
        // draw path, but first check whether we need to complete an interval
        if (!lastIntervalDone) {
            if (lowY < highY) {
                float intervalX = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                float intervalStartY = (float) rangeAxis.valueToJava2D(lowY, dataArea, yAxisLocation);
                float intervalEndY = (float) rangeAxis.valueToJava2D(highY, dataArea, yAxisLocation);
                s.intervalPath.moveTo(intervalX, intervalStartY);
                s.intervalPath.lineTo(intervalX, intervalEndY);
            }
        }
        PathIterator pi = s.seriesPath.getPathIterator(null);
        g2.setStroke(getItemStroke(series, 0));
        g2.setPaint(getItemPaint(series, 0));
        g2.draw(s.seriesPath);
        g2.draw(s.intervalPath);
    }

    protected void drawSeriesLine(Graphics2D g2,
            XYItemRendererState state,
            Rectangle2D dataArea,
            PlotRenderingInfo info,
            XYPlot plot,
            ValueAxis domainAxis,
            ValueAxis rangeAxis,
            XYDataset dataset,
            int series,
            CrosshairState crosshairState,
            int pass) {

        State s = (State) state;
        PlotOrientation orientation = plot.getOrientation();
        RectangleEdge xAxisLocation = plot.getDomainAxisEdge();
        RectangleEdge yAxisLocation = plot.getRangeAxisEdge();
        double lowestVisibleX = domainAxis.getLowerBound();
        double highestVisibleX = domainAxis.getUpperBound();
        double width = (orientation == PlotOrientation.HORIZONTAL) ? dataArea.getHeight() : dataArea.getWidth();
        double dX = (highestVisibleX - lowestVisibleX) / width * lineWidth;
        double lowestVisibleY = rangeAxis.getLowerBound();
        double highestVisibleY = rangeAxis.getUpperBound();

        double lastX = Double.NEGATIVE_INFINITY;
        double lastY = 0.0;
        double highY = 0.0;
        double lowY = 0.0;
        double closeY = 0.0;
        boolean lastIntervalDone = false;
        boolean currentPointVisible = false;
        boolean lastPointVisible = false;
        boolean lastPointGood = false;
        boolean lastPointInInterval = false;
        int intervalCount = 0;
        int badPoints = 0;
        for (int itemIndex = state.getFirstItemIndex(); itemIndex <= state.getLastItemIndex(); itemIndex++) {
            double x = dataset.getXValue(series, itemIndex);
            double y = dataset.getYValue(series, itemIndex);
            if (!Double.isNaN(x) && !Double.isNaN(y)) {
                if ((Math.abs(x - lastX) > dX)) {
                    //System.out.println("Breakpoint 1: leaving interval");
                    //in any case, add the interval that we are about to leave to the intervalPath
                    float intervalStartX = 0.0f;
                    float intervalEndX = 0.0f;
                    float intervalStartY = 0.0f;
                    float intervalEndY = 0.0f;
                    float currentX = 0.0f;
                    float currentY = 0.0f;
                    float lastFX = 0.0f;
                    float lastFY = 0.0f;

                    //first set some variables
                    if (orientation == PlotOrientation.VERTICAL) {
                        intervalStartX = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                        intervalEndX = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                        intervalStartY = (float) rangeAxis.valueToJava2D(lowY, dataArea, yAxisLocation);
                        intervalEndY = (float) rangeAxis.valueToJava2D(highY, dataArea, yAxisLocation);
                        currentX = (float) domainAxis.valueToJava2D(x, dataArea, xAxisLocation);
                        lastFX = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                        currentY = (float) rangeAxis.valueToJava2D(y, dataArea, yAxisLocation);
                        lastFY = (float) rangeAxis.valueToJava2D(closeY, dataArea, yAxisLocation);
                    } else {
                        intervalStartX = (float) rangeAxis.valueToJava2D(lowY, dataArea, yAxisLocation);
                        intervalEndX = (float) rangeAxis.valueToJava2D(highY, dataArea, yAxisLocation);
                        intervalStartY = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                        intervalEndY = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                        currentX = (float) rangeAxis.valueToJava2D(y, dataArea, yAxisLocation);
                        lastFX = (float) rangeAxis.valueToJava2D(closeY, dataArea, yAxisLocation);
                        currentY = (float) domainAxis.valueToJava2D(x, dataArea, xAxisLocation);
                        lastFY = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                    }
                    if ((lowY - highY) < 1) {
                        //System.out.println("Breakpoint 2: setting lastPointInInterval");
                        lastPointInInterval = false;
                    }
                    //System.out.println("Breakpoint 3: lastPointInInterval: " +lastPointInInterval);
                    if ((lowY < highY)) {
                        intervalCount++;
                        //System.out.println("Breakpoint 4: adding segment to interval path:" );
                        //System.out.println("xStart" + intervalStartX + ", yStart " + intervalStartY + ", xEnd " + intervalEndX + ", yEnd " + intervalEndY);
                        s.intervalPath.moveTo(intervalStartX, intervalStartY);
                        s.intervalPath.lineTo(intervalEndX, intervalEndY);
                        lastIntervalDone = true;
                    }

                    //now the series path
                    currentPointVisible = ((x >= lowestVisibleX) && (x <= highestVisibleX) && (y >= lowestVisibleY) && (y <= highestVisibleY));
                    if (!lastPointGood) {//last point not valid --
                        badPoints++;
                        if (currentPointVisible) {//--> if the current position is visible move seriesPath cursor to the current position
                            s.seriesPath.moveTo(currentX, currentY);
                        }
                    } else {//last point valid
                        //if the last point was visible and not part of an interval,
                        //we have already moved the seriesPath cursor to the last point, either with or without drawingh a line
                        //thus we only need to draw a line to the current position
                        if (lastPointVisible && !lastPointInInterval) {
                            s.seriesPath.lineTo(currentX, currentY);
                        } //if the last point was not visible or part of an interval, we have just stored the y values of the last point
                        //and not yet moved the seriesPath cursor. Thus, we need to move the cursor to the last point without drawing
                        //and draw a line to the current position.
                        else {
                            s.seriesPath.moveTo(lastFX, lastFY);
                            s.seriesPath.lineTo(currentX, currentY);
                        }
                    }
                    lastPointVisible = currentPointVisible;
                    lastX = x;
                    lastY = y;
                    highY = y;
                    lowY = y;
                    closeY = y;
                    lastPointInInterval = false;
                } else {
                    lastIntervalDone = false;
                    lastPointInInterval = true;
                    highY = Math.max(highY, y);
                    lowY = Math.min(lowY, y);
                    closeY = y;
                }
                lastPointGood = true;
            } else {
                lastPointGood = false;
            }
        }
        // if this is the last item, draw the path ...
        // draw path, but first check whether we need to complete an interval
        if (!lastIntervalDone) {
            if (lowY < highY) {
                float intervalStartX = 0.0f;
                float intervalEndX = 0.0f;
                float intervalStartY = 0.0f;
                float intervalEndY = 0.0f;
                if (orientation == PlotOrientation.VERTICAL) {
                    intervalStartX = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                    intervalEndX = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                    intervalStartY = (float) rangeAxis.valueToJava2D(lowY, dataArea, yAxisLocation);
                    intervalEndY = (float) rangeAxis.valueToJava2D(highY, dataArea, yAxisLocation);
                } else {
                    intervalStartX = (float) rangeAxis.valueToJava2D(lowY, dataArea, yAxisLocation);
                    intervalEndX = (float) rangeAxis.valueToJava2D(highY, dataArea, yAxisLocation);
                    intervalStartY = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                    intervalEndY = (float) domainAxis.valueToJava2D(lastX, dataArea, xAxisLocation);
                }
                intervalCount++;
                s.intervalPath.moveTo(intervalStartX, intervalStartY);
                s.intervalPath.lineTo(intervalEndX, intervalEndY);
            }
        }
        PathIterator pi = s.seriesPath.getPathIterator(null);
        g2.setStroke(getItemStroke(series, 0));
        g2.setPaint(getItemPaint(series, 0));
        g2.draw(s.seriesPath);
        g2.draw(s.intervalPath);
        //System.out.println("Interval count " + intervalCount);
        //System.out.println("Bad points " + badPoints);
    }

    /**
     * Returns a clone of the renderer.
     *
     * @return A clone.
     *
     * @throws CloneNotSupportedException if the clone cannot be created.
     */
	@Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

    /**
     * Tests this renderer for equality with an arbitrary object.
     *
     * @param obj  the object (<code>null</code> permitted).
     *
     * @return <code>true</code> or <code>false</code>.
     */
	@Override
    public boolean equals(Object obj) {
        if (obj == this) {
            return true;
        }
        if (!(obj instanceof SamplingXYLineAndShapeRenderer)) {
            return false;
        }
        SamplingXYLineAndShapeRenderer that = (SamplingXYLineAndShapeRenderer) obj;
        if (this.lineWidth != that.lineWidth) {
            return false;
        }
        if (this.shapeSize != that.shapeSize) {
            return false;
        }
        return super.equals(obj);
    }

    /**
     * Provides serialization support.
     *
     * @param stream  the input stream.
     *
     * @throws IOException  if there is an I/O error.
     * @throws ClassNotFoundException  if there is a classpath problem.
     */
    private void readObject(ObjectInputStream stream)
            throws IOException, ClassNotFoundException {
        stream.defaultReadObject();
        this.lineWidth = stream.readInt();
        this.shapeSize = stream.readInt();
    }

    /**
     * Provides serialization support.
     *
     * @param stream  the output stream.
     *
     * @throws IOException  if there is an I/O error.
     */
    private void writeObject(ObjectOutputStream stream) throws IOException {
        stream.defaultWriteObject();
        stream.writeInt(this.lineWidth);
        stream.writeInt(this.shapeSize);
    }
}
