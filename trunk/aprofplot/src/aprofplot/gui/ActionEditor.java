/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package aprofplot.gui;

import java.awt.event.ActionEvent;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.text.BadLocationException;
import javax.swing.text.JTextComponent;
import javax.swing.text.TextAction;

/**
 *
 * @author ercoppa
 */
class ActionEditor extends TextAction {

    private MainWindow main_win = null;

    public ActionEditor() {
        super("Jump to this function/symbol");
    }

    public void setMainWindow(MainWindow w) {
        main_win = w;
    }

    @Override
    public void actionPerformed(ActionEvent e) {

        JTextComponent tc = getTextComponent(e);
        int selStart = tc.getSelectionStart();
        int selEnd = tc.getSelectionEnd();
        try {

            String sel = tc.getText(selStart, selEnd - selStart);
            //System.out.println(sel);
            main_win.loadFunctionInTextEditor(sel);

        } catch (BadLocationException ex) {
            //
        }

    }

}
