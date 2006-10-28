using System;
using System.IO.Ports;
using System.Collections;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using Microsoft.Win32;

namespace MakingThings
{
  public interface PacketExchange
  {
    bool Open();
    void Close();
    bool IsOpen();
    void SendPacket( byte[] packet, int length );
    int  ReceivePacket( byte[] packet );
  }

  public class UsbPacket : PacketExchange
  {
    public UsbPacket()
    {
      StuffChars = new Byte[4];
      StuffChars[EndIndex] = End;
      StuffChars[EscIndex] = Esc;
      StuffChars[EscEndIndex] = EscEnd;
      StuffChars[EscEscIndex] = EscEsc;
    }

    public bool Open()
    {
      if (Port == null)
      {
        PortName = GetPortName();
        if ( PortName == null)
          return false;
        Port = new SerialPort();
        Port.PortName = PortName;
        try
        {
          Port.Open();
        }
        catch
        {
          Port = null;
          return false;
        }
      }
      return true;
    }

    public void Close()
    {
      if (Port != null)
        Port.Close();
    }

    public bool IsOpen()
    {
      return Port != null;
    }

    public void SendPacket( byte[] packet, int length )
    {
      if (!IsOpen())
        Open();
      if (!IsOpen())
        return;

      Console.WriteLine("SendPacket:");
      Port.Write(StuffChars, EndIndex, 1);
      Console.WriteLine( "  End" ); 
      for (int i = 0; i < length; i++)
      {
        int c = packet[i];
        if (c == End)
        {
          Console.WriteLine("  Esc");
          Port.Write(StuffChars, EscIndex, 1);
          Console.WriteLine("  EscEnd");
          Port.Write(StuffChars, EscEndIndex, 1);
        }
        else
        {
          if (c == Esc)
          {
            Console.WriteLine("  Esc");
            Port.Write(StuffChars, EscIndex, 1);
            Console.WriteLine("  EscEsc");
            Port.Write(StuffChars, EscEscIndex, 1);
          }
          else
          {
            Console.WriteLine("  " + c);
            Port.Write(packet, i, 1);
          } 
        }
      }
      Port.Write(StuffChars, EndIndex, 1);
    }

    public int ReceivePacket( byte[] buffer )
    {
      int index = 0;
      // Skip until there's an End character
      while (Port.ReadByte() != End)
        ;
      // Now we have a End we can start getting actual chars
      int c;
      bool escaped = false;
      do
      {
        c = Port.ReadByte();
        if (c != End)
        {
          if (escaped)
          {
            if (c == EscEnd)
              buffer[index++] = End;
            if (c == EscEsc)
              buffer[index++] = Esc;
            escaped = false;
          }
          else
          {
            if (c == Esc)
              escaped = true;
            else
              buffer[index++] = (byte)c;
          }
        }
      } while (c != End);

      return index;
    }

    private string GetPortName()
    {
      RegistryKey rk = Registry.LocalMachine.OpenSubKey(RegistryLocalMachinePath);
      string[] topNames = rk.GetSubKeyNames();
      // Print the contents of the array to the console.
      foreach (string ts in topNames )
      {
        // Console.WriteLine("Top: " + ts);
        try
        {
          RegistryKey srk = rk.OpenSubKey( ts );

          string[] subNames = srk.GetSubKeyNames();
          foreach (string ss in subNames)
          {
            RegistryKey trk = srk.OpenSubKey(ss);

            if ( (string)trk.GetValue("Class", "<none>") == "Ports" &&
                 (string)trk.GetValue("ClassGUID", "<none>") == MakingThingsUsbGuid &&
                 (string)trk.GetValue("DeviceDesc", "<none>") == MakingThingsUsbDesc )
            {
              Console.WriteLine("Usb: " + ss);
              Console.WriteLine("  " + trk.GetValue("FriendlyName", "<none>"));
              RegistryKey prk = trk.OpenSubKey("Device Parameters");

              string portName = (string)prk.GetValue("PortName", "<none>");
              Console.WriteLine("    " + portName);

              SerialPort port = new SerialPort();
              port.PortName = portName;

              // If this doesn't work, there will be an exception, taking us away
              try
              {
                port.Open();

                Thread.Sleep(1000);

                // It did work.  This is us.  Return.
                port.Close();

                return portName;
              }
              catch
              {
                Console.WriteLine("    Open Exception" );
              }
            }
          }
        }
        catch (System.Exception)
        {
        }
      }
      return null;
    }
    private const string RegistryLocalMachinePath = "SYSTEM\\CURRENTCONTROLSET\\ENUM\\USB";
    private const string MakingThingsUsbGuid = "{4D36E978-E325-11CE-BFC1-08002BE10318}";
    private const string MakingThingsUsbDesc = "Make Controller Kit";

    // Byte stuffing 
    // ... characters
    private const int End = 192; 
    private const int Esc = 219; 
    private const int EscEnd = 220; 
    private const int EscEsc = 221; 
    // ... indicies into the little byte array
    private const int EndIndex = 0;
    private const int EscIndex = 1;
    private const int EscEndIndex = 2;
    private const int EscEscIndex = 3;
      
    Byte[] StuffChars;

    private SerialPort Port;
    private string PortName;
    public string Name
    {
      get { return PortName; }
    }
  }

  public class UdpPacket : PacketExchange
  {
    public UdpPacket()
    {
      ToHostName = "192.168.0.200";
      ToPort = 10000;
      IncomingPort = 10000;
      socketsOpen = false;
    }

    public bool Open()
    {
      Sender = new UdpClient();
      Receiver = new UdpClient(localPort);
      socketsOpen = true;
      return true;
    }

    public void Close()
    {
      Sender.Close();
      Receiver.Close();
      socketsOpen = false;
    }

    public bool IsOpen()
    {
      return socketsOpen;
    }

    public void SendPacket(byte[] packet, int length)
    {
      Sender.Send(packet, length, remoteHostName, remotePort);
    }

    public int ReceivePacket(byte[] buffer)
    {
      IPEndPoint iep = new IPEndPoint(IPAddress.Any, 0);
      byte[] incoming = Receiver.Receive( ref iep );
      int count = Math.Min(buffer.Length, incoming.Length);
      System.Array.Copy(incoming, buffer, count);
      return count;
    }

    private UdpClient Sender;
    private UdpClient Receiver;
    private bool socketsOpen;
    private string remoteHostName;
    private int remotePort;
    private int localPort;

    public string ToHostName
    {
      get
      { 
        return remoteHostName; 
      }
      set
      { 
        remoteHostName = value; 
      }
    }
  
    private int ToPort
    {
      get
      { return remotePort; }
      set
      { remotePort = value; }
    }

    private int IncomingPort
    {
      get
      {
        return localPort; 
      }
      set
      { 
        localPort = value; 
      }
    }
  }
}