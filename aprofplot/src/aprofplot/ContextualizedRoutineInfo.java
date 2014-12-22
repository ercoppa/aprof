package aprofplot;

import java.util.*;

public class ContextualizedRoutineInfo extends RoutineInfo {

    // Status flag (useful for GUI, maybe should not be placed here...)
    private boolean collapsed = true;

    // All routine contexts
    private ArrayList<RoutineContext> contexts = new ArrayList<RoutineContext>();

    // Lazy add list (used for perfomance reason)
    private ArrayList<Input> lazy_list = new ArrayList<Input>();

    public ContextualizedRoutineInfo(int id, String name, String image) {
        super(id, name, image);
    }

    public ContextualizedRoutineInfo(int id) {
        super(id, null, null);
    }

    public int getContextCount() {
        return contexts.size();
    }

    public void addContext(RoutineContext c) {
        contexts.add(c);
        c.setContextId(contexts.size());
        lazy_list.addAll(c.getInputTuples());
    }

    public void addInputTupleLazy(Input r) {
        lazy_list.add(r);
    }

    public void mergeInputTupleLazyList() throws CloneNotSupportedException {

        if (lazy_list.isEmpty()) {
            return;
        }

        // Sort the list
        Collections.sort(lazy_list);

        // Merge duplicates
        Iterator lazy = lazy_list.iterator();

        sortInputTuplesByInput();
        Iterator list = getInputTuplesIterator();

        Input a = null;
        Input b = null;
        Input prev = null;

        ArrayList<Input> new_tuples = new ArrayList<Input>();

        // merge lazy and list
        while (true) {

            if (lazy.hasNext() && a == null) {
                a = (Input) lazy.next();
            }

            // we have finished :)
            if (a == null) {
                break;
            }

            if (b != null && b.getSize() < a.getSize()) {
                b = null;
            }

            while (list.hasNext() && b == null) {

                b = (Input) list.next();
                if (b.getSize() < a.getSize()) {
                    b = null;
                }
            }

            if (b == null) {

                if (prev != null && prev.getSize() == a.getSize()) {
                    prev.merge(a);
                } else {

                    prev = a.clone();
                    new_tuples.add(prev);
                }

                a = null;

            } else if (a.getSize() == b.getSize()) {
                b.merge(a);
                a = null;
            }
        }

        Iterator i = new_tuples.iterator();
        while (i.hasNext()) {
            addInput((Input) i.next());
        }

        // free lazy list
        lazy_list.clear();
    }

    public ArrayList<RoutineContext> getContexts() {
        return contexts;
    }

    public boolean getCollapsed() {
        return collapsed;
    }

    public void setCollapsed(boolean coll) {
        this.collapsed = coll;
    }

}
