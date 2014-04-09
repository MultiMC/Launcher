import java.lang.Comparable;
import java.io.Serializable;
import java.util.AbstractList;

public abstract class ClassParserTestClass extends AbstractList<String> implements Comparable, Serializable
{
	public static final String publicStaticFinalStringDefault = "Ì'm å ÛTF-8 ŝtrïñg";
	private Integer privateInteger;
	protected static volatile Comparable protectedStaticVolatileComparable;
	public transient Float publicTransientFloat;
	// TODO synthetic and enum
}
