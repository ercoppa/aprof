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
public class ContextualizedRoutineInfo extends RoutineInfo {
    
    private int context_id;
    private ContextualizedRoutineInfo parent;
    private UncontextualizedRoutineInfo overall;

    public ContextualizedRoutineInfo(String name, String address, String image,
                                        /*int context_id,*/
                                        ContextualizedRoutineInfo parent,
                                        UncontextualizedRoutineInfo overall) {
        super(name, address, image);
        //this.context_id = context_id;
        this.parent = parent;
        this.overall = overall;
    }

    public void setContextId(int id) {
        context_id = id;
    }

    public int getContextId() {
        return context_id;
    }

    public ContextualizedRoutineInfo getParent() {
        return parent;
    }

    public UncontextualizedRoutineInfo getOverallRoutineInfo() {
        return overall;
    }

    public ArrayList<ContextualizedRoutineInfo> getStackTrace() {
        ArrayList<ContextualizedRoutineInfo> res = new ArrayList<ContextualizedRoutineInfo>();
        ContextualizedRoutineInfo current = this;
        while (current != null) {
            res.add(current);
            current = current.getParent();
        }
        return res;
    }

    @Override
    public boolean equals(Object o) {
        if (o != null && getClass().equals(o.getClass())) {
            ContextualizedRoutineInfo rtn_info = (ContextualizedRoutineInfo)o;
            return (rtn_info.getAddress().equals(this.getAddress()) && (rtn_info.getContextId() == this.getContextId()));
        }
        else return false;
    }

    public String toString() {
        return this.getName() + " (" + this.getContextId() +  "/" + this.getOverallRoutineInfo().getContextCount() + ")";
    }
}
