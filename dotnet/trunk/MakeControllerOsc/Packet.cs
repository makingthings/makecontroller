using System;
using System.IO.Ports;
using System.Collections;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using Microsoft.Win32;

namespace MakingThings
{
  public interface PacketIO
  {
    bool Open();
    void Close();
    bool IsOpen();
    void SendPacket( byte[] packet, int length );
    int  ReceivePacket( byte[] packet );
  }

  public abstract class UsbPacketBase: PacketIO
  {
    protected UsbPacketBase()
    {
      StuffChars = new Byte[4];
      StuffChars[EndIndex] = End;
      StuffChars[EscIndex] = Esc;
      StuffChars[EscEndIndex] = EscEnd;
      StuffChars[EscEscIndex] = EscEsc;
    }

    /// <summary>
    /// Open the serial port that the board is connected to.
    /// </summary>
    /// <returns>True on success, false on fail.</returns>
    public bool Open()
    {
      PortName = GetPortName();
      //PortName = "COM1";
      if (PortName == null)
        return false;
      return PortOpen( PortName );
    }

    /// <summary>
    /// Close the serial port that the board is currently connected to.
    /// If the port is not already open, this has no effect.
    /// </summary>
    public void Close()
    {
      PortClose( );
    }

    /// <summary>
    /// Query the open state of the serial port.
    /// </summary>
    /// <returns>True if the port is open, false if it is not.</returns>
    public bool IsOpen()
    {
      return PortIsOpen();
    }

    /// <summary>
    /// Send a packet of bytes out over the serial port.
    /// </summary>
    /// <param name="packet">The byte array to be sent.</param>
    /// <param name="length">The length of the byte array to be sent.</param>
    public void SendPacket(byte[] packet, int length)
    {
      if (!IsOpen())
        Open();
      if (!IsOpen())
        return;

      //Console.WriteLine("SendPacket:");
      PortWrite(StuffChars, EndIndex, 1);
      //Console.WriteLine("  End");
      for (int i = 0; i < length; i++)
      {
        int c = packet[i];
        if (c == End)
        {
          //Console.WriteLine("  Esc");
          PortWrite(StuffChars, EscIndex, 1);
          //Console.WriteLine("  EscEnd");
          PortWrite(StuffChars, EscEndIndex, 1);
        }
        else
        {
          if (c == Esc)
          {
            //Console.WriteLine("  Esc");
            PortWrite(StuffChars, EscIndex, 1);
            //Console.WriteLine("  EscEsc");
            PortWrite(StuffChars, EscEscIndex, 1);
          }
          else
          {
            //Console.WriteLine("  " + c);
            PortWrite(packet, i, 1);
          }
        }
      }
      PortWrite(StuffChars, EndIndex, 1);
      //Console.WriteLine("  End");
    }

