using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.IO; 

namespace Homesys.SmartCard
{
    public class KeyServer
    {
        public KeyServer()
        {
            TcpListener server = new TcpListener(IPAddress.Any, 4296);

            server.Start();

            while(true)
            {
                new KeyClient(server.AcceptTcpClient());
            }
        }
    }

    class KeyClient
    {
        static Dictionary<ECM, byte[]> _ecms = new Dictionary<ECM, byte[]>();
        static DateTime _ecms_ts = DateTime.Now;

        TcpClient _client;
        BinaryReader _reader;
        BinaryWriter _writer;

        public KeyClient(TcpClient client)
        {
            _client = client;

            Thread t = new Thread(new ThreadStart(Run));

            t.Start();
        }

        void Run()
        {
            try
            {
                _reader = new BinaryReader(_client.GetStream());
                _writer = new BinaryWriter(_client.GetStream());

                while(true)
                {
                    ushort pid = _reader.ReadUInt16();
                    ushort size = _reader.ReadUInt16();
                    byte[] buff = _reader.ReadBytes(_reader.ReadUInt16());

                    switch(_reader.ReadByte())
                    {
                        case 1: ReceiveKey(pid, size, buff); break;
                        case 2: SendKey(pid, size, buff); break;
                        default: throw new InvalidDataException();
                    }
                }
            }
            catch(Exception e)
            {
                Console.WriteLine(e);
            }

            try
            {
                _client.Close();
            }
            catch(Exception e)
            {
                Console.WriteLine(e);
            }
        }

        void ReceiveKey(ushort pid, ushort size, byte[] buff)
        {
            byte[] cws = _reader.ReadBytes(_reader.ReadUInt16());

            ECM ecm = new ECM(pid, size, buff);
            /*
            Console.WriteLine("R {0} {1} {2} {3} {4} {5}",
                pid, size, ecm.GetHashCode().ToString("X").PadLeft(8, '0'),
                HexToString(cws).PadLeft(16, '0'),
                buff.Length,
                HexToString(buff));
            */
            lock(_ecms)
            {
                DateTime now = DateTime.Now;
                TimeSpan diff = new TimeSpan(0, 1, 0);

                if(now - _ecms_ts > diff)
                {
                    foreach(var pair in _ecms.Where(r => now - r.Key._ts > diff).ToList())
                    {
                        _ecms.Remove(pair.Key);
                    }

                    _ecms_ts = now;
                }

                _ecms[ecm] = cws;
            }
        }

        void SendKey(ushort pid, ushort size, byte[] buff)
        {
            ECM ecm = new ECM(pid, size, buff);
            /*
            Console.WriteLine("R {0} {1} {2} {3} {4} {5}",
                pid, size, ecm.GetHashCode().ToString("X").PadLeft(8, '0'),
                "".PadLeft(16, '0'),
                buff.Length,
                HexToString(buff));
            */
            for(int i = 0; i < 10; i++)
            {
                if(SendKey(ecm))
                {
                    return;
                }

                Thread.Sleep(100);
            }

            Console.WriteLine("missed");

            _writer.Write((UInt16)0);
        }

        bool SendKey(ECM ecm)
        {
            byte[] cws = null;

            lock(_ecms)
            {
                if(_ecms.TryGetValue(ecm, out cws))
                {
                    _writer.Write((UInt16)cws.Length);
                    _writer.Write(cws);

                    return true;
                }
            }

            return false;
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
        int _hash;

        public ushort _pid;
        public ushort _size;
        public byte[] _buff;
        public DateTime _ts;

        public ECM(ushort pid, ushort size, byte[] buff)
        {
            if(buff == null || buff.Length <= 0)
            {
                throw new ArgumentException("buff");
            }

            _pid = pid;
            _size = size;
            _buff = new byte[buff.Length];
            _ts = DateTime.Now;

            Array.Copy(buff, _buff, buff.Length);

            _hash = 0;

            foreach(byte b in _buff)
            {
                _hash = (_hash * 31) ^ b;
            }
        }

        bool Equals(ECM ecm)
        {
            if(_size != ecm._size || _buff.Length != ecm._buff.Length)
            {
                return false;
            }

            for(int i = 0; i < _buff.Length; i++)
            {
                if(_buff[i] != ecm._buff[i])
                {
                    return false;
                }
            }

            return true;
        }

        public override bool Equals(object obj)
        {
            return obj is ECM && Equals((ECM)obj);
        }

        public override int GetHashCode()
        {
            return _hash;
        }
    }
}
