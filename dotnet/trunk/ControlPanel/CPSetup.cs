using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using MakingThings;

namespace ControlPanel
{
  public partial class CPSetup : Form
  {
    public CPSetup()
    {
      InitializeComponent();

      cpuCounter = new PerformanceCounter();

      cpuCounter.CategoryName = "Processor";
      cpuCounter.CounterName = "% Processor Time";
      cpuCounter.InstanceName = "_Total";

      udpPacket = new UdpPacket();
      udpPacket.RemoteHostName = "192.168.0.200";
      udpPacket.RemotePort = 10000;
      udpPacket.LocalPort = 10000;
      udpPacket.Open();
      oscUdp = new Osc(udpPacket);
    }
   
    private void timer1_Tick(object sender, EventArgs e)
    {
      if (!SpeedSet)
      {
        OscMessage oscMS = new OscMessage();
        oscMS.Address = "/servo/0/speed";
        oscMS.Values.Add((int)200);
        oscUdp.Send(oscMS);
        SpeedSet = true;
      }

      float cpu = getCurrentCpuUsage();
      CPU.Text = cpu.ToString();
      int cpuSpeed = ((int)cpu) * 10;
      if (cpuSpeed != lastCpuSpeed)
      {
        OscMessage oscM = new OscMessage();
        oscM.Address = "/servo/0/position";
        oscM.Values.Add(((int)cpu) * 10);
        oscUdp.Send(oscM);
        lastCpuSpeed = cpuSpeed;
      }
    }

    protected PerformanceCounter cpuCounter;

    public float getCurrentCpuUsage()
    {
      return cpuCounter.NextValue();
    }

    //private UsbPacket usbPacket;
    private UdpPacket udpPacket;

    private Osc oscUdp;
    //private Osc oscUsb;

    private int lastCpuSpeed;
    private bool SpeedSet;
  }
}