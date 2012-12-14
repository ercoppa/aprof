package aprofplot.gui;

import javax.swing.table.*;
import java.util.*;
import aprofplot.*;
import javax.swing.SwingUtilities;

public class RoutinesTableModel extends AbstractTableModel {
	
	private AprofReport report;
	private ArrayList<Routine> elements;
	private ArrayList<String> columnNames;
	private ArrayList<Class> columnTypes;
	private MainWindow main = null;
	
	public RoutinesTableModel(AprofReport report, MainWindow main) {
		setData(report);
		this.main = main; 
	}

	private void updateColumns(boolean hasContext) {
		
        columnNames = new ArrayList<String>();
        columnTypes = new ArrayList<Class>();
        
        // Name
        columnNames.add("Routine");
        columnTypes.add(String.class);
        
        // Lib name
        columnNames.add("Lib");
        columnTypes.add(String.class);
        
        // Cost
        if (Main.getDisplayTotalCost() == Main.COST_CUMULATIVE)
            columnNames.add("Cost (cumul.)");
        else
            columnNames.add("Cost (self)");
        
        columnTypes.add(Double.class);
        
        // # RMS
        columnNames.add("#RMS");
        columnTypes.add(Integer.class);
        
        // Cost %
        if (Main.getDisplayTotalCost() == Main.COST_CUMULATIVE)
            columnNames.add("Cost % (cumul.)");
        else
            columnNames.add("Cost % (self)");
        
        columnTypes.add(Double.class);
        
        // Cost plot
        if (Main.getChartCost() == Main.COST_CUMULATIVE)
            columnNames.add("Cost plot (cumul.)");
        else
            columnNames.add("Cost plot (self)");
        
        columnTypes.add(Routine.class);
        
        // Calls
        columnNames.add("Calls");
        columnTypes.add(Integer.class);
        
        // Calls %
        columnNames.add("Calls %");
        columnTypes.add(Double.class);
        
		if (hasContext) {
            
            // Collapsed
            columnNames.add("Collapsed");
            columnTypes.add(Boolean.class);
            
            // Context
            columnNames.add("#Context");
            columnTypes.add(String.class);
            
        }
        
        // Favorites
        columnNames.add("Favorites");
        columnTypes.add(Boolean.class);
        
	}
	
	public final void setData(AprofReport report) {
		
		this.report = report;
		if (report != null) {
		
			elements = report.getRoutines();
			updateColumns(report.hasContexts());
			//System.out.println("Report has context? " + report.hasContexts());
		
		} else
			updateColumns(false);
        
		fireTableStructureChanged();
		fireTableDataChanged();
	}

	@Override
	public Class getColumnClass(int columnIndex) {
		if (columnIndex >= columnTypes.size()) return null;
		return columnTypes.get(columnIndex);
	}

	@Override
	public int getColumnCount() {
		return columnTypes.size();
	}

	@Override
	public String getColumnName(int columnIndex) {
		if (columnIndex >= columnTypes.size()) return null;
		return columnNames.get(columnIndex);
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
		if (columnIndex == columnNames.size() - 1) return true;
		
		// Collapse context
		if (report.hasContexts() && columnIndex == columnNames.size() - 3) {
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
                    if (main.isDisplayCumulativeTotalCost())
                        return new Double(rtn_info.getTotalCumulativeCost());
                    else
                        return new Double(rtn_info.getTotalSelfCost());
                
			case 3: // # Rms
					return new Integer(rtn_info.getSizeRmsList());
			
			case 4: // % total cost rtn wrt all rtns
                    if (main.isDisplayCumulativeTotalCost())
                        return new Double((rtn_info.getTotalCumulativeCost() / 
                                report.getTotalCost()) * 100);
                    else
                        return new Double((rtn_info.getTotalSelfCost() / 
                                report.getTotalCost()) * 100);
			
			case 5: // Cost Plot: we already set the renderer for Routine class
					return rtn_info;
				
			case 6: // # calls
					return new Long(rtn_info.getTotalCalls());
				
			case 7: // % of rtn calls wrt all rtns
					return new Double(((double)rtn_info.getTotalCalls() / (double)report.getTotalCalls()) * 100);
			
		}
		
		//System.out.println("Index requested: " + columnIndex + " over " + columnNames.length);
		
		if (report.hasContexts()) {
			
			if (columnIndex == columnNames.size() - 3) { // Collapsed?
				
				if (rtn_info instanceof ContextualizedRoutineInfo)
					return new Boolean(((ContextualizedRoutineInfo)rtn_info).getCollapsed());
				else return null;
				
			}
			
			if (columnIndex == columnNames.size() - 2) { // # context
			
				 if (rtn_info instanceof ContextualizedRoutineInfo)
					return "" + ((ContextualizedRoutineInfo)rtn_info).getContextCount();
				else if (rtn_info instanceof RoutineContext)
					return ((RoutineContext)rtn_info).getContextId() + "/" + 
							((RoutineContext)rtn_info).getOverallRoutine().getContextCount();
				else return "0";
				
			}
			
		} 
		
		// Fav
		if (columnIndex == columnNames.size() -1) {
			
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
	public void setValueAt(final Object aValue, final int rowIndex, int columnIndex) {
		
		if (report.hasContexts() && columnIndex == columnNames.size() - 3) {
			
			main.inhibit_selection(true);
			
			SwingUtilities.invokeLater(new Runnable() {
			
				@Override
				public void run() { 

					Routine rtn = elements.get(rowIndex);
					if (!((Boolean)aValue).booleanValue())
						expandRoutine(rtn);
					else
						collapseRoutine(rtn);
					
					main.inhibit_selection(false);
					
				}
		
			});
			
		} else if (columnIndex == columnNames.size() - 1) {
			
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

}
