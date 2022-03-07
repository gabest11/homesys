using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Linq;
using System.ServiceProcess;
using System.Text;

namespace Homesys.NetCard
{
    public partial class WinService : ServiceBase
    {
        KeyServer _ks = null;

        public WinService()
        {
            InitializeComponent();
        }

        protected override void OnStart(string[] args)
        {
            if(_ks == null)
            {
                _ks = new KeyServer();
            }
        }

        protected override void OnStop()
        {
            if(_ks != null)
            {
                _ks.Close();

                _ks = null;
            }
        }

        public void Run(string[] args)
        {
            OnStart(args);

            Console.WriteLine("Press enter to terminate the service");
            Console.ReadLine();

            OnStop();
        }
    }
}
