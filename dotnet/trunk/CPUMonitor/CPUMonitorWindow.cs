using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using MakingThings;

namespace CPUMonitor
{
  public partial class CPUMonitorWindow : Form
  {
    public CPUMonitorWindow()
    {
      InitializeComponent();

      cpuCounter = new PerformanceCounter();

      cpuCounter.CategoryName = "Processor";
      cpuCounter.CounterName = "% Processor Time";
      cpuCounter.InstanceName = "_Total";

      usbPacket = new UsbPacket();
      usbPacket.Open();
      osc = new Osc(usbPacket);

      // udpPacket = new UdpPacket();
      // udpPacket.RemoteHostName = "192.168.0.200";
      // udpPacket.RemotePort = 10000;
      // udpPacket.LocalPort = 10000;
      // udpPacket.Open();
      // osc = new Osc(udpPacket);
    }
   
    private void timer1_Tick(object sender, EventArgs e)
    {
      if (!SpeedSet)
      {
        OscMessage oscMS = new OscMessage();
        oscMS.Address = "/servo/0/speed";
        oscMS.Values.Add((int)200);
        osc.Send(oscMS);
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
        osc.Send(oscM);
        lastCpuSpeed = cpuSpeed;
      }
    }

    protected PerformanceCounter cpuCounter;

    public float getCurrentCpuUsage()
    {
      return cpuCounter.NextValue();
    }

    private UsbPacket usbPacket;
    //private UdpPacket udpPacket;
    private Osc osc;

    private int lastCpuSpeed;
    private bool SpeedSet;
  }
}