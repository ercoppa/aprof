package aprofplot.gui;

import javax.swing.table.*;
import java.util.*;
import aprofplot.*;
import javax.swing.SwingUtilities;

public class RoutinesTableModel extends AbstractTableModel {
	
	private AprofReport report;
	private ArrayList<Routine> elements = new ArrayList<Routine>();
	private ArrayList<String> columnNames;
	private ArrayList<Class> columnTypes;
	private MainWindow main = null;
    private RoutinesTableModel contexts = null;
    private boolean is_table_routine = true;
	
	public RoutinesTableModel(AprofReport report, MainWindow main,
                                RoutinesTableModel contexts) {
		setData(report);
		this.main = main; 
        this.contexts = contexts;
    }
    
    public RoutinesTableModel(AprofReport report, MainWindow main,
                                boolean is_table_routine) {
        this(report, main, null);
		this.is_table_routine = is_table_routine;
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
        if (main == null || main.isInputMetricRms())
            columnNames.add("#RMS");
        else
            columnNames.add("#RVMS");
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
        
        // ratio RMS/RVMS
        if (main != null && !main.isInputMetricRms()) {
            columnNames.add("Ext. input");
            columnTypes.add(Double.class);
        }   
        
		if (hasContext) {
            
            if (is_table_routine) {
                // Collapsed
                columnNames.add("Collapsed");
                columnTypes.add(Boolean.class);
            }
            
            // Context
            columnNames.add("#Context");
            
            if (is_table_routine)
                columnTypes.add(Long.class);
            else
                columnTypes.add(String.class);
            
        }
        
        // Favorites
        columnNames.add("Favorite");
        columnTypes.add(Boolean.class);
        
	}
    
    public final void setElements(ArrayList<RoutineContext> list) {
        
        this.elements = new ArrayList<Routine>();
        if (list != null) {
            this.elements.addAll(list);
        }
        updateColumns(true);
        fireTableStructureChanged();
		fireTableDataChanged();
        
    }
	
	public final void setData(AprofReport report) {
		
		this.report = report;
        if (!is_table_routine) return;
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
		if (is_table_routine && report.hasContexts() 
                   && columnIndex == columnNames.size() - 3) {
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
                        return rtn_info.getTotalCumulativeCost();
                    else
                        return rtn_info.getTotalSelfCost();
                
			case 3: // # Rms
					return rtn_info.getSizeRmsList();
			
			case 4: // % total cost rtn wrt all rtns
                    if (main.isDisplayCumulativeTotalCost())
                        return Math.ceil((rtn_info.getTotalCumulativeCost() / 
                                report.getTotalCost()) * 100  * 100) / 100;
                    else
                        return Math.ceil((rtn_info.getTotalSelfCost() / 
                                report.getTotalCost()) * 100  * 100) / 100;
			
			case 5: // Cost Plot: we already set the renderer for Routine class
					return rtn_info;
				
			case 6: // # calls
					return rtn_info.getTotalCalls();
				
			case 7: // % of rtn calls wrt all rtns
					return Math.ceil(((double)rtn_info.getTotalCalls() / 
                            (double)report.getTotalCalls()) * 100 * 100) / 100;
            case 8:
                    // # RMS
                    if (!main.isInputMetricRms())
                        return (1 - rtn_info.getRatioRmsRvms());
			
		}
		
		//System.out.println("Index requested: " + columnIndex + " over " + columnNames.length);
        
		if (report.hasContexts()) {
			
			if (is_table_routine && columnIndex == columnNames.size() - 3) { // Collapsed?
				
				if (rtn_info instanceof ContextualizedRoutineInfo) {
                    return ((ContextualizedRoutineInfo)rtn_info).getCollapsed(); 
                } else return null;
				
			}
			
			if (columnIndex == columnNames.size() - 2) { // # context
			
				 if (rtn_info instanceof ContextualizedRoutineInfo)
					return ((ContextualizedRoutineInfo)rtn_info).getContextCount();
				else if (rtn_info instanceof RoutineContext)
					return ((RoutineContext)rtn_info).getContextId() + "/" + 
							((RoutineContext)rtn_info).getOverallRoutine().getContextCount();
				else return "0";
				
			}
			
		} 
		
		// Fav
		if (columnIndex == columnNames.size() - 1) {
			
			String fav = "" + rtn_info.getID();
			if (rtn_info instanceof RoutineContext)
				fav += ("_" + ((RoutineContext)rtn_info).getContextId());
			return report.isFavorite(fav);

		}
		
		//throw new RuntimeException("Invalid Column index");
		return null;
		
	}

    public void loadContexts(Routine rtn, boolean null_hide) {
        
        if (main.isVisibleContextsTable())
            main.saveSortingContextsTable();
        
        if (rtn == null) {
            if (!main.isVisibleContextsTable()) return;
            if (null_hide) main.setVisibleContextsTable(false);
            //contexts.setElements(null);
            return;
        }
        
        ContextualizedRoutineInfo rtn_info = (ContextualizedRoutineInfo) rtn;
        if (rtn_info.getCollapsed()) {
            main.setVisibleContextsTable(false);
            return;
        }
        
        contexts.setElements(rtn_info.getContexts());
        main.restoreSortingContextsTable();
        main.setVisibleContextsTable(true);
        
    }
    
	public void collapseRoutine(Routine rtn) {
        
		if (rtn instanceof ContextualizedRoutineInfo) {
            ContextualizedRoutineInfo rtn_info = (ContextualizedRoutineInfo)rtn;
            rtn_info.setCollapsed(true);
            loadContexts(null, true);
		}  

	}

	public void expandRoutine(Routine rtn) {
		
        if (!(rtn instanceof ContextualizedRoutineInfo)) return;
        ContextualizedRoutineInfo rtn_info = (ContextualizedRoutineInfo)rtn;
		rtn_info.setCollapsed(false);
        loadContexts(rtn, true);

	}

	@Override
	public void setValueAt(final Object aValue, final int rowIndex, final int columnIndex) {
		
		if (is_table_routine && report.hasContexts() 
                && columnIndex == columnNames.size() - 3) {
			
			//main.inhibit_selection(true);
			final RoutinesTableModel me = this;
            
			SwingUtilities.invokeLater(new Runnable() {
			
				@Override
				public void run() { 

					Routine rtn = elements.get(rowIndex);
					if (!((Boolean)aValue).booleanValue())
						expandRoutine(rtn);
					else
						collapseRoutine(rtn);
					
                    //System.out.println("Set cell " + rowIndex + " " + columnIndex);
                    me.fireTableCellUpdated(rowIndex, columnIndex);
					//main.inhibit_selection(false);
					
				}
		
			});
			
		} else if (columnIndex == columnNames.size() - 1) {
			
			Routine rtn_info = elements.get(rowIndex);
			String fav = ""+rtn_info.getID();
			if (rtn_info instanceof RoutineContext)
				fav += ("_" + ((RoutineContext)rtn_info).getContextId());
			if (!report.isFavorite(fav)) report.addToFavorites(fav);
			else report.removeFromFavorites(fav);
			
            main.enableSaveCommand();
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
