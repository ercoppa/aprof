package aprofplot.gui;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;
import javax.swing.JPanel;

public class ColorLegend extends JPanel {

    private int pad_x = 0;
    private int pad_y = 12;
    private int box_width = 10;
    private int box_height = 15;
    private BufferedImage buffer = null;

    private static final Color[] colors = new Color[]{
        new Color(0, 85, 255),
        new Color(0, 170, 255),
        new Color(0, 213, 255),
        new Color(0, 255, 234),
        new Color(0, 255, 161),
        new Color(190, 235, 62),
        new Color(232, 253, 2),
        new Color(255, 242, 0),
        new Color(255, 225, 0),
        new Color(255, 191, 0),
        new Color(255, 145, 0),
        new Color(255, 64, 0)
    };

    @Override
    protected void paintComponent(Graphics g) {

        super.paintComponent(g);

        Graphics2D g2 = (Graphics2D) g;

        g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
            RenderingHints.VALUE_ANTIALIAS_ON);
        Font font = new Font(null, Font.BOLD, 10);
        g2.setFont(font);

        for (int i = 0; i < colors.length; i++) {

            int x = pad_x + (i * box_width);
            int y = pad_y;
            g.setColor(colors[i]);
            g.fillRect(x, y, box_width, box_height);

            g.setColor(Color.BLACK);
            g.drawLine(x, y, x, y + box_height);

        }

        g.setColor(Color.BLACK);
        g.drawLine(pad_x + box_width * colors.length, pad_y, pad_x + box_width * colors.length, pad_y + box_height);
        g.drawLine(pad_x, pad_y, pad_x + box_width * colors.length, pad_y);
        g.drawLine(pad_x, pad_y + box_height, pad_x + box_width * colors.length, pad_y + box_height);

        g2.drawString("1", pad_x + 1, pad_y - 3);
        g2.drawString("16", pad_x + 4 * box_width - 2, pad_y - 3);
        g2.drawString("256", pad_x + 8 * box_width - 6, pad_y - 3);
        g2.drawString(">4000", pad_x + 11 * box_width - 5, pad_y - 3);
    }

    @Override
    public int getWidth() {
        return pad_x + box_width * colors.length + 22;
    }

    @Override
    public int getHeight() {
        return box_height + pad_y + 2;
    }

    @Override
    public Dimension getPreferredSize() {
        return new Dimension(getWidth(), getHeight());
    }

    @Override
    public Dimension getMinimumSize() {
        return new Dimension(getWidth(), getHeight());
    }

    @Override
    public Dimension getMaximumSize() {
        return new Dimension(getWidth(), getHeight());
    }
}
