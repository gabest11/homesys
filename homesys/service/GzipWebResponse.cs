using System;
using System.IO;
using System.IO.Compression;
using System.Diagnostics;
using System.ComponentModel;
using System.Net;
using System.Web.Services;
using System.Web.Services.Protocols;
using System.Xml.Serialization;
using System.Threading;
using System.Xml;
using System.Security.Cryptography;

namespace Homesys.Web
{
    public class GzipWebResponse : WebResponse
    {
        private WebResponse _response;
        private Stream _stream = null;

        public GzipWebResponse(WebResponse response)
        {
            _response = response;
        }

        public override Stream GetResponseStream()
        {
            if(_stream == null)
            {
                _stream = _response.GetResponseStream();

                string encoding = Headers["Content-Encoding"];
                string type = Headers["Content-Type"];

                if(string.Compare(encoding, "gzip", true) == 0)
                {
                    _stream = new GZipStream(_stream, CompressionMode.Decompress);
                }
                else if(string.Compare(encoding, "deflate", true) == 0)
                {
                    _stream.ReadByte();
                    _stream.ReadByte();

                    _stream = new DeflateStream(_stream, CompressionMode.Decompress);
                }
                else if(string.Compare(encoding, "bzip2", true) == 0 || string.Compare(type, "application/bzip2", true) == 0)
                {
                    // _stream = new ICSharpCode.SharpZipLib.BZip2.BZip2InputStream(_stream);
                    _stream = new BZip2InputStream(_stream);
                }
            }

            return _stream;
        }

        public override long ContentLength
        {
            get { return _response.ContentLength; }
            set { _response.ContentLength = value; }
        }

        public override string ContentType
        {
            get { return _response.ContentType; }
            set { _response.ContentType = value; }
        }

        public override Uri ResponseUri
        {
            get { return _response.ResponseUri; }
        }

        public override WebHeaderCollection Headers
        {
            get { return _response.Headers; }
        }
    }

    public partial class HomesysService : SoapHttpClientProtocol
    {
        public HomesysService(int timeout)
            : this()
        {
            Timeout = timeout;

            // Url = Url.Replace("homesyslite.timbuktu.hu", String.Format("homesyslite{0:d2}.timbuktu.hu", 1 + (new Random().Next() % 3)));
        }

        public HomesysService(int timeout, string username, string password, string lang)
            : this(timeout)
        {
            _sessionId = InitiateSessionWithLang(username, password, lang);
        }

        protected override WebRequest GetWebRequest(Uri uri)
        {
            HttpWebRequest req = (HttpWebRequest)base.GetWebRequest(uri);

            // req.KeepAlive = false; // == true => The server committed a protocol violation. Section=ResponseStatusLine (IIS6 + PHP + fastcgi)

            req.Headers.Add("Accept-Encoding", "bzip2"); // gzip, deflate

            req.Proxy = new EmptyWebProxy();

            return req;
        }

        protected override WebResponse GetWebResponse(WebRequest request)
        {
            return new GzipWebResponse(request.GetResponse());
        }

        private string _sessionId = "";

        public string SessionId
        {
            get { return _sessionId; }
            set { _sessionId = value ?? ""; }
        }
    }
}
