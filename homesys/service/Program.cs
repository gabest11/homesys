using System;
using System.Collections.Generic;
using System.ServiceProcess;
using System.Text;
using System.IO;
using System.Globalization;
using Microsoft.Win32;
using System.Diagnostics;

namespace Homesys
{
	static class Program
	{
        // [STAThread]
		static void Main(string[] args)
		{
            // Test();

            CoInitSec dummy = new CoInitSec();

            SetupFirewall();

            /*
            try
            {
                AppAssocRegUI regui = new AppAssocRegUI();

                regui.LaunchAdvancedAssociationUI("Homesys");

                AppAssocReg reg = new AppAssocReg();

                if(!reg.QueryAppIsDefaultAll(AssocLevel.AL_EFFECTIVE, "Homesys"))
                {
                    reg.SetAppAsDefaultAll("Homesys");
                }
            }
            catch(Exception)
            {
            }

            return;
            */

            try
            {
                using(RegistryKey key = Registry.LocalMachine.CreateSubKey("Software\\Homesys"))
                {
                    string s = (string)key.GetValue("Language");

                    if(s == null)
                    {
                        key.SetValue("Language", CultureInfo.InstalledUICulture.ThreeLetterISOLanguageName);
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e.Message);
            }

            WinService svc = new WinService();

			if(args.Length >= 1 && String.Compare(args[0], "run", true) == 0)
			{
				try
				{
					svc.Run(args);
				}
				catch(Exception e)
				{
					Log.WriteLine(e);
				}

                Console.ReadKey();

				return;
			}

            FileStream fs = new FileStream(Service.AppDataPath + "log.txt", FileMode.Create, FileAccess.Write, FileShare.Read);

            TextWriter tmp = Console.Out;

            StreamWriter sw = new StreamWriter(fs);

            sw.AutoFlush = true;

            Console.SetOut(sw);

            ServiceBase.Run(svc);

            Console.SetOut(tmp);

            sw.Close();
        }

        private static void SetupFirewall()
        {
            try
            {
                Process p = new Process();

                p.StartInfo.UseShellExecute = false;
                p.StartInfo.RedirectStandardOutput = true;
                p.StartInfo.FileName = Environment.SystemDirectory + Path.DirectorySeparatorChar + "netsh.exe";
                p.StartInfo.Arguments = "advfirewall firewall show rule name=\"Homesys Service\"";
                p.Start();

                string output = p.StandardOutput.ReadToEnd();

                p.WaitForExit();

                if(p.ExitCode != 0)
                {
                    System.Reflection.Assembly a = System.Reflection.Assembly.GetExecutingAssembly();

                    p = new Process();

                    p.StartInfo.UseShellExecute = false;
                    p.StartInfo.RedirectStandardOutput = true;
                    p.StartInfo.FileName = Environment.SystemDirectory + Path.DirectorySeparatorChar + "netsh.exe";
                    p.StartInfo.Arguments = "advfirewall firewall add rule name=\"Homesys Service\" dir=in action=allow protocol=TCP program=\"" + a.Location + "\" enable=yes profile=private";
                    p.Start();

                    output = p.StandardOutput.ReadToEnd();

                    p.WaitForExit();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e.Message);
            }

            try
            {
                Process p = new Process();

                p.StartInfo.UseShellExecute = false;
                p.StartInfo.RedirectStandardOutput = true;
                p.StartInfo.FileName = Environment.SystemDirectory + Path.DirectorySeparatorChar + "netsh.exe";
                p.StartInfo.Arguments = "advfirewall firewall show rule name=Homesys";
                p.Start();

                string output = p.StandardOutput.ReadToEnd();

                p.WaitForExit();

                if(p.ExitCode != 0)
                {
                    p = new Process();

                    p.StartInfo.UseShellExecute = false;
                    p.StartInfo.RedirectStandardOutput = true;
                    p.StartInfo.FileName = Environment.SystemDirectory + Path.DirectorySeparatorChar + "netsh.exe";
                    p.StartInfo.Arguments = "advfirewall firewall add rule name=\"Homesys\" dir=in action=allow protocol=TCP localport=24935 profile=private";
                    p.Start();

                    output = p.StandardOutput.ReadToEnd();

                    p.WaitForExit();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e.Message);
            }
        }
    }
}