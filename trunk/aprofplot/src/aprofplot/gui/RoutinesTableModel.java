package aprofplot.gui;

import javax.swing.table.*;
import java.util.*;
import aprofplot.*;

public class RoutinesTableModel extends AbstractTableModel {
	
	private AprofReport report;
	private ArrayList<Routine> elements;
	private String[] columnNames;
	private Class[] columnTypes;
	
	public RoutinesTableModel(AprofReport report) {
		setData(report);
	}

	private void updateColumns(boolean hasContext) {
		
		if (hasContext) {
			columnNames = new String [] {
											"Routine",
											"Lib",
											"Cost",
											"#Rms",
											"Cost %",
											"Cost plot",
											"Calls",
											"Calls %",
											"Collapsed",
											"#Contexts",
											"Favorite"
										};

			columnTypes = new Class[] {
										String.class,
										String.class,
										Double.class,
										Integer.class,
										Double.class,
										Routine.class,
										Integer.class,
										Double.class,
										Boolean.class,
										String.class,
										Boolean.class
									};
		} else {
			
			columnNames = new String [] {
											"Routine",
											"Lib",
											"Cost",
											"#Rms",
											"Cost %",
											"Cost plot",
											"Calls",
											"Calls %",
											"Favorite"
										};

			columnTypes = new Class[] {
										String.class,
										String.class,
										Double.class,
										Integer.class,
										Double.class,
										Routine.class,
										Integer.class,
										Double.class,
										Boolean.class
									};
			
		}
	}
	
	public final void setData(AprofReport report) {
		
		this.report = report;
		if (report != null) {
		
			elements = report.getRoutines();
			updateColumns(report.hasContexts());
		
		} else
			updateColumns(false);
		
		fireTableStructureChanged();
		fireTableDataChanged();
	}

	@Override
	public Class getColumnClass(int columnIndex) {
		if (columnIndex >= columnTypes.length) return null;
		return columnTypes[columnIndex];
	}

	@Override
	public int getColumnCount() {
		return columnTypes.length;
	}

	@Override
	public String getColumnName(int columnIndex) {
		if (columnIndex >= columnTypes.length) return null;
		return columnNames[columnIndex];
	}

	@Override
	public int getRowCount() {
		if (elements == null) return 0;
		return elements.size();
	}

	@Override
	public boolean isCellEditable(int rowIndex, int columnIndex) {
		
		if (report == null) return false;
		
		// Fovourite
		if (columnIndex == columnNames.length - 1) return true;
		
		// Collapse context
		if (report.hasContexts() && columnIndex == columnNames.length - 3) {
			Routine rtn = elements.get(rowIndex);
			return (rtn instanceof ContextualizedRoutineInfo) || 
						(rtn instanceof RoutineContext);
		}

		return false;
	}

	@Override
	public Object getValueAt(int rowIndex, int columnIndex) {
		
		if (elements == null) return null;
		
		Routine rtn_info = null;
		try {
			rtn_info = elements.get(rowIndex);
		} catch(IndexOutOfBoundsException e) {
			return null;
		}
		
		switch (columnIndex) {
			
			case 0: // Routine name
					return rtn_info.getName();
			
			case 1: // Routine lib
					return rtn_info.getImage();
				
			case 2: // Routine Total cost
					return new Double(rtn_info.getTotalCost());
				
			case 3: // # Rms
					return new Integer(rtn_info.getSizeRmsList());
			
			case 4: // % total cost rtn wrt all rtns
					return new Double((rtn_info.getTotalCost() / report.getTotalCost()) * 100);
			
			case 5: // Cost Plot: we already set the renderer for Routine class
					return rtn_info;
				
			case 6: // # calls
					return new Long(rtn_info.getTotalCalls());
				
			case 7: // % of rtn calls wrt all rtns
					return new Double(((double)rtn_info.getTotalCalls() / (double)report.getTotalCalls()) * 100);
			
		}
		
		if (report.hasContexts()) {
			
			if (columnIndex == columnNames.length - 3) { // Collapsed?
				
				if (rtn_info instanceof ContextualizedRoutineInfo)
					return ((ContextualizedRoutineInfo)rtn_info).getCollapsed();
				else return null;
				
			}
			
			if (columnIndex == columnNames.length - 2) { // # context
			
				 if (rtn_info instanceof ContextualizedRoutineInfo)
					return "" + ((ContextualizedRoutineInfo)rtn_info).getContextCount();
				else if (rtn_info instanceof RoutineContext)
					return ((RoutineContext)rtn_info).getContextId() + "/" + 
							((RoutineContext)rtn_info).getOverallRoutine().getContextCount();
				else return "0";
				
			}
			
		} 
		
		// Fav
		if (columnIndex == columnNames.length -1) {
			
			String fav = "" + rtn_info.getID();
			if (rtn_info instanceof RoutineContext)
				fav += ("_" + ((RoutineContext)rtn_info).getContextId());
			return report.isFavorite(fav);
		
		}
		
		//throw new RuntimeException("Invalid Column index");
		return null;
		
	}

	public void collapseRoutine(Routine rtn) {
		if (rtn instanceof ContextualizedRoutineInfo) {
			elements.removeAll(((ContextualizedRoutineInfo)rtn).getContexts());
			((ContextualizedRoutineInfo)rtn).setCollapsed(true);
		} else {
			elements.removeAll(((RoutineContext)rtn).getOverallRoutine().getContexts());
			((RoutineContext)rtn).getOverallRoutine().setCollapsed(true);
		}
		this.fireTableDataChanged();
	}

	public void expandRoutine(Routine rtn) {
		if (!(rtn instanceof ContextualizedRoutineInfo)) return;
		ContextualizedRoutineInfo rtn_info = (ContextualizedRoutineInfo)rtn;
		elements.addAll(elements.indexOf(rtn_info) + 1, rtn_info.getContexts());
		rtn_info.setCollapsed(false);
		this.fireTableDataChanged();
	}

	@Override
	public void setValueAt(Object aValue, int rowIndex, int columnIndex) {
		
		if (report.hasContexts() && columnIndex == columnNames.length - 3) {
			
			Routine rtn = elements.get(rowIndex);
			if (!((Boolean)aValue).booleanValue())
				expandRoutine(rtn);
			else
				collapseRoutine(rtn);
			
		} else if (columnIndex == columnNames.length - 1) {
			
			Routine rtn_info = elements.get(rowIndex);
			String fav = ""+rtn_info.getID();
			if (rtn_info instanceof RoutineContext)
				fav += ("_" + ((RoutineContext)rtn_info).getContextId());
			if (!report.isFavorite(fav)) report.addToFavorites(fav);
			else report.removeFromFavorites(fav);
			
			this.fireTableCellUpdated(rowIndex, columnIndex);
		
		}
	}

	public Routine getRoutine(int index) {
		if (elements == null) return null;
		return elements.get(index);
	}

	public int getIndex(Routine r) {
		return this.elements.indexOf(r);
	}

	public void refresh() {
		fireTableDataChanged();
	}
}
