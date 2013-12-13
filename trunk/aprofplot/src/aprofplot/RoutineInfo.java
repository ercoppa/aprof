package aprofplot;

public class RoutineInfo extends Routine {

	private int id;
	private String name;
	private String mangled_name;
	private String full_name;
	private String image;

	public RoutineInfo(int id) {
		this.id = id;
	}

	public RoutineInfo(int id, String name, String image) {
		this.id = id;
		this.name = name;
		this.image = image.substring(image.lastIndexOf('/') + 1);
    }

	public void setName(String n) {
		name = n;
	}

	@Override
	public String getName() {
		if (full_name != null)
			return full_name;
		
		return name;
	}

	public void setFullName(String name) {
		full_name = name;
	}

	@Override
	public String getFullName() {
		return name;
	}

	@Override
	public String getMangledName() {
		return mangled_name;
	}
	
	public void setMangledName(String name) {
		mangled_name = name;
	}

	public void setImage(String i) {
		image = i;
	}

	@Override
	public String getImage() {
		return image;
	}

	@Override
	public int getID() {
		return id;
	}
	
	@Override
	public String toString() {
		return this.getName();
	}

	@Override
	public boolean equals(Object o) {
		if (o != null && getClass().equals(o.getClass())) {
			RoutineInfo rtn_info = (RoutineInfo)o;
			return rtn_info.id==this.id;
		}
		else return false;
	}

	@Override
	public int hashCode() {
		int hash = 3;
		hash = 31 * hash + this.id;
		return hash;
	}
    
}
