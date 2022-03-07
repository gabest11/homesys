using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Data;
using System.Threading;
using System.Data.SqlServerCe;

namespace Homesys
{
    internal class ConnectionPool
    {
        Dictionary<int, SqlCeConnection> _map;
        string _cs;

        public ConnectionPool(string cs)
        {
            _map = new Dictionary<int, SqlCeConnection>();
            _cs = cs;
        }

        public SqlCeConnection Connection
        {
            get
            {
                lock(_cs)
                {
                    int threadId = Thread.CurrentThread.ManagedThreadId;

                    SqlCeConnection c = null;

                    if(!_map.TryGetValue(threadId, out c))
                    {
                        c = new SqlCeConnection(_cs);

                        c.Open();

                        _map.Add(threadId, c);
                    }
     
                    return c;
                }
            }
        }
    }
}

 
    