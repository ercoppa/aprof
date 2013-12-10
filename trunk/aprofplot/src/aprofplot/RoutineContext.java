package aprofplot;

import java.util.*;

public class RoutineContext extends Routine {
	
	private int context_id;
	private RoutineContext parent;
	private ContextualizedRoutineInfo overall; 

	public RoutineContext() {
		super(0);
	}

	public RoutineContext(RoutineContext parent, ContextualizedRoutineInfo overall) {
		super(0);					
		this.parent = parent;
		this.overall = overall;
	}

	public void addRmsToContext(Rms r) {
		addRms(r);
		if (overall != null) overall.addRmsLazy(r);
	}

	public void setParent(RoutineContext p) {
		parent = p;
	}
	
	public void setOverallRoutine(ContextualizedRoutineInfo o) {
		overall = o;
	}

	public void setContextId(int id) {
		context_id = id;
	}

	public int getContextId() {
		return context_id;
	}

	@Override
	public int getID() {
		return overall.getID();
	}

	@Override
	public String getImage() {
		return overall.getImage();
	}

	@Override
	public String getName() {
		return overall.getName();
	}
	
	@Override
	public String getFullName() {
		return overall.getFullName();
	}
	
	@Override
	public String getMangledName() {
		return overall.getMangledName();
	}

	public RoutineContext getParent() {
		return parent;
	}

	public ContextualizedRoutineInfo getOverallRoutine() {
		return overall;
	}

	@Override
	public boolean equals(Object o) {
		if (o != null && getClass().equals(o.getClass())) {
			RoutineContext rtn_info = (RoutineContext)o;
			return (rtn_info.getID() == this.getID() &&
					rtn_info.getContextId() == this.getContextId());
		}
		else return false;
	}

	@Override
	public int hashCode() {
		int hash = 7;
		hash = 67 * hash + this.context_id;
		return hash;
	}

	@Override
	public String toString() {
		return this.getName() + " (" + this.getContextId() +  "/" 
				+ this.getOverallRoutine().getContextCount() + ")";
	}

	 public ArrayList<RoutineContext> getStackTrace() {
        ArrayList<RoutineContext> res = new ArrayList<RoutineContext>();
        RoutineContext current = this;
        while (current != null) {
            res.add(current);
            current = current.getParent();
        }
        return res;
    }
     
}
