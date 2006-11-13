using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using MakingThings;

namespace DeskTimer
{
  public partial class DeskTimerForm : Form
  {
    public DeskTimerForm()
    {
      InitializeComponent();

      // udpPacket = new UdpPacket();
      // udpPacket.RemoteHostName = "192.168.0.200";
      // udpPacket.RemotePort = 10000;
      // udpPacket.LocalPort = 10000;
      // udpPacket.Open();
      // osc = new Osc(udpPacket);

      usbPacket = new UsbPacket();
      usbPacket.Open();
      osc = new Osc(usbPacket);

      Working = true;
      ResetTimers();

      osc.SetAddressHandler("/analogin/0/value", SensorReading);
    }

    void SensorReading(OscMessage oscM)
    {
      int value = (int)oscM.Values[0];
      if (value < 300)
        AwayCount++;
      else
        AwayCount = 0;

      if (AwayCount > 5)
        Working = false;
      else
        Working = true;
    }

    private void Reset_Click(object sender, EventArgs e)
    {
      ResetTimers();
    }

    private void ResetTimers()
    {
      WorkingTime = 0;
      SetTime(WorkingText, WorkingTime);
      AwayTime = 0;
      SetTime(AwayText, AwayTime);
    }

    private void Timer_Tick(object sender, EventArgs e)
    {
      if (Working)
      {
        WorkingTime += 1.0F;
        SetTime(WorkingText, WorkingTime);
      }
      else
      {
        AwayTime += 1.0F;
        SetTime(AwayText, AwayTime);
      }

      OscMessage oscM = new OscMessage();
      oscM.Address = "/analogin/0/value";
      osc.Send(oscM);
    }

    private void SetTime(System.Windows.Forms.TextBox textbox, float t)
    {
      int totalSeconds = (int)t;
      int totalMinutes = totalSeconds / 60;
      int hours = totalMinutes / 60;
      int minutes = totalMinutes % 60;
      int seconds = totalSeconds % 60;
      textbox.Text = String.Format( "{0:D2}:{1:D2}:{2:D2}", hours, minutes, seconds );
    }

    // private UdpPacket udpPacket;
    private UsbPacket usbPacket;
    private Osc osc;

    bool Working;
    int AwayCount;
    float WorkingTime;
    float AwayTime;
  }
}