using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;

namespace Homesys.Web
{
    public class EmptyWebProxy : IWebProxy
    {
        ICredentials _credentials;

        public Uri GetProxy(Uri uri)
        {
            return uri;
        }

        public bool IsBypassed(Uri uri)
        {
            return true;
        }

        public ICredentials Credentials
        {
            get
            {
                return _credentials;
            }
            set
            {
                _credentials = value;
            }
        }
    }
}
