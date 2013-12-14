package aprofplot;

import aprofplot.gui.*;
import java.awt.*;
import java.util.*;
import java.io.*;
import java.util.regex.MatchResult;

public class Main {

    public enum RtnNameMode {
        DEMANGLED, DEMANGLED_FULL
    }
    
    private static RtnNameMode rtn_name_mode = RtnNameMode.DEMANGLED;
    private static Input.CostType rtn_cost_mode = Input.CostType.CUMULATIVE;
    private static Input.CostType chart_cost_mode = Input.CostType.CUMULATIVE;
    
	private static ArrayList<MainWindow> windows = new ArrayList<MainWindow>();
	
    private static ArrayList<String> blacklist = new ArrayList<String>();
	private static boolean blacklist_enabled = false;
    
    private static ArrayList<File> recent_files = new ArrayList<File>();
    private static final int RECENT_SIZE = 6;
    
	private static ArrayList<Integer> graph_order = null;
	
	private static String lastReportPath = null;
	private static String lastSourceDir = null;
    
	private static boolean editor = false;
	private static String ctags = "ctags-exuberant";
	
	public synchronized static void newWindow() {
		EventQueue.invokeLater(new Runnable() {
			@Override
			public void run() {
				MainWindow window = new MainWindow();
                window.postStart();
				windows.add(window);
				window.setVisible(true);
			}
		});
	}
    
    public synchronized static Input.CostType getRtnCostMode() {
        return rtn_cost_mode;
    }
    
    public synchronized static void setRtnCostMode(Input.CostType cost) {
        rtn_cost_mode = cost;
    }
    
    public synchronized static Input.CostType getChartCostMode() {
        return chart_cost_mode;
    }
    
    public synchronized static void setChartCostMode(Input.CostType cost) {
        chart_cost_mode = cost;
    }
	
	public synchronized static void removeWindow(MainWindow window) {
		windows.remove(window);
		if (windows.isEmpty()) System.exit(0);
	}

	public synchronized static String getLastReportPath() {
		return lastReportPath;
	}
	
	public synchronized static boolean getEditorVisible() {
		return editor;
	}
	
	public synchronized static void setEditorVisible(boolean editor_visible) {
		editor = editor_visible;
		saveSettings();
	}
	
	public synchronized static String getCtagsPath() {
		return ctags;
	}
	
	public synchronized static void setCtagsPath(String ctags_path) {
		ctags = ctags_path;
		saveSettings();
	}
	
	public synchronized static String getLastSourceDir() {
		return lastSourceDir;
	}

	public synchronized static void setLastReportPath(String path) {
		lastReportPath = path;
		saveSettings();
	}
	
	public synchronized static void setLastSourceDir(String path) {
		lastSourceDir = path;
		saveSettings();
	}

	public synchronized static ArrayList<String> getBlackList() {
		return blacklist;
	}

	public synchronized static boolean getBlackListEnabled() {
		return blacklist_enabled;
	}

	public synchronized static void setBlacklist(ArrayList<String> v, boolean enabled) {
		blacklist = v;
		blacklist_enabled = enabled;
		saveSettings();
	}

	public synchronized static RtnNameMode getRtnNameMode() {
		return rtn_name_mode;
	}

	public synchronized static void setRtnNameMode(RtnNameMode mode) {
		rtn_name_mode = mode;
		saveSettings();
	}

	public synchronized static void addRecentFile(File f) {
		int index = recent_files.indexOf(f);
		if (index < 0) {
			recent_files.add(0, f);
			if (recent_files.size() > RECENT_SIZE) 
				recent_files.remove(recent_files.size() - 1);
		} else {
			recent_files.remove(f);
			recent_files.add(0, f);
		}
		saveSettings();
	}

	public synchronized static void removeRecentFile(File f) {
		recent_files.remove(f);
		saveSettings();
	}
	
	public synchronized static ArrayList<File> getRecentFiles() {
		return recent_files;
	}

	public synchronized static ArrayList<Integer> getGraphOrder() {
		return graph_order;
	}
	
	public synchronized static void setGraphOrder(ArrayList<Integer> go) {
		graph_order = go;
		saveSettings();
	}

	private synchronized static void saveSettings() {
		try {
			
			PrintWriter out = new PrintWriter(new FileWriter("settings.dat"));
			out.println(lastReportPath);
			
			out.print("blacklist ");
			if (blacklist_enabled) out.print("1 ");
			else out.print("0 ");
			for (int i = 0; i < blacklist.size(); i++) 
				out.print(blacklist.get(i) + " ");
			out.println("");
			

			if (graph_order == null) resetGraphOrder();
			out.print("graph_order ");
			for (int i = 0; i < graph_order.size(); i++)
				out.print(graph_order.get(i) + " ");
			out.println("");

			if (!lastSourceDir.equals(""))
				out.println("src " + lastSourceDir);
			
			if (!ctags.equals(""))
				out.println("ctags " + ctags);
			
			if (editor)
				out.println("editor");
				
			out.print("rtn_display ");
			if (rtn_name_mode == RtnNameMode.DEMANGLED) out.println("0");
			else out.println("1");
            
            out.print("cost_display ");
			if (rtn_cost_mode == Input.CostType.CUMULATIVE) out.println("0");
			else out.println("1");
            
            out.print("chart_cost ");
			if (chart_cost_mode == Input.CostType.CUMULATIVE) out.println("0");
			else out.println("1");
            
			for (int i = 0; i < recent_files.size(); i++) 
				out.println("recent " + recent_files.get(i));
			out.close();
			
		} catch (Exception e) {
			e.printStackTrace();
			System.out.println("Problem during write of settings.dat");
		}
	}

