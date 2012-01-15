package aprofplot.gui;

import java.util.*;

public class MergeListModel extends javax.swing.AbstractListModel {

	private ArrayList<java.io.File> elements;

	public MergeListModel() {
		elements = new ArrayList<java.io.File>();
	}

	public void add(java.io.File[] f) {
		int elements_added = 0;
		for (int i = 0; i < f.length; i++) {
			if (elements.indexOf(f[i]) < 0) {
				elements.add(f[i]);
				elements_added++;
			}
		}
		fireIntervalAdded(this, elements.size() - elements_added, elements.size() - 1);
	}

	public void remove(int index) {
		if (index >= 0) {
			elements.remove(index);
			fireIntervalRemoved(this, index, index);
		}
	}

	@Override
	public Object getElementAt(int index) {
		if (elements == null) return null;
		return (elements.get(index)).getName();
	}

	@Override
	public int getSize() {
		if (elements == null) return 0;
		return elements.size();
	}

	public java.io.File[] getElements() {
		return elements.toArray(new java.io.File[0]);
	}
}
