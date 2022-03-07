 using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration.Install;
using System.ServiceProcess;
using System.ServiceModel;
using Microsoft.Win32;
using System.Runtime.InteropServices;

namespace Homesys
{
	[RunInstaller(true)]
	public partial class WinServiceInstaller : Installer
    {
        #region Interop

        enum RecoverAction
        {
            None = 0, Restart = 1, Reboot = 2, RunCommand = 3
        }

        class FailureAction
        {
            RecoverAction _type = RecoverAction.None;
            int _delay = 0;

            public FailureAction() { }
            
            public FailureAction(RecoverAction type, int delay)
            {
                _type = type;
                _delay = delay;
            }

            public RecoverAction Type
            {
                get { return _type; }
                set { _type = value; }
            }

            public int Delay
            {
                get { return _delay; }
                set { _delay = value; }
            }
        }
        
        const int SERVICE_ALL_ACCESS = 0xF01FF;
        const int SC_MANAGER_ALL_ACCESS = 0xF003F;
        const int SERVICE_CONFIG_DESCRIPTION = 0x1;
        const int SERVICE_CONFIG_FAILURE_ACTIONS = 0x2;
        const int SERVICE_NO_CHANGE = -1;
        const int ERROR_ACCESS_DENIED = 5;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct SERVICE_FAILURE_ACTIONS
        {
            public int dwResetPeriod;
            [MarshalAs(UnmanagedType.LPWStr)] public string lpRebootMsg;
            [MarshalAs(UnmanagedType.LPWStr)] public string lpCommand;
            public int cActions;
            public IntPtr lpsaActions;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class SC_ACTION
        {
            public Int32 type;
            public UInt32 dwDelay;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct QueryServiceConfigStruct
        {
            public int serviceType;
            public int startType;
            public int errorControl;
            public IntPtr binaryPathName;
            public IntPtr loadOrderGroup;
            public int tagID;
            public IntPtr dependencies;
            public IntPtr startName;
            public IntPtr displayName;
        }

        public struct ServiceInfo
        {
            public int serviceType;
            public int startType;
            public int errorControl;
            public string binaryPathName;
            public string loadOrderGroup;
            public int tagID;
            public string dependencies;
            public string startName;
            public string displayName;
        }
        
        [DllImport("advapi32.dll")]
        static extern IntPtr OpenSCManager(string lpMachineName, string lpDatabaseName, int dwDesiredAccess);

        [DllImport("advapi32.dll")]
        static extern IntPtr OpenService(IntPtr hSCManager, string lpServiceName, int dwDesiredAccess);

        [DllImport("advapi32.dll", EntryPoint = "ChangeServiceConfig2")]
        static extern bool ChangeServiceFailureActions(IntPtr hService, int dwInfoLevel, [MarshalAs(UnmanagedType.Struct)] ref SERVICE_FAILURE_ACTIONS lpInfo);

        [DllImport("advapi32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern Boolean QueryServiceConfig(IntPtr hService, IntPtr queryServiceConfig, UInt32 cbBufSize, out UInt32 pcbBytesNeeded);

        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, SetLastError = true, EntryPoint = "QueryServiceConfig2W")]
        static extern Boolean QueryServiceConfig2(IntPtr hService, UInt32 dwInfoLevel, IntPtr buffer, UInt32 cbBufSize, out UInt32 pcbBytesNeeded);

        [DllImport("kernel32.dll")]
        static extern int GetLastError();

        #endregion

        public WinServiceInstaller()
		{
			InitializeComponent();
		}

        protected override void OnBeforeInstall(IDictionary savedState)
        {
            base.OnBeforeInstall(savedState);

            try
            {
                ServiceInfo si = GetServiceInfo("upnphost");

                if(si.startType != 4) // disabled
                {
                    serviceInstaller1.ServicesDependedOn = new string[] { "upnphost" };
                }
            }
            catch(Exception)
            {
            }
        }

        protected override void OnAfterInstall(IDictionary savedState)
		{
			base.OnAfterInstall(savedState);

            try
            {
                string dir = Context.Parameters["targetdir"].TrimEnd('\\') + '\\';
                string exe = '"' + dir + "homesys.exe" + '"' + " \"%1\"";
                string hfa = "Homesys.FileAssoc";

                using(RegistryKey homesys = Registry.LocalMachine.CreateSubKey(@"Software\Homesys"))
                {
                    using(RegistryKey caps = homesys.CreateSubKey(@"Capabilities"))
                    {
                        caps.SetValue("ApplicationDescription", "Homesys");
                        caps.SetValue("ApplicationName", "Homesys");
                        caps.SetValue("Hidden", 0, RegistryValueKind.DWord);

                        using(RegistryKey assoc = caps.CreateSubKey(@"FileAssociations"))
                        {
                            assoc.SetValue(".dvr-ms", hfa);
                            assoc.SetValue(".avi", hfa);
                            assoc.SetValue(".mpg", hfa);
                            assoc.SetValue(".mpeg", hfa);
                            assoc.SetValue(".mkv", hfa);
                            assoc.SetValue(".mka", hfa);
                            assoc.SetValue(".wav", hfa);
                            assoc.SetValue(".mp3", hfa);
                            assoc.SetValue(".ts", hfa);
                            assoc.SetValue(".m2ts", hfa);
                            assoc.SetValue(".ogg", hfa);
                            assoc.SetValue(".ogm", hfa);
                            assoc.SetValue(".flv", hfa);
                            assoc.SetValue(".mp4", hfa);
                            assoc.SetValue(".m4p", hfa);
                            assoc.SetValue(".asf", hfa);
                            assoc.SetValue(".wmv", hfa);
                            assoc.SetValue(".wma", hfa);
                            assoc.SetValue(".ac3", hfa);
                            assoc.SetValue(".dts", hfa);
                            assoc.SetValue(".mp2", hfa);
                            assoc.SetValue(".mpa", hfa);
                            assoc.SetValue(".cda", hfa);
                            assoc.SetValue(".dsm", hfa);
                            assoc.SetValue(".vob", hfa);
                            assoc.SetValue(".ifo", hfa);
                            assoc.SetValue(".midi", hfa);
                            assoc.SetValue(".m4a", hfa);
                            assoc.SetValue(".aac", hfa);
                            assoc.SetValue(".mov", hfa);
                            assoc.SetValue(".rm", hfa);
                            assoc.SetValue(".rmvb", hfa);
                            assoc.SetValue(".ra", hfa);
                            assoc.SetValue(".3gp", hfa);
                            assoc.SetValue(".divx", hfa);
                            assoc.SetValue(".podcast", hfa);
                        }
                    }

                    using(RegistryKey command = homesys.CreateSubKey(@"shell\open\command"))
                    {
                        command.SetValue("", exe);
                    }
                }

                using(RegistryKey command = Registry.ClassesRoot.CreateSubKey(hfa + @"\shell\open\command"))
                {
                    command.SetValue("", exe);
                }

                using(RegistryKey command = Registry.LocalMachine.CreateSubKey(@"Software\RegisteredApplications"))
                {
                    command.SetValue("Homesys", @"Software\Homesys\Capabilities");
                }
            }
            catch(Exception)
            {
            }

            try
            {
                using(ServiceController sc = new ServiceController(serviceInstaller1.ServiceName))
                {
                    if(sc.Status == ServiceControllerStatus.Stopped)
                    {
                        sc.Start();

                        sc.WaitForStatus(ServiceControllerStatus.Running, new TimeSpan(0, 0, 0, 15));
                    }

                    sc.Close();
                }
            }
            catch(Exception)
            {
            }

            try
            {
                SetRecoveryOptions(serviceInstaller1.ServiceName, 3000, 3000);
            }
            catch(Exception)
            {
            }
        }

        public static void SetRecoveryOptions(string name, int first, int second)
        {
            IntPtr scmHndl = IntPtr.Zero;
            IntPtr svcHndl = IntPtr.Zero;
            IntPtr tmpBuf = IntPtr.Zero;
            IntPtr svcLock = IntPtr.Zero;

            try
            {
                scmHndl = OpenSCManager(null, null, SC_MANAGER_ALL_ACCESS);

                if(scmHndl.ToInt32() <= 0)
                {
                    return;
                }

                svcHndl = OpenService(scmHndl, name, SERVICE_ALL_ACCESS);

                if(svcHndl.ToInt32() <= 0)
                {
                    return;
                }

                ArrayList FailureActions = new ArrayList();

                FailureActions.Add(new FailureAction(RecoverAction.Restart, first)); // First Failure Actions and Delay (msec)
                FailureActions.Add(new FailureAction(RecoverAction.Restart, second)); // Second Failure Actions and Delay (msec)
                FailureActions.Add(new FailureAction(RecoverAction.None, 0)); // Subsequent Failures Actions and Delay (msec)

                int numActions = FailureActions.Count;

                int[] myActions = new int[numActions * 2];

                int currInd = 0;

                foreach(FailureAction fa in FailureActions)
                {
                    myActions[currInd] = (int)fa.Type;
                    myActions[++currInd] = fa.Delay;
                    currInd++;
                }

                tmpBuf = Marshal.AllocHGlobal(numActions * 8); // Need to pack 8 bytes per struct

                Marshal.Copy(myActions, 0, tmpBuf, numActions * 2); // Move array into marshallable pointer

                // Set the SERVICE_FAILURE_ACTIONS struct

                SERVICE_FAILURE_ACTIONS sfa = new SERVICE_FAILURE_ACTIONS();

                sfa.cActions = 3;
                sfa.dwResetPeriod = 3600 * 24;
                sfa.lpCommand = null;
                sfa.lpRebootMsg = null;
                sfa.lpsaActions = new IntPtr(tmpBuf.ToInt32());

                // Call the ChangeServiceFailureActions() abstraction of ChangeServiceConfig2()

                if(!ChangeServiceFailureActions(svcHndl, SERVICE_CONFIG_FAILURE_ACTIONS, ref sfa))
                {
                    int err = GetLastError();

                    if(err == ERROR_ACCESS_DENIED)
                    {
                        throw new Exception("Access Denied while setting Failure Actions");
                    }
                }

                // Free the memory

                Marshal.FreeHGlobal(tmpBuf);

                tmpBuf = IntPtr.Zero;
            }
            catch(Exception)
            {
            }
        }

        public static ServiceInfo GetServiceInfo(string ServiceName)
        {
            if(ServiceName.Equals(""))
            {
                throw new NullReferenceException("ServiceName must contain a valid service name.");
            }

            IntPtr scManager = OpenSCManager(null, null, SC_MANAGER_ALL_ACCESS);

            if(scManager.ToInt32() <= 0)
            {
                throw new Win32Exception();
            }

            IntPtr service = OpenService(scManager, ServiceName, SERVICE_ALL_ACCESS);

            if(service.ToInt32() <= 0)
            {
                throw new NullReferenceException();
            }

            uint bytesNeeded;
            QueryServiceConfigStruct qscs = new QueryServiceConfigStruct();
            IntPtr qscPtr = Marshal.AllocCoTaskMem(0);

            if(!QueryServiceConfig(service, qscPtr, 0, out bytesNeeded) && bytesNeeded == 0)
            {
                throw new Win32Exception();
            }
            else
            {
                qscPtr = Marshal.AllocCoTaskMem((int)bytesNeeded);
                
                if(!QueryServiceConfig(service, qscPtr, bytesNeeded, out bytesNeeded))
                {
                    throw new Win32Exception();
                }

                qscs.binaryPathName = IntPtr.Zero;
                qscs.dependencies = IntPtr.Zero;
                qscs.displayName = IntPtr.Zero;
                qscs.loadOrderGroup = IntPtr.Zero;
                qscs.startName = IntPtr.Zero;

                qscs = (QueryServiceConfigStruct)Marshal.PtrToStructure(qscPtr, new QueryServiceConfigStruct().GetType());
            }

            ServiceInfo serviceInfo = new ServiceInfo();

            serviceInfo.binaryPathName = Marshal.PtrToStringAuto(qscs.binaryPathName);
            serviceInfo.dependencies = Marshal.PtrToStringAuto(qscs.dependencies);
            serviceInfo.displayName = Marshal.PtrToStringAuto(qscs.displayName);
            serviceInfo.loadOrderGroup = Marshal.PtrToStringAuto(qscs.loadOrderGroup);
            serviceInfo.startName = Marshal.PtrToStringAuto(qscs.startName);
            serviceInfo.errorControl = qscs.errorControl;
            serviceInfo.serviceType = qscs.serviceType;
            serviceInfo.startType = qscs.startType;
            serviceInfo.tagID = qscs.tagID;

            Marshal.FreeCoTaskMem(qscPtr);

            return serviceInfo;
        }
    }
}