using System;
using System.IO.Ports;
using System.Threading;
using Microsoft.Win32;

namespace MakingThings
{
    /// <summary>
    /// This is the PortChat class.
    /// </summary>
    /// <remarks>
    /// Some more info about PortChat
    /// </remarks>
    public class PortChat
    {
        static bool _continue;
        static SerialPort _serialPort;

        /// <summary>
        /// This is the zMain method.
        /// </summary>
        /// <remarks>
        /// Some more info about zMain.
        /// </remarks>
        public static void zMain()
        {
            string name;
            string message;
            StringComparer stringComparer = StringComparer.OrdinalIgnoreCase;
            Thread readThread = new Thread(Read);

            // Create a RegistryKey, which will access the HKEY_USERS
            // key in the registry of this machine.
            RegistryKey rk = Registry.LocalMachine;
            RegistryKey sk = rk.OpenSubKey("SYSTEM\\CURRENTCONTROLSET\\ENUM\\USB");

            // Print out the keys.
            // UPrintKeys("  ", sk);

            // Create a new SerialPort object with default settings.
            _serialPort = new SerialPort( "COM6" );

            // Set the read/write timeouts
            _serialPort.ReadTimeout = 1000;
            _serialPort.WriteTimeout = 1000;

            _serialPort.Open();
            _continue = true;
            readThread.Start();


            Console.Write("Name: ");
            name = Console.ReadLine();

            Console.WriteLine("Type QUIT to exit");

            while (_continue)
            {
                message = Console.ReadLine();

                if (stringComparer.Equals("quit", message))
                {
                    _continue = false;
                }
                else
                {
                  try
                  {
                    _serialPort.WriteLine(
                      String.Format("<{0}>: {1}", name, message));
                  }
                  catch
                  {
                    Console.WriteLine("Write Timeout");
                  }
                }
            }

            readThread.Join();
            _serialPort.Close();
        }
        /// <summary>
        /// The PortChat.Read() method reads from the serial port and prints out to the console.
        /// </summary>
        public static void Read()
        {
            while (_continue)
            {
                try
                {
                    string message = _serialPort.ReadLine();
                    Console.WriteLine(message);
                }
                catch (TimeoutException) 
                {
                  Console.WriteLine("Read Timeout");
                }
            }
        }

        /// <summary>
        /// PortChat.PrintKeys() prints the names of entries in the registry that correspond to a given key.
        /// </summary>
        /// <param name="indent"></param>
        /// <param name="rkey">The key to look for.</param>
        static void PrintKeys(string indent, RegistryKey rkey)
        {
            Console.WriteLine(indent + rkey.Name);

            // Get the values
            String[] values = rkey.GetValueNames();
            foreach (String v in values)
            {
                Console.WriteLine(indent + v + ":" + rkey.GetValue(v, "<none>"));
            }


            // Retrieve all the subkeys for the specified key.
            String[] names = rkey.GetSubKeyNames();

            int icount = 0;

            Console.WriteLine(indent + "Subkeys of " + rkey.Name);

            // Print the contents of the array to the console.
            foreach (String s in names)
            {
                try
                {
                    RegistryKey k = rkey.OpenSubKey(s);
                    PrintKeys(indent + "  ", k);
                }
                catch (System.Exception)
                {
                    Console.WriteLine(indent + s + ": Can't descend");
                }

                // The following code puts a limit on the number
                // of keys displayed.  Comment it out to print the
                // complete list.
                icount++;
                if (icount >= 10)
                    break;
            }
            Console.WriteLine(indent + "-----------------------------------------------");

        }

    }
}