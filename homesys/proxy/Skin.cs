using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml.Serialization;
using System.Net;
using System.IO;

namespace Homesys.XML.Skin
{
	[Serializable, XmlRootAttribute("item")]
	public class Item
	{
        [XmlAttribute]
        public string id;

        [XmlElement]
        public string title;

        [XmlElement]
        public string desc;
    }

    public class Skin
	{
        [XmlElementAttribute("item", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public Item[] item;

        public static Skin Get(string url)
        {
			HttpWebRequest req = (HttpWebRequest)WebRequest.Create(url);

            req.Timeout = 15000;

            req.Proxy = new Web.EmptyWebProxy();

            using(WebResponse res = req.GetResponse())
            {
                using(Stream stream = res.GetResponseStream())
                {
                    XmlSerializer serializer = new XmlSerializer(typeof(Skin));

                    Skin rss = (Skin)serializer.Deserialize(stream);

                    stream.Close();

                    return rss;
                }
            }
        }
	}
}
