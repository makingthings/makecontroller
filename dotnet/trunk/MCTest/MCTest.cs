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
    mct = new MCTestForm(this);

    //usbPacket = new UsbWin32Packet();
    usbPacket = new UsbPacket();
    usbPacket.Open();
    if (usbPacket.IsOpen())
      mct.SetUsbPortName(usbPacket.Name);
    oscUsb = null;
    oscUsb = new Osc(usbPacket);
    oscUsb.SetAllMessageHandler(UsbMessages);

    udpPacket = new UdpPacket();
    udpPacket.RemoteHostName = mct.GetUdpRemoteHostName();
    udpPacket.RemotePort = mct.GetUdpRemotePort();
    udpPacket.LocalPort = mct.GetUdpLocalPort();
    udpPacket.Open();
    oscUdp = new Osc(udpPacket);
    oscUdp.SetAllMessageHandler(UdpMessages);
    oscUdp.SetAddressHandler("/analogin/0/value", AIn0Message);

    Application.Run(mct);
  }

  public void usbSend(string text)
  {
    mct.WriteLine("USB < " + text);
    OscMessage oscM = Osc.StringToOscMessage(text);
    oscUsb.Send(oscM);
  }

  public void udpSend(string text)
  {
    mct.WriteLine("UDP < " + text);
    OscMessage oscM = Osc.StringToOscMessage(text);
    oscUdp.Send(oscM);
  }

  public void UdpMessages(OscMessage oscMessage)
  {
    mct.WriteLine( "UDP > " + Osc.OscMessageToString(oscMessage) );
  }

  public void UsbMessages(OscMessage oscMessage)
  {
    mct.WriteLine("USB > " + Osc.OscMessageToString(oscMessage));
  }

  public void AIn0Message(OscMessage oscMessage)
  {
    mct.WriteLine("AIn0 > " + Osc.OscMessageToString(oscMessage));
  }

  public void SetUdpRemoteHostName(string hostname)
  {
    udpPacket.Close();
    udpPacket.RemoteHostName = hostname;    
    udpPacket.Open();

    mct.WriteLine("Udp < [New Remote Host]" + hostname + "]");
  }

  public void SetUdpRemotePort(int remotePort)
  {
    udpPacket.Close();
    udpPacket.RemotePort = remotePort;
    udpPacket.Open();

    mct.WriteLine("Udp < [New Remote Port]" + remotePort + "]");
  }

  public void SetUdpLocalPort(int localPort)
  {
    udpPacket.Close();
    udpPacket.LocalPort = localPort;
    udpPacket.Open();

    mct.WriteLine("Udp < [New Local Port]" + localPort + "]");
  }

  //private UsbWin32Packet usbPacket;
  private UsbPacket usbPacket;
  private UdpPacket udpPacket;

  private Osc oscUdp;
  private Osc oscUsb;

  MCTestForm mct;
}

