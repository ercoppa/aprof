package aprofplot;

import java.util.Iterator;

public class RmsIterator implements Iterator {

	private Iterator it = null;
	
	public RmsIterator(Iterator<Input> i) {
		this.it = i;
	}
	
	@Override
	public boolean hasNext() {
		return it.hasNext();
	}

	@Override
	public Input next() {
		return (Input)it.next();
	}

	@Override
	public void remove() {
		throw new RuntimeException("Disabled operation");
	}
	
}
