using NLog;
using System;
using System.Diagnostics;
using System.Security.AccessControl;
using System.Threading;

namespace Runner
{
    public class Program
    {
        static ILogger _logger = LogManager.GetCurrentClassLogger();
        
        static void DoTrace(int loops, int len)
        {
            int pid = Process.GetCurrentProcess().Id;
            string line = new string('X', len);
            string chars = string.Format("{0,-5} " + line, pid);

            for (int i = 0; i < loops; ++i)
                _logger.Info(chars);
            
            LogManager.Flush();

            //Thread.Sleep(1000);
        }
        static int Main(string[] args)
        {
            //for (int i = 0; i < 1000; ++i)
            //    Thread.Sleep(1000);

            if (args.Length != 2)
            {
                return 1;
            }

            try
            {
                var ev = EventWaitHandle.OpenExisting("HelloTestFileAppendRunner", EventWaitHandleRights.Synchronize);
                ev.WaitOne();
                DoTrace(int.Parse(args[0]), int.Parse(args[1]));
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "WTF");
                Debugger.Break();
            }
            return 0;
        }
    }
}
