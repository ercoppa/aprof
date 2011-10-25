/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aprofplot.gui;

import javax.swing.table.*;
import java.util.*;
import aprofplot.*;

/**
 *
 * @author bruno
 */
public class SmsTableModel extends AbstractTableModel {

    private ArrayList<SmsEntry> elements;
    private String[] columnNames = new String [] {"Rms",
                                                  "Cost",
                                                  "Ratio",
                                                  "Occ"
    };
    private Class[] columnTypes = new Class[] {Integer.class,
                                               Double.class,
                                               Double.class,
                                               Integer.class
    };

    public SmsTableModel(RoutineInfo r) {
        if (r != null) this.elements = r.getTimeEntries();
        else elements = null;
    }

    public void setData(RoutineInfo r) {
        if (r != null) this.elements = r.getTimeEntries();
        else elements = null;
        fireTableDataChanged();
    }

    public Class getColumnClass(int columnIndex) {
 		return columnTypes[columnIndex];
 	}

    public int getColumnCount() {
 		return columnTypes.length;
 	}

    public String getColumnName(int columnIndex) {
 		return columnNames[columnIndex];
 	}

    public int getRowCount() {
        if (elements == null) return 0;
 		return elements.size();
 	}

    public boolean isCellEditable(int rowIndex, int columnIndex) {
 		return false;
 	}

    public Object getValueAt(int rowIndex, int columnIndex) {
        if (elements == null) return null;
        SmsEntry te = elements.get(rowIndex);
        switch (columnIndex) {
            case 0: return new Integer(te.getSms());
            case 1: return new Double(te.getCost()/* / 1000000*/);
            case 2: return new Double(te.getRatio());
            case 3: return new Long(te.getOcc());
            default: return null;
        }
    }

    public void setValueAt(Object aValue, int rowIndex, int columnIndex) {
            // not available
            return;
    }

    public void refresh() {
        fireTableDataChanged();
    }
}
