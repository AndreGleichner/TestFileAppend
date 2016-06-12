using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace TestFileAppendManaged
{
    class Program
    {
        static void Main(string[] args)
        {
            const int processCount = 100;
            const int loops = 100000;
            const int charCount = 42;

            string logFileName = Path.Combine(
                Path.GetDirectoryName(Assembly.GetEntryAssembly().Location),
                "log.txt");

            File.Delete(logFileName);

            string runnerExe = typeof(Runner.Program).Assembly.Location;
            string param = loops + " " + charCount;

            Console.WriteLine("Launching " + processCount + " processes each writing " + loops + " log events with " + charCount + " characters.");

            var sw = new Stopwatch();
            sw.Start();

            using (var ev = new EventWaitHandle(false, EventResetMode.ManualReset, "HelloTestFileAppendRunner"))
            {
                var si = new ProcessStartInfo(runnerExe, param);
                si.CreateNoWindow = true;
                si.WindowStyle = ProcessWindowStyle.Hidden;

                var processes = new Process[processCount];
                for (int i = 0; i < processCount; i++)
                {
                    processes[i] = Process.Start(si);
                    Console.Write("+");
                }

                Console.WriteLine("\nGO");
                // Synchronous start of all processes, as otherwise there's often not much concurrency.
                // Because before the 2nd process starts, the 1st often already has done his job.
                ev.Set();

                foreach (var process in processes)
                {
                    process.WaitForExit();
                    process.Dispose();
                    Console.Write("-");
                }
            }

            sw.Stop();

            Console.WriteLine("\nDuration: " + sw.Elapsed);
            
            DoVerify(logFileName, processCount, loops, charCount);

            Console.WriteLine("Press RETURN...");

            Console.ReadLine();
        }

        static long _pos;
        static void AssertChar(Stream stream, char c)
        {
            _pos = stream.Position;
            int theChar = stream.ReadByte();
            if (theChar == -1)
                throw new InvalidDataException("Can't read a '" + c + "' char");
            if (theChar != c)
                throw new InvalidDataException("Should have read a '" + c + "' char, but read '" + theChar + "'");
        }

        static byte[] _buf;
        static void AssertChars(Stream stream, char c, int count)
        {
            if (_buf == null)
                _buf = new byte[count];
            else if (_buf.Length < count)
                _buf = new byte[count];

            _pos = stream.Position;
            int read = stream.Read(_buf, 0, count);
            if (read != count)
                throw new InvalidDataException("Can't read " + count + " '" + c + "' chars: " + read);

            for (int i = 0; i < count; ++i)
            {
                if (_buf[i] != c)
                    throw new InvalidDataException("Should have read a '" + c + "' char, but read '" + _buf[i] + "'");
            }
        }

        static void DoVerify(string logFileName, int processCount, int loops, int charCount)
        {
            if (!File.Exists(logFileName))
                return;

            Console.WriteLine("Verifying...");

            var pids = new Dictionary<int, int>();
            var processLines = new int[processCount];
            byte[] pidBuf = new byte[5];
            byte[] charsBuf = new byte[charCount];
            FileInfo fi = new FileInfo(logFileName);

            using (var fileMap = MemoryMappedFile.CreateFromFile(logFileName, FileMode.Open, "log", fi.Length, MemoryMappedFileAccess.Read))
            using (var stream = fileMap.CreateViewStream(0, fi.Length, MemoryMappedFileAccess.Read))
            {
                try
                {
                    while (stream.Position < stream.Length)
                    {
                        _pos = stream.Position;

                        int read = stream.Read(pidBuf, 0, 5);
                        if (read != 5)
                            throw new InvalidDataException("Can't read PID");

                        int pid = int.Parse(Encoding.UTF8.GetString(pidBuf));

                        if (!pids.ContainsKey(pid))
                        {
                            if (pids.Count == processCount)
                                throw new InvalidDataException("Already reached " + processCount + " PIDs");
                            pids.Add(pid, 0);
                        }

                        pids[pid]++;

                        AssertChar(stream, ' ');
                        AssertChars(stream, 'X', charCount);
                        AssertChar(stream, '\r');
                        AssertChar(stream, '\n');
                    }

                    if (stream.Position != processCount * loops * (charCount + 8))
                        throw new InvalidDataException("File with wrong length " + stream.Position);

                    if (pids.Count != processCount)
                        throw new InvalidDataException("Wrong # processes: " + pids.Count);

                    foreach (var p in pids)
                    {
                        if (p.Value != loops)
                            throw new InvalidDataException("PID " + p.Key + " has wrong loops: " + p.Value);
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Verify failed at offset {0}: {1}", _pos, ex);
                    return;
                }
            }

            Console.WriteLine("Well done :)");
        }
    }
}
