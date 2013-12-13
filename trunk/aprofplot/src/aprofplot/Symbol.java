package aprofplot;

public class Symbol {
    
    public enum Type {
        FUNCTION, OTHER, NONE
    }
    
	private Type type = Type.NONE;
	private String name = null;
	private int line = -1;
	private String file = null;
    
	public Symbol(String name, int line, String file, Type type) {
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
	
	public Type getType() {
		return type;
	}
	
}
