package aprofplot.gui;

import javax.swing.*;
import java.util.*;
import aprofplot.*;

public class StackTraceListModel extends AbstractListModel {

    private ArrayList<RoutineContext> elements;

    public StackTraceListModel() {

    }

    public void setData(ArrayList<RoutineContext> elements) {
        this.elements = elements;
        if (elements != null) {
            this.fireIntervalAdded(this, 0, elements.size() - 1);
        } else {
            this.fireIntervalAdded(this, 0, 0);
        }
    }

    @Override
    public Object getElementAt(int index) {
        return (elements == null) ? null : elements.get(index).getName();
    }

    @Override
    public int getSize() {
        return (elements == null) ? 0 : elements.size();
    }

    public RoutineContext getContext(int index) {
        return (elements == null) ? null : elements.get(index);
    }

}
