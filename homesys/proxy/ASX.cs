using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml.Serialization;

namespace Homesys.XML.ASX
{
	public class Ref
	{
        [XmlAttribute]
		public string href;
	};

	public class Entry
	{
        public string title;

		public string author;

		public string copyright;

		[XmlElementAttribute("ref", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
		public Ref[] refs;
	};

	[XmlRootAttribute("asx")]
	public class ASX
	{
        public string title;

		[XmlElementAttribute("entry", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
		public Entry[] entries;
	};
}
