package aprofplot;

import java.util.Iterator;

public class RmsIterator implements Iterator {

	private Iterator it = null;
	
	public RmsIterator(Iterator<Rms> i) {
		this.it = i;
	}
	
	@Override
	public boolean hasNext() {
		return it.hasNext();
	}

	@Override
	public Rms next() {
		return (Rms)it.next();
	}

	@Override
	public void remove() {
		throw new RuntimeException("Disabled operation");
	}
	
}
