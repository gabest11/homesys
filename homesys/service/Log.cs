using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Homesys
{
    internal class Log
    {
        public static void WriteLine(string format, params object[] arg)
        {
            Console.WriteLine("[" + DateTime.Now.ToString("s") + "] " + String.Format(format, arg));
        }

        public static void WriteLine(object o)
        {
            Console.WriteLine("[" + DateTime.Now.ToString("s") + "] " + o.ToString());
        }
    }
}
