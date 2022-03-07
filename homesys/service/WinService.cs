using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.ServiceProcess;
using System.Text;
using System.ServiceModel;
using System.ServiceModel.Description;

namespace Homesys
{
	public partial class WinService : ServiceBase
	{
        ServiceHost _serviceHost;

		public WinService()
		{
			InitializeComponent();
		}

		protected override void OnStart(string[] args)
		{
            if(args.Length == 0)
            {
                RequestAdditionalTime(30000);
            }

            _serviceHost = Service.Create();

            if(args.Length > 0)
            {
                Service svc = (Service)_serviceHost.SingletonInstance;

                if(!svc.Run(args))
                {
                    return;
                }
            }

            _serviceHost.Open();
        }

		protected override void OnStop()
		{
            object o = _serviceHost.SingletonInstance;

            _serviceHost.Close();

            if(o != null)
            {
                ((IDisposable)o).Dispose();
            }
		}

		protected override bool OnPowerEvent(PowerBroadcastStatus powerStatus)
		{
            Log.WriteLine("PowerEvent {0}", powerStatus);

            if(_serviceHost != null && _serviceHost.SingletonInstance != null)
            {
                Service svc = (Service)_serviceHost.SingletonInstance;

                switch(powerStatus)
                {
                    case PowerBroadcastStatus.QuerySuspend:
                        if(!svc.OnQuerySuspend()) { EventLog.WriteEntry("QuerySuspend denied"); return false; }
                        break;

                    case PowerBroadcastStatus.Suspend:
                        svc.OnSuspend();
                        break;

                    case PowerBroadcastStatus.ResumeSuspend:
                        svc.OnResumeSuspend();
                        break;
                }
            }

			return true;
		}

		protected override void OnSessionChange(SessionChangeDescription changeDescription)
		{
			EventLog.WriteEntry(String.Format("OnSessionChange({0} {1})", changeDescription.Reason, changeDescription.SessionId));

			base.OnSessionChange(changeDescription);
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