	private synchronized static void loadSettings() {

		try {
			
			BufferedReader in = new BufferedReader(new FileReader("settings.dat"));
			
			String line = null;
			while ((line = in.readLine()) != null) {
			
				StringTokenizer tokenizer = new StringTokenizer(line);
				if (!tokenizer.hasMoreTokens()) continue;
				String tag = tokenizer.nextToken();
				
				if (tag.equals("blacklist")) {
					
					if (tokenizer.hasMoreTokens() && tokenizer.nextToken().equals("1")) 
						blacklist_enabled = true;
					else
						blacklist_enabled = false;
					
					while (tokenizer.hasMoreTokens()) 
						blacklist.add(tokenizer.nextToken());
					
				} else if (tag.equals("rtn_display")) {
					
					if (tokenizer.hasMoreTokens()) {
                    
                        int mode = Integer.parseInt(tokenizer.nextToken());
                        if (mode == 0) 
                            Main.setRtnNameMode(RtnNameMode.DEMANGLED);
                        else if (mode == 1)
                            Main.setRtnNameMode(RtnNameMode.DEMANGLED_FULL);
                    
                    }
						
					
				} else if (tag.equals("cost_display")) {
					
					if (tokenizer.hasMoreTokens()) {
                    
                        int mode = Integer.parseInt(tokenizer.nextToken());
                        if (mode == 0) 
                            Main.setRtnCostMode(Input.CostType.CUMULATIVE);
                        else if (mode == 1)
                            Main.setRtnCostMode(Input.CostType.SELF);
                    }
						
					
				} else if (tag.equals("chart_cost")) {
					
					if (tokenizer.hasMoreTokens()) {
                    
                        int cost = Integer.parseInt(tokenizer.nextToken());
                        if (cost == 0) 
                            Main.setChartCostMode(Input.CostType.CUMULATIVE);
                        else if (cost == 1)
                            Main.setChartCostMode(Input.CostType.SELF);
                    }
					
				} else if (tag.equals("graph_order")) {
					
					graph_order = new ArrayList<Integer>();
					
					while (tokenizer.hasMoreTokens()) 
						graph_order.add(Integer.parseInt(tokenizer.nextToken()));
					
				} else if (tag.equals("recent")) {
					
					Scanner s = new Scanner(line);
					s.findInLine("recent (.+)");
					MatchResult result = s.match();
					try {
						recent_files.add(new File(result.group(1)));
					} catch (Exception e) {
						System.out.println("Recent file not valid. Ignored.");
					}
					
				} else if (tag.equals("src")) { // last source dir
			
					while (tokenizer.hasMoreTokens())
						lastSourceDir = lastSourceDir + " " +tokenizer.nextToken();
				
				} else if(tag.equals("editor")) { // editor visible
				
					editor = true;
				
				} else if (tag.equals("ctags")) { 
					
					if (tokenizer.hasMoreTokens())
						ctags = tokenizer.nextToken();
			
				} else { // last path
					
					lastReportPath = line;
					
				}
			
			}
			
			// load default settings for graph_order
			if (graph_order == null) { 
				
				resetGraphOrder();
				saveSettings();
				
			}

			in.close();
		
		} catch (FileNotFoundException e) {
			
			resetGraphOrder();
			saveSettings();
			
		} catch (Exception e) {
			
			javax.swing.JOptionPane.showMessageDialog(null, "Settings file is corrupt;\nusing default settings");
			resetGraphOrder();
			saveSettings();
		
		}
	}

	private synchronized static void resetGraphOrder() {
	
		graph_order = new ArrayList<Integer>();
		graph_order.add(GraphPanel.COST_PLOT); // cost plot
		graph_order.add(GraphPanel.FREQ_PLOT); // frequency plot
		graph_order.add(GraphPanel.RATIO_PLOT); // ratio plot
	
	}
    
	/**
	 * @param args the command line arguments
	 */
	public static void main(String[] args) {
        
		// Mac OS X integration
		System.setProperty("apple.laf.useScreenMenuBar", "true");
		System.setProperty("com.apple.mrj.application.apple.menu.about.name", "aprof-plot");
		
		loadSettings();
		newWindow();
	}

}
