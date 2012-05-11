package aprofplot;

/**
 *
 * @author ercoppa
 */
public class Symbol {
	
	private String name = null;
	private int line = -1;
	private String file = null;
	private int type = 0;
	// type of symbol
	public static final int NONE = 0;
	public static final int FUNCTION = 1;
	public static final int OTHER = 2;
	
	public Symbol(String name, int line, String file, int type) {
		this.type = type;
		this.name = name;
		this.line = line;
		this.file = file;
	}
	
	public String getName() {
		return name;
	}
	
	public int getLine() {
		return line;
	}
	
	public String getFile() {
		return file;
	}
	
	public int getType() {
		return type;
	}
	
}
