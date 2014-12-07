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
	
    public enum COLUMN {
        NAME, LIB, COST, N_INPUT, P_COST, COST_PLOT, CALL, P_CALL, P_THREAD,
        P_SYSCALL, CONTEXT, CONTEXT_COLLAPSED, FIT_A, FIT_B, FIT_C, FIT_R2,
        FAVORITE, LAST
    }
    
    private COLUMN[] column_conf = defaultColumnConfig();
    private Vector<COLUMN> column_index = null;
    
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

    private static COLUMN[] defaultColumnConfig() {
        return new COLUMN[]{
                                COLUMN.NAME,
                                COLUMN.LIB,
                                COLUMN.COST,
                                COLUMN.N_INPUT,
                                COLUMN.P_COST,
                                COLUMN.COST_PLOT,
                                COLUMN.CALL,
                                COLUMN.P_CALL,
                                COLUMN.P_SYSCALL,
                                COLUMN.P_THREAD,
                                COLUMN.FIT_A,
                                COLUMN.FIT_B,
                                COLUMN.FIT_C,
                                COLUMN.FIT_R2,
                                COLUMN.CONTEXT,
                                COLUMN.CONTEXT_COLLAPSED,
                                COLUMN.FAVORITE
        };
    }
    
    public void setColumnConfig(COLUMN[] config) {
        column_conf = config;
        updateColumns();
        refresh();
    }
    
    public COLUMN[] getColumnConfig() {
        return column_conf.clone();
    }
    
    public int getIndexOfColumn(COLUMN c) {
        for (int i = 0; i < column_index.size(); i++)
            if (c == column_index.get(i))
                return i;
        return -1;
    }
    
    private void updateColumnIndexes() {
        
        column_index = new Vector<COLUMN>();
        for (COLUMN c : column_conf) {
            
            if (c == COLUMN.P_SYSCALL
                || c == COLUMN.P_THREAD)
                if (report == null || !report.hasInputStats())
                    continue;
            
            if (c == COLUMN.FIT_A 
                || c == COLUMN.FIT_B 
                || c == COLUMN.FIT_C
                || c == COLUMN.FIT_R2)
                if (report == null || !report.hasFitter() 
                        || main == null || !main.isVisibleFittingData() )
                    continue;
            
            if (c == COLUMN.CONTEXT
                || c == COLUMN.CONTEXT_COLLAPSED)
                if (report == null || !report.hasContexts())
                    continue;
            
            column_index.add(c);
        } 
    }
    
	public void updateColumns() {
		
        columnNames = new ArrayList<String>();
        columnTypes = new ArrayList<Class>();
        updateColumnIndexes();
        
        for (COLUMN c : column_index) {
            
            switch (c) {
            
                case NAME:
                    columnNames.add("Routine");
                    columnTypes.add(String.class);
                    break;
                    
                case LIB:
                    columnNames.add("Binary");
                    columnTypes.add(String.class);
                    break;
                    
                case COST:
                    if (Main.getRtnCostMode() == Input.CostType.CUMULATIVE)
                        columnNames.add("Cost (cumul.)");
                    else
                        columnNames.add("Cost (self)");
                    columnTypes.add(Double.class);
                    break;
                    
                case N_INPUT:
                    if (main == null || main.isInputMetricRms())
                        columnNames.add("#RMS");
                    else
                        columnNames.add("#DRMS");
                    columnTypes.add(Integer.class);
                    break;
                    
                case P_COST:
                    if (Main.getRtnCostMode() == Input.CostType.CUMULATIVE)
                        columnNames.add("Cost % (cumul.)");
                    else
                        columnNames.add("Cost % (self)");
                    columnTypes.add(Double.class);
                    break;
                
                case COST_PLOT:    
                    columnNames.add("Cost plot");
                    columnTypes.add(Routine.class);
                    break;
                    
                case CALL:
                    columnNames.add("Calls");
                    columnTypes.add(Integer.class);
                    break;
                    
                case P_CALL:
                    columnNames.add("Calls %");
                    columnTypes.add(Double.class);
                    break;
                
                case P_SYSCALL:
                    columnNames.add("%Syscall");
                    columnTypes.add(Double.class);
                    break;
                    
                case P_THREAD:
                    columnNames.add("%Thread");
                    columnTypes.add(Double.class);
                    break;
                    
                case FIT_A:
                    columnNames.add("a");
                    columnTypes.add(Double.class);
                    break;
                    
                case FIT_B:
                    columnNames.add("b");
                    columnTypes.add(Double.class);
                    break;
                
                case FIT_C:
                    columnNames.add("c");
                    columnTypes.add(Double.class);
                    break;
                
                case FIT_R2:
                    columnNames.add("r^2");
                    columnTypes.add(Double.class);
                    break;

                case CONTEXT:
                    columnNames.add("#Context");
                    if (is_table_routine)
                        columnTypes.add(Long.class);
                    else
                        columnTypes.add(String.class);
                    break;
                
                case CONTEXT_COLLAPSED:
                    if (is_table_routine) {
                        columnNames.add("Collapsed");
                        columnTypes.add(Boolean.class);
                    }
                    break;
                
                case FAVORITE:
                    columnNames.add("Favorite");
                    columnTypes.add(Boolean.class);
                    break;
                    
                default:
                    throw new RuntimeException("Invalid column: " + c);
            }
        }
	}
    
    public final void setElements(ArrayList<RoutineContext> list) {
        
        this.elements = new ArrayList<Routine>();
        if (list != null) {
            this.elements.addAll(list);
        }
        updateColumns();
        fireTableStructureChanged();
		fireTableDataChanged();
    }
	
	public final void setData(AprofReport report) {
		
		this.report = report;
        if (!is_table_routine) return;
		if (report != null) {
		
			elements = report.getRoutines();
			updateColumns();
			//System.out.println("Report has context? " + report.hasContexts());
		
		} else
			updateColumns();
        
		refresh();
	}
    
    public void refresh() {
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
		
		if (report == null) 
            return false;
        
        if (column_index.get(columnIndex) == COLUMN.FAVORITE)
            return true;

		// Collapse context
		if (is_table_routine && report.hasContexts() 
                   && column_index.get(columnIndex) == COLUMN.CONTEXT_COLLAPSED) {
			
            Routine rtn = elements.get(rowIndex);
            
            if (rtn instanceof ContextualizedRoutineInfo) {
                ContextualizedRoutineInfo c = (ContextualizedRoutineInfo) rtn;
                return c.getContextCount() >= 1;
            }
                
			return rtn instanceof RoutineContext;
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
		
        COLUMN c = this.column_index.get(columnIndex);
		Fit f = null;
        switch (c) {

            case NAME:
                return rtn_info.getName();

            case LIB:
                return rtn_info.getImage();

            case COST:
                if (main.isDisplayCumulativeTotalCost())
                    return rtn_info.getTotalCumulativeCost();
                else
                    return rtn_info.getTotalSelfCost();

            case N_INPUT:
                return rtn_info.getInputTuplesCount();

            case P_COST:
                if (main.isDisplayCumulativeTotalCost())
                    return Math.ceil((rtn_info.getTotalCumulativeCost() / 
                            report.getTotalCost()) * 100  * 100) / 100;
                else
                    return Math.ceil((rtn_info.getTotalSelfCost() / 
                            report.getTotalCost()) * 100  * 100) / 100;
                
            case COST_PLOT:    
                return rtn_info;

            case CALL:
                return rtn_info.getTotalCalls();

            case P_CALL:
                return Math.ceil(((double)rtn_info.getTotalCalls() / 
                        (double)report.getTotalCalls()) * 100 * 100) / 100;

            case P_SYSCALL:
                return Math.round(rtn_info.getRatioSyscallInput() * 100);

            case P_THREAD:
                return Math.round(rtn_info.getRatioThreadInput() * 100);

            case FIT_A:
                f = this.report.getFit(rtn_info.getID());
                if (f == null) return 0.0;
                return f.getParams()[0];

            case FIT_B:
                f = this.report.getFit(rtn_info.getID());
                if (f == null) return 0.0;
                return f.getParams()[1];

            case FIT_C:
                f = this.report.getFit(rtn_info.getID());
                if (f == null) return 0.0;
                return f.getParams()[2];
               
            case FIT_R2:
                f = this.report.getFit(rtn_info.getID());
                if (f == null) return 0.0;
                return f.getFitQuality();

            case CONTEXT:
                if (rtn_info instanceof ContextualizedRoutineInfo)
					return ((ContextualizedRoutineInfo)rtn_info).getContextCount();
				else if (rtn_info instanceof RoutineContext)
					return ((RoutineContext)rtn_info).getContextId() + "/" + 
							((RoutineContext)rtn_info).getOverallRoutine().getContextCount();
				else return "0";

            case CONTEXT_COLLAPSED:
                if (rtn_info instanceof ContextualizedRoutineInfo) {
                    
                    ContextualizedRoutineInfo co = (ContextualizedRoutineInfo) rtn_info;
                    return co.getCollapsed(); 
                
                } else return null;
                
            case FAVORITE:
                String fav = "" + rtn_info.getID();
                if (rtn_info instanceof RoutineContext)
                    fav += ("_" + ((RoutineContext)rtn_info).getContextId());
                return report.isFavorite(fav);

            default:
                throw new RuntimeException("Invalid column: " + c);
        }		
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
		
        COLUMN c = column_index.get(columnIndex);
		if (is_table_routine && report.hasContexts() 
                && c == COLUMN.CONTEXT_COLLAPSED) {
			
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
			
		} else if (c == COLUMN.FAVORITE) {
			
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
