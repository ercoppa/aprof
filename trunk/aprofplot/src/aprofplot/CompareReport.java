/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package aprofplot;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author ercoppa
 */
public class CompareReport {
    
    private AprofReport reportA = null;
    private AprofReport reportB = null;

    CompareReport(String repA, String repB) {
        
        File rA = new File(repA);
        if (!rA.canRead()) throw new RuntimeException("Invalid first report");
        File rB = new File(repB);
        if (!rB.canRead()) throw new RuntimeException("Invalid second report");
        
        try {
        
            this.reportA = new AprofReport(rA);
            this.reportB = new AprofReport(rB);
        
        } catch (Exception ex) {
            
            this.reportA = null;
            this.reportB = null;
            System.out.println(ex.getMessage());
            throw new RuntimeException("Problem during report's parsing");
        
        }
        
    }

    private boolean printDiffData(double a, double b, double tollerance_perc,
                                    double static_tollerance) {
        
        double diff = b - a;
        double diff_p = diff / (a / 100);
        if ((diff_p > -tollerance_perc && diff_p < tollerance_perc)
                || (diff > -static_tollerance && diff < static_tollerance)) {
            return true;
            //System.out.format("[%+.2f%%] ", diff);
        } else if (diff < 0) {
            System.out.format("\u001B[42m[%+.2f%%]\u001B[40m ", diff_p);
        } else  {
            System.out.format("\u001B[41m[%+.2f%%]\u001B[40m ", diff_p);
        }
        
        return false;
        
    }
    
    public void printDiff() {
        
        ArrayList<Routine> missing_A = new ArrayList<Routine>();
        ArrayList<Routine> missing_B = new ArrayList<Routine>();
        
        boolean skip = false;
        
        System.out.println();
        System.out.println("Report A: " + reportA.getName());
        System.out.println("Report B: " + reportB.getName());
        System.out.println();
        
        long diff = (long)reportB.getTotalCost() - (long)reportA.getTotalCost();
        
        printDiffData(reportA.getTotalCost(), reportB.getTotalCost(), 0, 0);
        System.out.println( "Total cumulative cost: "
                            + (long)reportA.getTotalCost() + " : " 
                            + (long)reportB.getTotalCost() + " = "
                            + diff
                           );
        
        diff = (long)reportB.getTotalSelfCost() - (long)reportA.getTotalSelfCost();
        
        printDiffData(reportA.getTotalSelfCost(), reportB.getTotalSelfCost(), 0, 0);
        System.out.println( "Total self cost: "
                            + (long)reportA.getTotalSelfCost() + " : " 
                            + (long)reportB.getTotalSelfCost() + " = "
                            + diff
                           );
        
        if (diff < 0) diff = -diff;
        
        printDiffData(reportA.getRoutineCount(), reportB.getRoutineCount(), 0, 0);
        System.out.println( "Number of routines: "
                            + reportA.getRoutineCount() + " : " 
                            + reportB.getRoutineCount() + " = "
                            + (reportB.getRoutineCount() - reportA.getRoutineCount())
                            + "\n");
        
        HashMap<String, Routine> hA = reportA.getHashMapRoutines();
        HashMap<String, Routine> hB = reportB.getHashMapRoutines();
        
        Set<String> rA = hA.keySet();
        Iterator iA = rA.iterator();
        while(iA.hasNext()) {
        
            String name = (String) iA.next();
            
            if (!name.equals("strcmp") && !name.equals("__GI_strcmp")) 
                continue;
            
            Routine r1 = hA.get(name);
            Routine r2 = hB.get(name);
            
            if (r1 == null) throw new RuntimeException("Invalid routine");
            
            if (r2 == null) {
                missing_B.add(r1);
                continue;
            }
            
            skip = printDiffData(r1.getTotalCost(), r2.getTotalCost(), 0, 0);
            if (!skip) System.out.println(r1.getName() + " "
                                + (long) r1.getTotalCost() + " : " 
                                + (long) r2.getTotalCost() + " = "
                                + (long) (r2.getTotalCost() - r1.getTotalCost())
                                );
        
        }
        
        Set<String> rB = hB.keySet();
        
        
        
    }
    
}
