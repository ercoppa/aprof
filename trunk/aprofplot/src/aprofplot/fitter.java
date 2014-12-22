package aprofplot;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.StringTokenizer;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.filechooser.FileNameExtensionFilter;
import lma.LMA;
import lma.LMAMatrix;
import lma.implementations.FitModel;

/**
 *
 * @author ercoppa
 */
public class fitter {

    private HashMap<Long, Fit> fitting = new HashMap<Long, Fit>();
    private AprofReport report = null;

    public fitter(AprofReport report) throws FileNotFoundException, IOException {

        this.report = report;
        if (!loadFromFile()) {

            for (Routine r : report.getRoutines()) {

                LMA lma = new LMA(
                    new FitModel.Function(),
                    new double[]{1, 1, 1},
                    r.getRawData()
                );

                try {
                    lma.fit();
                    Fit f = new Fit(lma.parameters[0],
                        lma.parameters[1],
                        lma.parameters[2],
                        lma.computeR2());

                    fitting.put((long) r.getID(), f);
                    /*
                     System.out.println("Routine " + r.getName() 
                     + " has fit=" + lma.parametersToString()
                     + " R2=" + lma.computeR2());
                     */
                } catch (LMAMatrix.InvertException e) {
                    // fail to fit, skip it
                }

            }

            // export for future use
            saveToFile();
        }
    }

    private boolean loadFromFile() {

        try {

            File fitlog = new File(report.getName().replaceAll("\\.aprof", ".fitlog"));
            if (!fitlog.exists() || fitlog.isDirectory()) {
                return false;
            }

            String str;
            StringTokenizer tokenizer;
            BufferedReader in;
            try {
                in = new BufferedReader(new FileReader(fitlog));
            } catch (FileNotFoundException ex) {
                return false;
            }

            if (fitlog.length() == 0) {
                throw new RuntimeException("Zero length log: " + fitlog);
            }

            while ((str = in.readLine()) != null) {

                tokenizer = new StringTokenizer(str);

                // expected format:
                // <routine_id> <a> <b> <c> <r^2 quality>
                // fitted function: a + b * (x^c)
                // get routine id
                if (!tokenizer.hasMoreTokens()) {
                    continue;
                }
                long id = Long.parseLong(tokenizer.nextToken());

                // get a
                if (!tokenizer.hasMoreTokens()) {
                    continue;
                }
                double a = Double.parseDouble(tokenizer.nextToken());

                // get b
                if (!tokenizer.hasMoreTokens()) {
                    continue;
                }
                double b = Double.parseDouble(tokenizer.nextToken());

                // get c
                if (!tokenizer.hasMoreTokens()) {
                    continue;
                }
                double c = Double.parseDouble(tokenizer.nextToken());

                // get fitting quality
                if (!tokenizer.hasMoreTokens()) {
                    continue;
                }
                double r2 = Double.parseDouble(tokenizer.nextToken());

                Fit f = new Fit(a, b, c, r2);
                if (fitting.get(id) != null) {
                    throw new RuntimeException("Routine " + id
                        + " has more than one fit.");
                }
                fitting.put(id, f);
            }

        } catch (IOException ex) {
            Logger.getLogger(fitter.class.getName()).log(Level.SEVERE, null, ex);
            return false;
        }

        return true;
    }

    private void saveToFile() {

        try {

            File tmp = new File(report.getName().replaceAll("\\.aprof", ".fitlog"));
            tmp.createNewFile();
            PrintWriter out = new PrintWriter(new FileWriter(tmp));

            for (Long id : fitting.keySet()) {

                Fit fit = fitting.get(id);
                out.println(id + " " + fit.getParams()[0]
                    + " " + fit.getParams()[1]
                    + " " + fit.getParams()[2]
                    + " " + fit.getFitQuality()
                );
            }
            out.close();

        } catch (java.io.IOException e) {
            e.printStackTrace();
        }
    }

    public Fit getFit(long id) {
        return fitting.get(id);
    }

}
