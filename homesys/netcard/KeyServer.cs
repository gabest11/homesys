using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.IO;

namespace Homesys.NetCard
{
    public class KeyServer
    {
        volatile bool _exit = false;

        public KeyServer()
        {
            Thread t = new Thread(new ThreadStart(Run));

            t.Start();
        }

        void Run()
        {
            TcpListener server = new TcpListener(IPAddress.Any, 4896);

            server.Start();

            while(!_exit)
            {
                while(server.Pending())
                {
                    new KeyClient(server.AcceptTcpClient());
                }

                Thread.Sleep(1);
            }
        }

        public void Close()
        {
            _exit = true;
        }
    }

    class KeyClient
    {
        static Dictionary<long, ECM> _ecms = new Dictionary<long, ECM>();
        static DateTime _ecms_ts = DateTime.Now;

        TcpClient _client;
        BinaryReader _reader;
        BinaryWriter _writer;
        Dictionary<long, DateTime> _gotkey;

        public KeyClient(TcpClient client)
        {
            _client = client;

            _gotkey = new Dictionary<long, DateTime>();

            Thread t = new Thread(new ThreadStart(Run));

            t.Start();
        }

        void Run()
        {
            string ip = "";

            try
            {
                ip = _client.Client.RemoteEndPoint.ToString();

                Console.WriteLine("Connected {0}", ip);

                _reader = new BinaryReader(_client.GetStream());
                _writer = new BinaryWriter(_client.GetStream());

                while(true)
                {
                    long hash = _reader.ReadInt64();

                    if(hash < 0)
                    {
                        hash &= 0x7fffffffffffffff;

                        SendKey(hash);
                    }
                    else
                    {
                        ReceiveKey(hash);
                    }
                }
            }
            catch(Exception e)
            {
                Console.WriteLine(e);
            }

            try
            {
                Console.WriteLine("Disconnected {0}", ip);

                _client.Close();
            }
            catch(Exception e)
            {
                Console.WriteLine(e);
            }
        }

        void ReceiveKey(long hash)
        {
            byte[] cws = _reader.ReadBytes(_reader.ReadUInt16());

            Console.WriteLine("{0} R {1} {2}", _client.Client.RemoteEndPoint, hash.ToString("X").PadLeft(16, '0'), HexToString(cws).PadLeft(32, '0'));
            
            lock(_ecms)
            {
                DateTime now = DateTime.Now;
                TimeSpan diff = new TimeSpan(0, 1, 0);

                if(now - _ecms_ts > diff)
                {
                    foreach(var pair in _ecms.Where(r => now - r.Value._ts > diff).ToList())
                    {
                        _ecms.Remove(pair.Key);
                    }

                    _ecms_ts = now;
                }

                _ecms[hash] = new ECM(cws);
            }

            _gotkey[hash >> 48] = DateTime.Now;
        }

        void SendKey(long hash)
        {
            int i = 0;
            int j = 50;

            DateTime t;

            if(_gotkey.TryGetValue(hash >> 48, out t) && DateTime.Now - t < new TimeSpan(0, 0, 20))
            {
                j = 1;
            }

            while(i < j)
            {
                lock(_ecms)
                {
                    ECM ecm = null;

                    if(_ecms.TryGetValue(hash, out ecm))
                    {
                        Console.WriteLine("{0} W {1} {2}", _client.Client.RemoteEndPoint, hash.ToString("X").PadLeft(16, '0'), HexToString(ecm._cws).PadLeft(32, '0'));

                        _writer.Write((UInt16)ecm._cws.Length);
                        _writer.Write(ecm._cws);

                        return;
                    }
                }

                if(++i < j) Thread.Sleep(100);
            }

            Console.WriteLine("{0} W {1} missed {2}", _client.Client.RemoteEndPoint, hash.ToString("X").PadLeft(16, '0'), j);

            _writer.Write((UInt16)0);
        }

        string HexToString(byte[] buff)
        {
            string s = "";

            foreach(byte b in buff)
            {
                s += b.ToString("X").PadLeft(2, '0');
            }

            return s;
        }
    }

    class ECM
    {
        public byte[] _cws;
        public DateTime _ts;

        public ECM(byte[] cws)
        {
            if(cws == null || cws.Length <= 0)
            {
                throw new ArgumentException("buff");
            }

            _cws = new byte[cws.Length];
            _ts = DateTime.Now;

            Array.Copy(cws, _cws, cws.Length);
        }
    }
}
