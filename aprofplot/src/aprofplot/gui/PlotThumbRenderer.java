package aprofplot.gui;

import javax.swing.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.image.*;
import aprofplot.*;
import java.util.*;

public class PlotThumbRenderer extends JLabel implements TableCellRenderer {

	private int thumbWidth = 80;
	private int thumbHeight = 50;
	private int x_axis_length;
	private int y_axis_length;
	private double min_x, min_y, max_x, max_y;

	public PlotThumbRenderer() {
		setOpaque(true);
	}

	@Override
	public Component getTableCellRendererComponent(
							JTable table, Object rtn_info,
							boolean isSelected, boolean hasFocus,
							int row, int column) {

		Color bgColor;
		if (isSelected) bgColor = table.getSelectionBackground();
		else bgColor = Color.WHITE;
		this.setBackground(bgColor);

		BufferedImage image = new BufferedImage(thumbWidth, table.getRowHeight() - 2/*thumbHeight*/, BufferedImage.TYPE_INT_RGB);
		Graphics2D g2d = image.createGraphics();
		g2d.setColor(bgColor);
		g2d.fillRect(0, 0, thumbWidth, thumbHeight);
		g2d.setColor(Color.BLACK);
		g2d.drawRect(0, 0, thumbWidth - 1, thumbHeight - 1);

		int H_MARGIN = 3;
		int V_MARGIN = 3;

		x_axis_length = image.getWidth() - 2 * H_MARGIN;
		y_axis_length = image.getHeight() - 2 * V_MARGIN;
		g2d.translate(H_MARGIN, image.getHeight() - V_MARGIN);

		Routine r = (Routine)rtn_info;
		if (r != null) {
			Iterator i = r.getInputTuplesIterator();
			if (i.hasNext()) {

				/* maximize graph */
				min_x = min_y = 0;
				max_y = Math.ceil(r.getMaxCost());
				if (max_y == 0) max_y++;
				max_x = r.getMaxInput();
				if (max_x == 0) max_x++;

				/* draw points */
				g2d.setColor(java.awt.Color.BLUE);
				while(i.hasNext()) {
					Rms s = (Rms) i.next();
					double x = s.getRms();
					double y = s.getCost();
					int x_trans = XCoordTransform(x);
					int y_trans = -YCoordTransform(y);
					g2d.drawLine(x_trans, y_trans, x_trans, y_trans);
				}
			}
		}

		this.setIcon(new ImageIcon(image));
		this.setHorizontalAlignment(JLabel.CENTER);
		return this;
	}

	private int XCoordTransform(double x) {
		if (max_x == 0) return 0;
		return (int) Math.round(((x - min_x) * x_axis_length) / (max_x - min_x));
	}

	private int YCoordTransform(double y) {
		if (max_y == 0) return 0;
		return (int) (((y - min_y) * y_axis_length) / (max_y - min_y));
	}
}
