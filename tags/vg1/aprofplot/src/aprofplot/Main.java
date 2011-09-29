/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aprofplot;

import aprofplot.gui.*;
import java.awt.*;
import java.util.*;
import java.io.*;

/**
 *
 * @author bruno
 */
public class Main {

    public static final int DEMANGLED = 0;
    public static final int DEMANGLED_FULL = 1;
    private static final int RECENT_SIZE = 6;

    private static ArrayList<MainWindow> windows = new ArrayList<MainWindow>();
    private static ArrayList<String> blacklist = new ArrayList<String>();
    private static ArrayList<File> recent_files = new ArrayList<File>();
    private static boolean blacklist_enabled = false;
    private static String lastReportPath = "";
    private static int rtn_display_mode = DEMANGLED;
    
    public synchronized static void newWindow() {
        EventQueue.invokeLater(new Runnable() {
            public void run() {
                MainWindow window = new MainWindow();
                windows.add(window);
                window.setVisible(true);
            }
        });
    }
    
    public synchronized static void removeWindow(MainWindow window) {
        windows.remove(window);
        if (windows.size() == 0) System.exit(0);
    }

    public synchronized static String getLastReportPath() {
        return lastReportPath;
    }

    public synchronized static void storeLastReportPath(String path) {
        lastReportPath = path;
        saveSettings();
    }

    public synchronized static ArrayList<String> getBlackList() {
        return blacklist;
    }

    public synchronized static boolean getBlackListEnabled() {
        return blacklist_enabled;
    }

    public synchronized static void storeBlacklist(ArrayList<String> v, boolean enabled) {
        blacklist = v;
        blacklist_enabled = enabled;
        saveSettings();
    }

    public synchronized static int getRtnDisplayMode() {
        return rtn_display_mode;
    }

    public synchronized static void storeRtnDisplayMode(int mode) {
        rtn_display_mode = mode;
        saveSettings();
    }

    public synchronized static void addRecentFile(File f) {
        int index = recent_files.indexOf(f);
        if (index < 0) {
            recent_files.add(0, f);
            if (recent_files.size() > RECENT_SIZE) recent_files.remove(recent_files.size() - 1);
        }
        else {
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

    private synchronized static void saveSettings() {
        try {
            PrintWriter out = new PrintWriter(new FileWriter("settings.dat"));
            out.println(lastReportPath);
            out.print("blacklist ");
            if (blacklist_enabled) out.print("1 ");
            else out.print("0 ");
            if (blacklist.size() > 0) {
                for (int i = 0; i < blacklist.size(); i++) out.print(blacklist.get(i) + " ");
            }
            out.println("");
            out.print("rtn_display ");
            if (rtn_display_mode == DEMANGLED) out.println("0");
            else out.println("1");
            out.print("recent ");
            for (int i = 0; i < recent_files.size(); i++) out.print(recent_files.get(i) + " ");
            out.println("");
            out.close();
        }
        catch (Exception e) {

        }
    }

    private synchronized static void loadSettings() {
        try {
            BufferedReader in = new BufferedReader(new FileReader("settings.dat"));
            lastReportPath = in.readLine();
            String blist = in.readLine();
            StringTokenizer tokenizer = new StringTokenizer(blist);
            tokenizer.nextToken(); // discard blacklist tag
            if (tokenizer.nextToken().equals("1")) blacklist_enabled = true;
            else blacklist_enabled = false;
            while (tokenizer.hasMoreTokens()) blacklist.add(tokenizer.nextToken());
            tokenizer = new StringTokenizer(in.readLine());
            tokenizer.nextToken();
            rtn_display_mode = Integer.parseInt(tokenizer.nextToken());
            tokenizer = new StringTokenizer(in.readLine());
            tokenizer.nextToken();
            while (tokenizer.hasMoreTokens()) recent_files.add(new File(tokenizer.nextToken()));
            in.close();
        }
        catch (FileNotFoundException e) {
            saveSettings();
        }
        catch (Exception e) {
            //lastReportPath = "";
            javax.swing.JOptionPane.showMessageDialog(null, "Settings file is corrupt;\nusing default settings");
            saveSettings();
        }
    }

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        // TODO code application logic here
        loadSettings();
        newWindow();
    }

}
