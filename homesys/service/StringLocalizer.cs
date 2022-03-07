using System;
using System.Collections.Generic;
using System.Text;
using System.Globalization;
using Microsoft.Win32;

namespace Homesys
{
    public enum StringLocalizerType
    {
        RecordingStarted,
        RecordingEnded,
        NewVersionAvailable,
        IncomingRecording,
        RecordingCollision,
        GuideUpdate,
        GuideUpdateStep,
        GuideUpdateDone,
        GuideUpdateImport,
        ManualRecording,
        NewPresetName,
        MyStations,
        ScanAllFrequencies,
        DiskSpaceLow
    }

    public class StringLocalizer
    {
        Dictionary<StringLocalizerType, string> _eng = new Dictionary<StringLocalizerType, string>();
        Dictionary<StringLocalizerType, string> _hun = new Dictionary<StringLocalizerType, string>();
        Dictionary<StringLocalizerType, string> _deu = new Dictionary<StringLocalizerType, string>();
        Dictionary<StringLocalizerType, string> _current;
        string _lang = null;
        object _sync = new object();

        public StringLocalizer()
        {
            Language = Language;

            _eng[StringLocalizerType.RecordingStarted] = "Recording started";
            _hun[StringLocalizerType.RecordingStarted] = "Időzített felvétel kezdődött";

            _eng[StringLocalizerType.RecordingEnded] = "Recording ended";
            _hun[StringLocalizerType.RecordingEnded] = "Időzített felvétel befejeződött";

            _eng[StringLocalizerType.NewVersionAvailable] = "Software update available!";
            _hun[StringLocalizerType.NewVersionAvailable] = "Új verzió letölthető!";

            _eng[StringLocalizerType.IncomingRecording] = "Incoming scheduled recording ({0})";
            _hun[StringLocalizerType.IncomingRecording] = "Felvétel időzítés érkezett ({0} db)";

            _eng[StringLocalizerType.RecordingCollision] = "Recording collision ({0})";
            _hun[StringLocalizerType.RecordingCollision] = "Felvétel ütközés ({0} db)";

            _eng[StringLocalizerType.GuideUpdate] = "Updating EPG";
            _hun[StringLocalizerType.GuideUpdate] = "Műsorfrissítés (letöltés)";

            _eng[StringLocalizerType.GuideUpdateStep] = "Updating EPG ({0}/{1})";
            _hun[StringLocalizerType.GuideUpdateStep] = "Műsorfrissítés (letöltés {0}/{1})";

            _eng[StringLocalizerType.GuideUpdateDone] = "Updating EPG completed";
            _hun[StringLocalizerType.GuideUpdateDone] = "Műsorfrissítés (kész)";

            _eng[StringLocalizerType.GuideUpdateImport] = "Importing EPG";
            _hun[StringLocalizerType.GuideUpdateImport] = "Műsorfrissítés (import)";

            _eng[StringLocalizerType.ManualRecording] = "Manual recording";
            _hun[StringLocalizerType.ManualRecording] = "Kézi felvétel";

            _eng[StringLocalizerType.NewPresetName] = "New preset";
            _hun[StringLocalizerType.NewPresetName] = "Új csatorna";

            _eng[StringLocalizerType.MyStations] = "My Links";
            _hun[StringLocalizerType.MyStations] = "Saját";

            _eng[StringLocalizerType.ScanAllFrequencies] = "Scan all frequencies";
            _hun[StringLocalizerType.ScanAllFrequencies] = "Keresés minden frekvencián";

            _eng[StringLocalizerType.DiskSpaceLow] = "Disk space low";
            _hun[StringLocalizerType.DiskSpaceLow] = "Kevés a hely a merevlemezen";

            _deu[StringLocalizerType.RecordingStarted] = "Zeitgesteuerte Aufnahme begonnen"; 
            _deu[StringLocalizerType.RecordingEnded] = "Zeitgesteuerte Aufnahme beendet"; 
            _deu[StringLocalizerType.NewVersionAvailable] = "Neue Version abladar!"; 
            _deu[StringLocalizerType.IncomingRecording] = "Aufnahme Zeitpunkt ist eingegangen ({0} db)";
            _deu[StringLocalizerType.RecordingCollision] = "Aufnahmekollision ({0} db)";
            _deu[StringLocalizerType.GuideUpdate] = "Programmerneuerung(laden)";
            _deu[StringLocalizerType.GuideUpdateStep] = "Programmerneuerung (laden {0}/{1})";
            _deu[StringLocalizerType.GuideUpdateDone] = "Programmerneuerung (fertig)";
            _deu[StringLocalizerType.GuideUpdateImport] = "Programmerneuerung (import)";
            _deu[StringLocalizerType.ManualRecording] = "Manuelle Aufnahme";
            _deu[StringLocalizerType.NewPresetName] = "Neuer Kanal";
            _deu[StringLocalizerType.MyStations] = "Eigen";
            _deu[StringLocalizerType.ScanAllFrequencies] = "Suchlauf auf allen Frequenzen";
            _deu[StringLocalizerType.DiskSpaceLow] = "Zu wenig Platz auf der Festplatte";
        }

        public string Language
        {
            get
            {
                lock(_sync)
                {
                    try
                    {
                        if(_lang == null)
                        {
                            using(RegistryKey key = Registry.LocalMachine.OpenSubKey("Software\\Homesys"))
                            {
                                _lang = (string)key.GetValue("Language");
                            }

                            if(_lang == null)
                            {
                                _lang = CultureInfo.InstalledUICulture.ThreeLetterISOLanguageName;
                            }
                        }
                    }
                    catch(Exception e)
                    {
                        Log.WriteLine(e.Message);
                    }

                    return _lang;
                }
            }

            set
            {
                lock(_sync)
                {
                    try
                    {
                        using(RegistryKey key = Registry.LocalMachine.CreateSubKey("Software\\Homesys"))
                        {
                            key.SetValue("Language", value);
                        }

                        switch(value)
                        {
                            case "hun": _current = _hun; break;
                            case "deu": _current = _deu; break;
                            default: _current = _eng; break;
                        }

                        _lang = value;
                    }
                    catch(Exception e)
                    {
                        Log.WriteLine(e);
                    }
                }
            }
        }

        public string this[StringLocalizerType index]
        {
            get
            {
                lock(_sync)
                {
                    string s;

                    if(_current.TryGetValue((StringLocalizerType)index, out s))
                    {
                        return s;
                    }
                    else if(_eng.TryGetValue((StringLocalizerType)index, out s))
                    {
                        return s;
                    }

                    return "";
                }
            }
        }
    }
}
