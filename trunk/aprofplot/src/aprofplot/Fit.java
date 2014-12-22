package aprofplot;

/**
 *
 * @author ercoppa
 */
public class Fit {

    // param[0] + param[1] * (x^param[2])
    private double[] param = {0, 0, 0};

    // fitting quality
    private double r2;

    public Fit(double a, double b, double c, double r2) {
        param[0] = a;
        param[1] = b;
        param[2] = c;
        this.r2 = r2;
    }

    public double[] getParams() {
        return param.clone();
    }

    public double getFitQuality() {
        return r2;
    }
}
