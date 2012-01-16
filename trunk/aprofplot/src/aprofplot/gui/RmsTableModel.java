package aprofplot.gui;

import javax.swing.table.*;
import java.util.*;
import aprofplot.*;

public class RmsTableModel extends AbstractTableModel {

	private Routine rtn = null;
	private String[] columnNames = new String [] {"rms",
												  "min cost",
												  "avg cost",
												  "max cost",
												  "freq",
												  //"var",
												  //"mcc"
	};
	private Class[] columnTypes = new Class[] {Integer.class,
											   Double.class,
											   Double.class,
											   Double.class,
											   Long.class//,
											   //Double.class,
											   //Double.class
	};

	public RmsTableModel(Routine r) {
		this.rtn = r;
	}

	public void setData(Routine r) {
		this.rtn = r;
		fireTableDataChanged();
	}

	@Override
	public Class getColumnClass(int columnIndex) {
 		return columnTypes[columnIndex];
 	}

	@Override
	public int getColumnCount() {
 		return columnTypes.length;
 	}

	@Override
	public String getColumnName(int columnIndex) {
 		return columnNames[columnIndex];
 	}

	@Override
	public int getRowCount() {
		if (rtn == null) return 0;
		else return rtn.getSizeRmsList();
 	}

	@Override
	public boolean isCellEditable(int rowIndex, int columnIndex) {
 		return false;
 	}
	
	@Override
	public Object getValueAt(int rowIndex, int columnIndex) {
		
		if (rtn == null) return null;
		
		Rms te = rtn.getRmsItem(rowIndex);
		switch (columnIndex) {
			case 0: return te.getRms();
			case 1: return te.getMinCost();
			case 2: return te.getAvgCost();
			case 3: return te.getMaxCost();
			case 4: return te.getOcc();
			//case 5: return te.getVar();
			//case 6: return rtn.getMcc(te.getRms());
			default: return null;
		}
	}

	@Override
	public void setValueAt(Object aValue, int rowIndex, int columnIndex) {
		// not available
		return;
	}

	public void refresh() {
		fireTableDataChanged();
	}
}
