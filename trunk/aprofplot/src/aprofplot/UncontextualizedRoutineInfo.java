/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aprofplot;

import java.util.*;

/**
 *
 * @author bruno
 */
public class UncontextualizedRoutineInfo extends RoutineInfo {

    private boolean collapsed;
    private ArrayList<ContextualizedRoutineInfo> contexts;

    public UncontextualizedRoutineInfo(int id, String name, String image) {
        super(id, name, image);
        this.collapsed = true;
    }

    public int getContextCount() {
        return (contexts == null)? 0 : contexts.size();
    }

    public void addContext(ContextualizedRoutineInfo ctxt) {
        if (contexts == null) contexts = new ArrayList<ContextualizedRoutineInfo>();
        contexts.add(ctxt);
        ctxt.setContextId(contexts.size());
    }

    public ArrayList<ContextualizedRoutineInfo> getContexts() {
        //return (ArrayList<ContextualizedRoutineInfo>)contexts.clone();
        return new ArrayList<ContextualizedRoutineInfo>(this.contexts);
    }

    public boolean getCollapsed() {
        return this.collapsed;
    }

    public void setCollapsed(boolean coll) {
        this.collapsed = coll;
    }

    public String toString() {
        return this.getName();
    }
}