    /// <summary>
    /// Receive a packet of bytes from the serial port.
    /// </summary>
    /// <param name="buffer">The buffer to be read into.</param>
    /// <returns>Returns the number of bytes read, or 0 on failure.</returns>
    public int ReceivePacket(byte[] buffer)
    {
      if (!IsOpen())
        Open();
      if (!IsOpen())
        return 0;

      int index = 0;
      // Skip until there's an End character
      while (PortReadByte() != End)
        ;
      // Now we have a End we can start getting actual chars
      int c;
      bool escaped = false;
      do
      {
        c = PortReadByte();
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

    /// <summary>
    /// Read the name and parameters of the serial port that the board is connected to.
    /// 
    /// Prints the name and parameters to the Console.
    /// </summary>
    /// <returns>The COM port name the board is connected to.</returns>
    private string GetPortName()
    {
      RegistryKey rk = Registry.LocalMachine.OpenSubKey(RegistryLocalMachinePath);
      string[] topNames = rk.GetSubKeyNames();
      // Print the contents of the array to the console.
      foreach (string ts in topNames)
      {
        // Console.WriteLine("Top: " + ts);
        try
        {
          RegistryKey srk = rk.OpenSubKey(ts);

          string[] subNames = srk.GetSubKeyNames();
          foreach (string ss in subNames)
          {
            RegistryKey trk = srk.OpenSubKey(ss);

            if ((string)trk.GetValue("Class", "<none>") == "Ports" &&
                 (string)trk.GetValue("ClassGUID", "<none>") == MakingThingsUsbGuid &&
                 (string)trk.GetValue("DeviceDesc", "<none>") == MakingThingsUsbDesc)
            {
              Console.WriteLine("Usb: " + ss);
              Console.WriteLine("  " + trk.GetValue("FriendlyName", "<none>"));
              RegistryKey prk = trk.OpenSubKey("Device Parameters");

              string portName = (string)prk.GetValue("PortName", "<none>");
              Console.WriteLine("    " + portName);

              // If this doesn't work, there will be an exception, taking us away
              try
              {
                if (PortOpen(portName))
                {
                  Thread.Sleep(1000);
                  // It did work.  This is us.  Return.
                  PortClose();
                  return portName;
                }
              }
              catch
              {
                Console.WriteLine("    Open Exception");
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

    private string PortName;
    public string Name
    {
      get { return PortName; }
    }

    abstract public bool PortOpen(string portName);
    abstract public void PortClose();
    abstract public bool PortIsOpen();
    abstract public int  PortReadByte();
    abstract public int  PortWrite(byte[] buffer, int index, int count); 
  }

  public class UsbPacket : UsbPacketBase
  {
    public override bool PortOpen( string portName )
    {
      if ( Port == null )
      {
        Port = new SerialPort();
        Port.PortName = portName;
        try
        {
          Port.Open();
          Port.DtrEnable = true;
        }
        catch
        {
          Port = null;
          return false;
        }
      }
      return true;
    }

    public override void PortClose()
    {
      if (Port != null)
      {
        Port.Close();
        Port = null;
      }
    }

    public override bool PortIsOpen()
    {
      return (Port != null);
    }

    public override int PortReadByte()
    {
      int c;
      c = Port.ReadByte();
      //Console.WriteLine("R  " + c);
      return c;
    }

    public override int PortWrite(byte[] buffer, int index, int count)
    {
      if (Port != null)
      {
        Port.Write(buffer, index, count);
        return count;
      }
      else
        return 0;
    }

    private SerialPort Port;
  }

  /// <summary>
  /// UdpPacket provides packetIO over UDP
  /// </summary>
  public class UdpPacket : PacketIO
  {
    public UdpPacket()
    {
      RemoteHostName = "192.168.0.200";
      RemotePort = 10000;
      LocalPort = 10000;
      socketsOpen = false;
    }

    ~UdpPacket()
    {
      if (IsOpen())
        Close();
    }

    /// <summary>
    /// Open a UDP socket and create a UDP sender.
    /// 
    /// The default values with which a UdpPacket is created are:
    /// - Address of the board to send to - 192.168.0.200
    /// - Remote port to send to - 10000
    /// - Local port to listen on - 10000
    /// </summary>
    /// <returns>True on success, false on failure.</returns>
    public bool Open()
    {
      try
      {
        Sender = new UdpClient();
        Receiver = new UdpClient(localPort);
        socketsOpen = true;
        return true;
      }
      catch
      {
      }
      return false;
    }

    /// <summary>
    /// Close the socket currently listening, and destroy the UDP sender device.
    /// </summary>
    public void Close()
    {
      Sender.Close();
      Receiver.Close();
      socketsOpen = false;
    }

    /// <summary>
    /// Query the open state of the UDP socket.
    /// </summary>
    /// <returns>True if open, false if closed.</returns>
    public bool IsOpen()
    {
      return socketsOpen;
    }

    /// <summary>
    /// Send a packet of bytes out via UDP.
    /// </summary>
    /// <param name="packet">The packet of bytes to be sent.</param>
    /// <param name="length">The length of the packet of bytes to be sent.</param>
    public void SendPacket(byte[] packet, int length)
    {
      if (!IsOpen())
        Open();
      if (!IsOpen())
        return; 
      
      Sender.Send(packet, length, remoteHostName, remotePort);
    }

    /// <summary>
    /// Receive a packet of bytes over UDP.
    /// </summary>
    /// <param name="buffer">The buffer to be read into.</param>
    /// <returns>The number of bytes read, or 0 on failure.</returns>
    public int ReceivePacket(byte[] buffer)
    {
      if (!IsOpen())
        Open();
      if (!IsOpen())
        return 0;

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

    /// <summary>
    /// The address of the board that you're sending to.
    /// </summary>
    public string RemoteHostName
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
  
    /// <summary>
    /// The remote port that you're sending to.
    /// </summary>
    public int RemotePort
    {
      get
      { 
        return remotePort; 
      }
      set
      { 
        remotePort = value; 
      }
    }

    /// <summary>
    /// The local port you're listening on.
    /// </summary>
    public int LocalPort
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