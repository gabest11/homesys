using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Data;
using System.Data.SqlServerCe;
using System.Net;
using System.Xml.Serialization;
using System.Security.Cryptography;
using System.Linq;
using System.Reflection;
using System.Transactions;
using System.Diagnostics;

/*

To create a new DB.data.cs with sqlce4 and VS2022: 

1. copy sqlmetal.exe and sqlmetal.exe.config into the bin directory where System.Data.SqlServerCe.dll is

2. add the following to sqlmetal.exe.config

  <system.data>
    <DbProviderFactories>
        <remove invariant="System.Data.SqlServerCe.3.5" />
        <add name="Microsoft SQL Server Compact Data Provider 4.0" 
             invariant="System.Data.SqlServerCe.3.5" 
             description=".NET Framework Data Provider for Microsoft SQL Server Compact" 
             type="System.Data.SqlServerCe.SqlCeProviderFactory, System.Data.SqlServerCe, Version=4.0.0.0, Culture=neutral, PublicKeyToken=89845dcd8080cc91" />
    </DbProviderFactories>
  </system.data>

3. run DB.cmd there

4. copy DB.data.cs back here

*/

namespace Homesys
{
    public enum WebUpdateType
    {
        Guide,
        Recordings,
        OverlappingRecordings,
    }

    internal class DB
    {
        static uint DB_E_ERRORSINCOMMAND = 0x80040e14;
        static uint DB_E_NOTABLE = 0x80040e37;

        ConnectionPool _pool;

        public DB(string fn)
        {
            string cs = "Data Source = '" + fn + "'; Max Database Size = 512; ";

            if(!File.Exists(fn))
            {
                using(SqlCeEngine engine = new SqlCeEngine(cs))
                {
                    engine.CreateDatabase();
                }
            }
            else
            {
                using(SqlCeConnection c = new SqlCeConnection(cs))
                {
                    try
                    {
                        c.Open();
                    }
                    catch(SqlCeException e)
                    {
                        if((uint)e.NativeError == 0x000061c4)
                        {
                            cs += "Password = 1234; ";
                        }
                    }
                }

                using(SqlCeEngine engine = new SqlCeEngine(cs))
                {
                    try
                    {
                        engine.Shrink();
                        engine.Upgrade();
                    }
                    catch(Exception)// e)
                    {
                        // Log.WriteLine(e.Message);
                    }
                }
            }

            _pool = new ConnectionPool(cs);

            StreamReader txt = new StreamReader(Assembly.GetExecutingAssembly().GetManifestResourceStream("Homesys.DB.txt"));

            string table = null;
            List<string> rows = new List<string>();
            List<string> index = new List<string>();

            while(!txt.EndOfStream)
            {
                string s = txt.ReadLine().Trim();

                if(s != "")
                {
                    if(table == null)
                    {
                        table = s;
                    }
                    else
                    {
                        rows.Add(s);
                    }
                }
                else if(table != null)
                {
                    if(!IsTablePresent(table))
                    {
                        string create = "CREATE TABLE [" + table + "] (";

                        foreach(string row in rows)
                        {
                            s = row;

                            if(s.StartsWith("//"))
                            {
                                continue;
                            }

                            if(s.StartsWith("*"))
                            {
                                s = s.TrimStart('*');

                                index.Add(s.Substring(0, s.IndexOf(' ')));
                            }

                            create += s + ",";
                        }

                        create = create.TrimEnd(',') + ')';

                        SqlCeCommand cmd = Connection.CreateCommand();

                        cmd.CommandText = create;
                        cmd.ExecuteNonQuery();

                        foreach(string field in index)
                        {
                            cmd.CommandText = "CREATE INDEX idx_" + table + "_" + field + " ON [" + table + "] ([" + field + "])";
                            cmd.ExecuteNonQuery();
                        }

                        cmd.Dispose();

                        index.Clear();
                    }
                    else
                    {
                        foreach(string row in rows)
                        {
                            s = row;

                            if(s.StartsWith("//"))
                            {
                                continue;
                            }

                            bool createIndex = false;

                            if(s.StartsWith("*"))
                            {
                                s = s.TrimStart('*');

                                createIndex = true;
                            }

                            string field = s.Substring(0, s.IndexOf(' '));

                            if(!IsFieldPresent(table, field))
                            {
                                SqlCeCommand cmd = Connection.CreateCommand();

                                cmd.CommandText = "ALTER TABLE [" + table + "] ADD " + s;
                                cmd.ExecuteNonQuery();

                                if(createIndex)
                                {
                                    cmd.CommandText = "CREATE INDEX idx_" + table + "_" + field + " ON [" + table + "] ([" + field + "])";
                                    cmd.ExecuteNonQuery();
                                }
                            }
                        }
                    }

                    table = null;
                    rows.Clear();
                }
            }

            using(Data.HomesysDB Context = new Data.HomesysDB(Connection))
            {
                if(Context.Parental.Count() == 0)
                {
                    Context.Parental.InsertOnSubmit(new Data.Parental()
                    {
                        Password = "",
                        Enabled = false,
                        Inactivity = 0,
                        Rating = 0
                    });
                }

                foreach(var r in Context.Recording.Where(row => row.Channel_id == 0))
                {
                    if(r.Program_id.HasValue)
                    {
                        Data.Program program = Context.Program.Where(row => row.Id == r.Program_id.Value).SingleOrDefault();

                        if(program != null && program.Channel_id != 0)
                        {
                            Log.WriteLine("{0} {1}", r.Id, program.Channel_id);

                            r.Channel_id = program.Channel_id;
                        }
                    }

                    if(r.Preset_id.HasValue)
                    {
                        Data.Preset preset = Context.Preset.Where(row => row.Id == r.Preset_id.Value).SingleOrDefault();

                        if(preset != null && preset.Channel_id != 0)
                        {
                            Log.WriteLine("{0} {1}", r.Id, preset.Channel_id);

                            r.Channel_id = preset.Channel_id;
                        }
                    }
                }

                Context.SubmitChanges();
            }
        }

