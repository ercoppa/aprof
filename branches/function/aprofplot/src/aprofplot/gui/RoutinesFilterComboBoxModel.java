/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aprofplot.gui;

import java.util.*;
import javax.swing.*;
import javax.swing.event.*;

/**
 *
 * @author bruno
 */
public class RoutinesFilterComboBoxModel implements ComboBoxModel {

    private int selectedIndex;
    private ArrayList<String> elements;

    public RoutinesFilterComboBoxModel(ArrayList<String> v) {
        selectedIndex = -1;
        elements = v;
    }
    
    public void setSelectedItem(Object o) {
        selectedIndex = elements.indexOf(o);
    }

    public Object getSelectedItem() {
        if (selectedIndex < 0) return null;
        return elements.get(selectedIndex);
    }

    public Object getElementAt(int index) {
        if (elements == null || index >= elements.size()) return null;
        return elements.get(index);
    }

    public int getSize() {
        if (elements == null) return 0;
		return elements.size();
	}

    public void addListDataListener(ListDataListener l) {
		//not used
	}

	public void removeListDataListener(ListDataListener l) {
		//not used
	}
}
