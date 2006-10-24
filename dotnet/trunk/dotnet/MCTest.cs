using System;
using System.Windows.Forms;
using System.Threading;
using System.Collections;
using MakingThings;

public class MCTest
{
  public static void Main()
  {
    new MCTest();
  }

  MCTest()
  {
    usbPacket = new UsbPacket();
    usbPacket.Open();

    udpPacket = new UdpPacket();
    udpPacket.Open();
   
    mct = new MCTestForm( this );
    osc = new Osc();

    Running = true;
    if (usbPacket.IsOpen())
    {
      mct.SetUsbPortName(usbPacket.Name);

      usbReadThread = new Thread(UsbRead);
      usbReadThread.Start();
    }

    if (udpPacket.IsOpen())
    {
      udpReadThread = new Thread(UdpRead);
      udpReadThread.Start();
    }

    Application.Run(mct);
  }

  public void UsbRead()
  {
    while ( Running )
    {
      if ( usbPacket.IsOpen() )
      {
        try
        {
          byte[] buffer = new byte[ 1000 ];
          int length = usbPacket.ReceivePacket( buffer );
          if (length > 0)
          {
            Console.WriteLine("  Packet Received");
            Console.Write("  ");
            for (int i = 0; i < length; i++)
            {
              Console.Write(buffer[i]);
              Console.Write(' ');
            }
            Console.WriteLine();
          }
        }
        catch (TimeoutException) { }
      }
      else
      {
        usbPacket.Open();
        if (!usbPacket.IsOpen())
          Thread.Sleep(100);
      }
    }
  }

  public void UdpRead()
  {
    while (Running)
    {
      if (udpPacket.IsOpen())
      {
        try
        {
          byte[] buffer = new byte[1000];
          int length = udpPacket.ReceivePacket(buffer);
          if (length > 0)
          {
            ArrayList messages = osc.PacketToOscMessages(buffer, length);
            foreach (OscMessage om in messages)
            {
              string message = osc.OscMessageToString(om);
              //Console.WriteLine("Received Message: " + message );
              mct.WriteLine("UDP>"+message);
            }
          }
        }
        catch (TimeoutException) { }
      }
      else
      {
        udpPacket.Open();
        if (!udpPacket.IsOpen())
          Thread.Sleep(100);
      }
    }
  }

  public void usbSend(string text)
  {
    if ( !usbPacket.IsOpen() )
      usbPacket.Open();
    if (usbPacket.IsOpen())
    {
      mct.WriteLine("USB<" + text);
      byte[] buffer = new byte[1000];

      OscMessage oscM = osc.StringToOscMessage(text);
      int length = osc.OscMessageToPacket(oscM, buffer, 1000);
      usbPacket.SendPacket(buffer, length);
    }
  }

  public void udpSend(string text)
  {
    if (!udpPacket.IsOpen())
      udpPacket.Open();
    if (udpPacket.IsOpen())
    {
      mct.WriteLine("UDP<" + text );
      byte[] buffer = new byte[1000];

      OscMessage oscM = osc.StringToOscMessage(text);
      int length = osc.OscMessageToPacket(oscM, buffer, 1000);
      udpPacket.SendPacket(buffer, length);
    }
  }

  private bool Running;
  UsbPacket usbPacket;
  UdpPacket udpPacket;

  Thread udpReadThread;
  Thread usbReadThread;

  Osc osc;

  MCTestForm mct;
}

