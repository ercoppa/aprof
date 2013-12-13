package aprofplot;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.StringTokenizer;

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
			
			String[] args = new String[]{Main.getCtagsPath(), "-R", "-x", "--extra=+q",
											source.getPath()};
			Process p = Runtime.getRuntime().exec(args);
			BufferedReader input = new BufferedReader(new InputStreamReader(p.getInputStream()));
			BufferedReader error = new BufferedReader(new InputStreamReader(p.getErrorStream()));
			ls = new HashMap<String, Symbol>();
			files = new HashSet<String>();
			
			String line=null;
			
			/*
			while((line = error.readLine()) != null) {
				System.out.println(line);
			}
			*/
			
			while((line = input.readLine()) != null) {
				
				//System.out.println(line);
				
				StringTokenizer tokenizer = new StringTokenizer(line);
				if (!tokenizer.hasMoreTokens()) continue;
				String name = tokenizer.nextToken();
				
				if(name.contains("::operator")) {
					
					if (!tokenizer.hasMoreTokens()) continue;
					name = name + tokenizer.nextToken();
					
				}
				
				if (!tokenizer.hasMoreTokens()) continue;
				String type = tokenizer.nextToken();
				if (!tokenizer.hasMoreTokens()) continue;
				String lns = tokenizer.nextToken();
				
				if (type.equals("function") || type.equals("macro")) {
					
					int ln = Integer.parseInt(lns);
					if (!tokenizer.hasMoreTokens()) continue;
					String file = tokenizer.nextToken();
					boolean file_name_valid = true;
					
					while (!file.toLowerCase().contains(".c") &&
						!file.toLowerCase().contains(".cpp") &&
						!file.toLowerCase().contains(".cc") &&
						!file.toLowerCase().contains(".h") &&
						!file.toLowerCase().contains(".cc") &&
						!file.toLowerCase().contains(".cxx") &&
						!file.toLowerCase().contains(".h++") &&
						!file.toLowerCase().contains(".h+") &&
						!file.toLowerCase().contains(".cp") &&
						!file.toLowerCase().contains(".hpp") &&
						!file.toLowerCase().contains(".hxx")) {
					
						if (!tokenizer.hasMoreTokens()) {
							file_name_valid = false;
							break;
						}
						file = file + " " + tokenizer.nextToken(); 
					
					}
					
					if (!file_name_valid) continue;
					
					//System.out.println(file);
					if(!files.contains(file))
						files.add(file);
					
					ls.put(name, new Symbol(name, ln, file, Symbol.Type.FUNCTION));
					//System.out.println("Function " + name + " at line " + line_n + " of " + file);
				}

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
			//System.out.println(ex);
			System.out.println("Fail to read ctags output [1]");
		}
		
		return null;
	
	}
	
	public ArrayList<String> getFiles() {
		
		if (files == null) getSymbols();
		return new ArrayList<String>(files);
		
	}
	
}