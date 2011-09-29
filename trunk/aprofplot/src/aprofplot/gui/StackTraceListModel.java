/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aprofplot.gui;

import javax.swing.*;
import java.util.*;
import aprofplot.*;

/**
 *
 * @author bruno
 */
public class StackTraceListModel extends AbstractListModel{

    private ArrayList<ContextualizedRoutineInfo> elements;

    public StackTraceListModel() {
    }

    public void setData(ArrayList<ContextualizedRoutineInfo> elements) {
        this.elements = elements;
        if (elements != null) this.fireIntervalAdded(this, 0, elements.size() - 1);
        else  this.fireIntervalAdded(this, 0, 0);
    }

    public Object getElementAt(int index) {
        return (elements == null)? null : elements.get(index).getName();
    }

    public int getSize() {
        return (elements == null)? 0 : elements.size();
    }

    public ContextualizedRoutineInfo getContextualizedoutineInfo(int index) {
        return (elements == null)? null : elements.get(index);
    }

}