        public SqlCeConnection Connection
        {
            get { return _pool.Connection; }
        }

        public bool IsTablePresent(string table)
        {
            try
            {
                SqlCeCommand cmd = Connection.CreateCommand();

                cmd.CommandText = string.Format("SELECT * FROM [{0}]", table);

                cmd.ExecuteNonQuery();
            }
            catch(SqlCeException e)
            {
                if((uint)e.HResult == DB_E_NOTABLE)
                {
                    return false;
                }

                throw e;
            }

            return true;
        }

        public bool IsFieldPresent(string table, string field)
        {
            try
            {
                SqlCeCommand cmd = Connection.CreateCommand();

                cmd.CommandText = string.Format("SELECT [{0}] FROM [{1}]", field, table);

                cmd.ExecuteNonQuery();
            }
            catch(SqlCeException e)
            {
                if((uint)e.HResult == DB_E_ERRORSINCOMMAND)
                {
                    return false;
                }

                throw e;
            }

            return true;
        }
    }

    namespace Data
    {
        public partial class HomesysDB : System.Data.Linq.DataContext
        {
            partial void OnCreated()
            {
                // Log = Console.Out;
            }

            public void Delete<T, TEntity>(System.Data.Linq.Table<TEntity> table, IEnumerable<T> ids, string key = "id") where TEntity : class
            {
                if(table.Count() == 0)
                {
                    return;
                }

                if(System.Transactions.Transaction.Current != null)
                {
                    Connection.EnlistTransaction(System.Transactions.Transaction.Current);
                }

                System.Data.Common.DbCommand cmd = Connection.CreateCommand();

                cmd.CommandText = string.Format("DELETE FROM [{0}] WHERE {1} = @value ", Mapping.GetTable(typeof(TEntity)).TableName, key);
                cmd.Parameters.Add(new SqlCeParameter("@value", typeof(T) == typeof(Guid) ? SqlDbType.UniqueIdentifier : SqlDbType.Int));
                cmd.Prepare();

                foreach(T id in ids)
                {
                    cmd.Parameters["@value"].Value = id;
                    cmd.ExecuteNonQuery();
                }
            }

            public void Truncate<TEntity>(System.Data.Linq.Table<TEntity> table, string id = null) where TEntity : class
            {
                string name = Mapping.GetTable(typeof(TEntity)).TableName;

                ExecuteCommand("DELETE FROM [" + name + "]");

                if(id != null)
                {
                    ExecuteCommand("ALTER TABLE [" + name + "] ALTER COLUMN " + id + " IDENTITY (1,1)");
                }
            }

            public void TruncateExcept<TEntity>(System.Data.Linq.Table<TEntity> table, IEnumerable<int> ids) where TEntity : class
            {
                int[] tmp = ids.ToArray();

                if(tmp.Length != 0)
                {
                    string name = Mapping.GetTable(typeof(TEntity)).TableName;

                    ExecuteCommand("DELETE FROM [" + name + "] WHERE Id NOT IN (" + String.Join(",", tmp) + ")");
                }
                else
                {
                    Truncate(table);
                }
            }

            public void Insert<TEntity>(System.Data.Linq.Table<TEntity> table, IEnumerable<object[]> rows, params string[] cols) where TEntity : class
            {
                if(cols.Length == 0 || rows.Count() == 0)
                {
                    return;
                }

                if(System.Transactions.Transaction.Current != null)
                {
                    Connection.EnlistTransaction(System.Transactions.Transaction.Current);
                }

                SqlCeCommand cmd = Connection.CreateCommand() as SqlCeCommand;

                cmd.CommandText = "SELECT [" + String.Join("], [", cols) + "] FROM [" + Mapping.GetTable(typeof(TEntity)).TableName + "]";

                SqlCeResultSet rs = cmd.ExecuteResultSet(ResultSetOptions.Updatable);

                SqlCeUpdatableRecord rec = rs.CreateRecord();

                foreach(object[] r in rows)
                {
                    for(int i = 0; i < r.Length; i++)
                    {
                        rec.SetValue(i, r[i]);
                    }

                    rs.Insert(rec);
                }

                rs.Close();
            }
        }
    }
}
