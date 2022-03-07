using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml.Serialization;
using System.Net;
using System.IO;

namespace Homesys.XML.Youtube
{
    [XmlRootAttribute("youtube")]
    public class Youtube
    {
        [XmlElementAttribute("item", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public Item[] Items;

        public static Item[] Get(string url)
        {
            HttpWebRequest req = (HttpWebRequest)WebRequest.Create(url);

            req.Timeout = 15000;

            req.Proxy = new Web.EmptyWebProxy();

            using(WebResponse res = req.GetResponse())
            {
                using(Stream stream = res.GetResponseStream())
                {
                    XmlSerializer serializer = new XmlSerializer(typeof(Youtube));

                    Youtube youtube = (Youtube)serializer.Deserialize(stream);

                    stream.Close();

                    return youtube.Items;
                }
            }
        }
    }

    [Serializable]
    public class Item
    {
        [XmlElement("author")]
        public string Author;

        [XmlElement("Modified")]
        public DateTime Modified;

        [XmlElement("thumbnail")]
        public string Thumbnail;
    }
}
