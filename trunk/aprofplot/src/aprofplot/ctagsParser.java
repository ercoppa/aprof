package aprofplot;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.StringTokenizer;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author ercoppa
 */
public class ctagsParser {
	
	private File source = null;
	private HashSet<String> files = null;
	
	public ctagsParser(File source) {
		
		this.source = source;
	
	}
	
	public HashMap<String, Symbol> getSymbols() {
	
		if (source == null) return null;
		
		HashMap<String, Symbol> ls = null;
		
		try {
			
			Process p = Runtime.getRuntime().exec(Main.getCtagsPath() +
								" -R -x --extra=+q " + source.getPath());
			BufferedReader input = new BufferedReader(new InputStreamReader(p.getInputStream()));
			ls = new HashMap<String, Symbol>();
			files = new HashSet<String>();
			
			String line=null;
 
			while((line = input.readLine()) != null) {
				
				StringTokenizer tokenizer = new StringTokenizer(line);
				String name = tokenizer.nextToken();
				
				if(name.contains("::operator")) {
					
					name = name + tokenizer.nextToken();
					
				}
				
				String type = tokenizer.nextToken();
				String lns = tokenizer.nextToken();
				
				if (type.equals("function") || type.equals("macro")) {
					
					int ln = Integer.parseInt(lns);
					String file = tokenizer.nextToken();
					
					if(!files.contains(file))
						files.add(file);
					
					ls.put(name, new Symbol(name, ln, file, Symbol.FUNCTION));
					//System.out.println("Function " + name + " at line " + line_n + " of " + file);
				}
				//System.out.println(line);
			}
			
			int exitVal = -1;
			try {
				exitVal = p.waitFor();
			} catch (InterruptedException ex) {
				//
			}
			
            //System.out.println("Exited with error code " + exitVal);
			return ls;
			
		} catch (IOException ex) {
			//System.out.println("Fail to read ctags output [1]");
		}
		
		return null;
	
	}
	
	public ArrayList<String> getFiles() {
		
		if (files == null) getSymbols();
		return new ArrayList<String>(files);
		
	}
	
}