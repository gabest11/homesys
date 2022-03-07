using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceProcess;
using System.Text;
using System.IO;

namespace Homesys.NetCard
{
    static class Program
    {
        static void Main(string[] args)
        {
            WinService svc = new WinService();

            if(args.Length >= 1 && String.Compare(args[0], "run", true) == 0)
            {
                try
                {
                    svc.Run(args);
                }
                catch(Exception e)
                {
                    Console.WriteLine(e);
                }

                return;
            }

            bool log = false;

            foreach(string s in args)
            {
                if(String.Compare(s, "log", true) == 0)
                {
                    log = true;
                }
            }

            StreamWriter sw = null;

            TextWriter tmp = null;

            if(log)
            {
                FileStream fs = new FileStream(AppDataPath + "log.txt", FileMode.Create, FileAccess.Write, FileShare.Read);

                sw = new StreamWriter(fs);

                sw.AutoFlush = true;

                tmp = Console.Out;

                Console.SetOut(sw);
            }

            ServiceBase.Run(svc);

            if(log)
            {
                Console.SetOut(tmp);

                sw.Close();
            }
        }

        public static string AppDataPath
        {
            get
            {
                string path = Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData) + Path.DirectorySeparatorChar + "HSNetCard";

                if(!Directory.Exists(path))
                {
                    Directory.CreateDirectory(path);
                }

                return path + Path.DirectorySeparatorChar;
            }
        }
    }
}
