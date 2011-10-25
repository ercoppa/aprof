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
public class RoutinesTableModel extends AbstractTableModel {
    
    private AprofReport report;
    private ArrayList<RoutineInfo> elements;
    private String[] columnNames = new String [] {"Routine",
                                                  "Lib",
                                                  //"Addr",
                                                  "Cost",
                                                  "#Rms",
                                                  "Cost %",
                                                  "Avg ratio (μs)",
                                                  //"Null",
                                                  "Cost plot",
                                                  "Calls",
                                                  "Calls %",
                                                  "Collapsed",
                                                  "#Contexts",
                                                  "Favourite"
    };
    private Class[] columnTypes = new Class[] {String.class,
                                               String.class,
                                               //String.class,
                                               Double.class,
                                               Integer.class,
                                               Double.class,
                                               Double.class,
                                               //Integer.class,
                                               RoutineInfo.class,
                                               Integer.class,
                                               Double.class,
                                               Boolean.class,
                                               String.class,
                                               Boolean.class
    };

    public RoutinesTableModel(AprofReport report) {
        this.report = report;
        if (report != null) elements = report.getRoutines();
    }

    public void setData(AprofReport report) {
        this.report = report;
        if (report != null) elements = report.getRoutines();
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

    @Override
    public boolean isCellEditable(int rowIndex, int columnIndex) {
        if (columnIndex == 11) return true;
        if (columnIndex == 9) {
            RoutineInfo rtn = elements.get(rowIndex);
            return (rtn instanceof ContextualizedRoutineInfo ||
                    (rtn instanceof UncontextualizedRoutineInfo &&
                    ((UncontextualizedRoutineInfo)rtn).getContextCount() > 0));
        }
        return false;
    }

    public Object getValueAt(int rowIndex, int columnIndex) {
        if (elements == null) return null;
        RoutineInfo rtn_info = elements.get(rowIndex);
        switch (columnIndex) {
            case 0: return rtn_info.getName();
            case 1: return rtn_info.getImage();
            //case 2: return rtn_info.getAddress();
            case 2: return new Double(rtn_info.getTotalTime() / 1000000);
            case 3: return new Integer(rtn_info.getTimeEntries().size());
            case 4: return new Double((rtn_info.getTotalTime() / report.getTotalTime()) * 100);
            case 5: return new Double(rtn_info.getMeanRatio());
            //case 6: return 0;
            case 6: return rtn_info;  // time plot
            case 7: return new Integer(rtn_info.getTotalCalls());
            case 8: return new Double(((double)rtn_info.getTotalCalls() / (double)report.getTotalCalls()) * 100);
            case 9: if (rtn_info instanceof UncontextualizedRoutineInfo && ((UncontextualizedRoutineInfo)rtn_info).getContextCount() > 0)
                        return new Boolean(((UncontextualizedRoutineInfo)rtn_info).getCollapsed());
                    else return null;
            case 10:if (rtn_info instanceof UncontextualizedRoutineInfo)
                        return "" + ((UncontextualizedRoutineInfo)rtn_info).getContextCount();
                    else return ((ContextualizedRoutineInfo)rtn_info).getContextId() + "/" + ((ContextualizedRoutineInfo)rtn_info).getOverallRoutineInfo().getContextCount();
            case 11:String fav = "" + rtn_info.getID();
                    if (rtn_info instanceof ContextualizedRoutineInfo)
                        fav += ("_" + ((ContextualizedRoutineInfo)rtn_info).getContextId());
                    return new Boolean(report.isFavorite(fav));

            default: return null;
        }
    }

    public void collapseRoutine(RoutineInfo rtn) {
        if (rtn instanceof UncontextualizedRoutineInfo) {
            elements.removeAll(((UncontextualizedRoutineInfo)rtn).getContexts());
            ((UncontextualizedRoutineInfo)rtn).setCollapsed(true);
        }
        else {
            elements.removeAll(((ContextualizedRoutineInfo)rtn).getOverallRoutineInfo().getContexts());
            ((ContextualizedRoutineInfo)rtn).getOverallRoutineInfo().setCollapsed(true);
        }
        this.fireTableDataChanged();
    }

    public void expandRoutine(RoutineInfo rtn) {
        if (rtn instanceof ContextualizedRoutineInfo) return;
        UncontextualizedRoutineInfo rtn_info = (UncontextualizedRoutineInfo)rtn;
        elements.addAll(elements.indexOf(rtn_info) + 1, rtn_info.getContexts());
        rtn_info.setCollapsed(false);
        this.fireTableDataChanged();
    }

    public void setValueAt(Object aValue, int rowIndex, int columnIndex) {
        if (columnIndex == 9) {
            RoutineInfo rtn = elements.get(rowIndex);
            if (!((Boolean)aValue).booleanValue()) {
                expandRoutine(rtn);
            }
            else {
                collapseRoutine(rtn);
            }
        }
        else if (columnIndex == 11) {
            RoutineInfo rtn_info = elements.get(rowIndex);
            String fav = ""+rtn_info.getID();
            if (rtn_info instanceof ContextualizedRoutineInfo)
                fav += ("_" + ((ContextualizedRoutineInfo)rtn_info).getContextId());
            if (!report.isFavorite(fav)) report.addToFavorites(fav);
            else report.removeFromFavorites(fav);
            this.fireTableCellUpdated(rowIndex, columnIndex);
        }
    }

    public RoutineInfo getRoutineInfo(int index) {
        if (elements == null) return null;
        return elements.get(index);
    }

    public int getIndex(RoutineInfo r) {
        return this.elements.indexOf(r);
    }

    public void refresh() {
        fireTableDataChanged();
    }
}
