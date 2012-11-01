import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;


public class Test {
	public static void main(String[] args){
		
		try {
			BufferedReader pre = new BufferedReader(new FileReader(args[0]));
			BufferedReader post = new BufferedReader(new FileReader(args[1]));
			int row_num;
			String p1, p2;
			p1 = pre.readLine();
			p2 = post.readLine();
			
			row_num = 1;
			
			if(!p1.equals("STACKS")||!p2.equals("STACKS")){
				pre.close();post.close();return;
			}

			p1 = pre.readLine();
			p2 = post.readLine();
			
			row_num++;
			
			ArrayList<Double> array = new ArrayList();
			ArrayList<Double> tmp = new ArrayList();
			
			System.out.println("\n\n\nLeggo le stack prima dell'overflow...");

			while(!(p1 = pre.readLine()).equals(""))
				array.add(Double.parseDouble(p1));
			
			while(!(p1 = pre.readLine()).equals("$")){
				if(p1.equals("")){
					ArrayList<Double> app = new ArrayList();

					int i = 0;
					int j = 0;
					
					while(i<array.size()||j<tmp.size()){
						if(i<array.size()&&j<tmp.size()){
							if(array.get(i)>tmp.get(j))
								app.add(array.get(i++));
							else
								app.add(tmp.get(j++));
						}
						if(i<array.size()&& j>=tmp.size())
							app.add(array.get(i++));
							
						if(i>=array.size()&&j<tmp.size())
							app.add(tmp.get(j++));
					}
					
					array = app;
					tmp = new ArrayList();
					
				}
				else
				tmp.add(Double.parseDouble(p1));
				
			}
			
			
			pre.close();
			
			pre = new BufferedReader(new FileReader(args[0]));
			
			
			p1 = pre.readLine();
			p1 = pre.readLine();
			
			row_num++;
			p1 = pre.readLine();
			p2 = post.readLine();

			System.out.println("\nVerifico la correttezza dei nuovi stack...");
			int i=0;
			System.out.print("\nStack "+(i++)+"...");
		
			/* preso un nuovo ts verificare che il nuovo valore corrisponde
 * 				al ts pre_overflow*/
	
			while(!p1.equals("$")&&!p2.equals("$")){
				
		
				double d1 = Double.parseDouble(p1);
				double d2 = Double.parseDouble(p2);
				int app = array.size()-1 -(int)((d2-1)/2); //check line
				if(d1!=array.get(app)){
					System.out.println("ERRORE "+d1+" "+array.get(app) + " "+d2+" "+ app);
					System.out.println(array);
					return;
				}
				p1 = pre.readLine();
				p2 = post.readLine();
				if(p1.equals("")){
					row_num++;
					System.out.println("OK");
					
				p1 = pre.readLine();
				p2 = post.readLine();
				if((!p1.equals("$")&&!p2.equals("$")))
					System.out.print("Stack "+(i++)+"...");
				}

				
				
			}
			
			p1 = pre.readLine();
			p2 = post.readLine();
			row_num++;
			p1 = pre.readLine();
			p2 = post.readLine();
			row_num++;
			row_num+=array.size();

			if(!p1.equals(p2) || !p1.equals("GLOBAL SHADOW MEMORY")){
				System.out.println("ERRORE: Report danneggiati");
				return;
			}

			/* preso un nuovo ts verifico se in quale intervallo è compreso
 * 				e se il  ts pre_overflow è compreso nel medesimo intervallo*/

			System.out.print("\nVerifico correttezza GSM...");

			ArrayList<Double> app = new ArrayList<Double>();
			i = 0;
			while(i<array.size()){
				app.add(0.0);
				app.add(array.get(array.size() -1 -i++));
			}
			app.add(0.0);
			array = app;
			
			p1 = pre.readLine();
			p2 = post.readLine();
			p1 = pre.readLine();
			p2 = post.readLine();
			row_num+=2;
			boolean sm = true;
			while(!p1.equals("")&&!p2.equals("")){
				double d1 = Double.parseDouble(p1);
				int d2 = Integer.parseInt(p2); //check line
				if((d2+1 > array.size() && array.get(d2+1)<d1) || (d2-1>0 && array.get(d2-1)>d1)){
					if(sm)
					System.out.println();

					System.out.println("\nERRORE: riga #"+row_num);
					sm = false;
				}

				p1 = pre.readLine();
				p2 = post.readLine();
				row_num++;		
			}
			
			
			
			
			p1 = pre.readLine();
			p2 = post.readLine();
			row_num++;
			
			if(p1.equals(p2) && !p1.equals("$")){
				System.out.println("\nERRORE REPORT 1");
				return;
			}
			
		
			if(sm)
				System.out.println("OK");
			else
				System.out.println("\n GSM FAIL");
			
			p1 = pre.readLine();
			p2 = post.readLine();
			row_num++;
			
			/* preso un nuovo ts verifico se in quale intervallo è compreso
 * 				e se il  ts pre_overflow è compreso nel medesimo intervallo*/
			

			System.out.println("\nVerifico correttezza SM private...\n");
			
			boolean fine = sm;
			
			i=0;
			while(p1!=null && p2!= null){
				p1 = pre.readLine();
				p2 = post.readLine();
				row_num++;
				
				if(!p1.equals(p2)||!p1.equals("SHADOW MEMORY PRIVATA")){
					System.out.println("\nERRORE REPORT 2 "+p1+" "+p2);
					return;
				}
				
				p1 = pre.readLine();
				p2 = post.readLine();
				row_num++;
				p1 = pre.readLine();
				p2 = post.readLine();
				row_num++;
				
				System.out.print("Shadow Memory Privata "+i+"...");
				
				sm = true;
				
				while(!p1.equals("")&&!p2.equals("")){
					double d1 = Double.parseDouble(p1);
					int d2 = Integer.parseInt(p2);//check line
					if((d2+1 > array.size() && array.get(d2+1)<d1) || (d2>=0 && array.get(d2)>d1)){
						if(sm)
						System.out.println();

						System.out.println("\nERRORE: riga #"+row_num);
						sm = false;
					}

					p1 = pre.readLine();
					p2 = post.readLine();
					row_num++;		
				}
				
				if(sm)
					System.out.println("OK");
				else
					System.out.println("\n Shadow Memory Privata "+i+"FAIL");
				i++;
				
				fine = fine && sm;
				
				p1 = pre.readLine();
				p2 = post.readLine();
				row_num++;	
				
				p1 = pre.readLine();
				p2 = post.readLine();
				row_num++;	
				
				
			}
			
			if(fine)
				System.out.println("\nCheck Completo: TUTTO OK!");
			else
				System.out.println("\nCheck Completo: FAIL!");
			
		
			
			
			
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}
