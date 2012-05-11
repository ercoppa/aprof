package aprofplot.gui;

import java.util.*;
import javax.swing.*;
import javax.swing.event.*;

public class ListFilesEditorModel implements ComboBoxModel {

	private int selectedIndex;
	private ArrayList<String> elements;

	public ListFilesEditorModel(ArrayList<String> v) {
		selectedIndex = -1;
		elements = v;
	}
	
	@Override
	public void setSelectedItem(Object o) {
		selectedIndex = elements.indexOf(o);
	}

	@Override
	public Object getSelectedItem() {
		if (selectedIndex < 0) return null;
		return elements.get(selectedIndex);
	}

	@Override
	public Object getElementAt(int index) {
		if (elements == null || index >= elements.size()) return null;
		return elements.get(index);
	}

	@Override
	public int getSize() {
		if (elements == null) return 0;
		return elements.size();
	}

	@Override
	public void addListDataListener(ListDataListener l) {
		//not used
	}

	@Override
	public void removeListDataListener(ListDataListener l) {
		//not used
	}
}

