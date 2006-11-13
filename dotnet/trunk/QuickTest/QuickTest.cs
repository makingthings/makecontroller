using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using MakingThings;

namespace QuickTest
{
  public partial class QuickTest : Form
  {
    public QuickTest()
    {
      InitializeComponent();

      // udpPacket = new UdpPacket();
      // udpPacket.RemoteHostName = "192.168.0.200";
      // udpPacket.RemotePort = 10000;
      // udpPacket.LocalPort = 10000;
      // udpPacket.Open();
      // oscUdp = new Osc(udpPacket);

      usbPacket = new UsbPacket();
      usbPacket.Open();
      osc = new Osc(usbPacket);

      osc.SetAddressHandler("/analogin/7/value", TrimPotReading);
    }

    // This delegate enables asynchronous calls for setting
    // the value property on the Indicator.
    delegate void IndicatorCallback(int value);

    void TrimPotReading(OscMessage oscM)
    {      
      int value = (int)oscM.Values[0];
      SetIndicator( value );
    }

    public void SetIndicator(int value)
    {
      // InvokeRequired required compares the thread ID of the
      // calling thread to the thread ID of the creating thread.
      // If these threads are different, it returns true.
      if (TrimPotIndicator.InvokeRequired)
      {
        IndicatorCallback d = new IndicatorCallback(SetIndicator);
        this.Invoke(d, new object[] { value });
      }
      else
      {
        TrimPotIndicator.Value = value;
      }
    }

    // private UdpPacket udpPacket;
    private UsbPacket usbPacket;

    private Osc osc;

    private void timer1_Tick(object sender, EventArgs e)
    {
      OscMessage oscMS = new OscMessage();
      oscMS.Address = "/analogin/7/value";
      osc.Send(oscMS);
    }

    private void Led0_CheckedChanged(object sender, EventArgs e)
    {
      LedSet(0, Led0.Checked);
    }

    private void Led1_CheckedChanged(object sender, EventArgs e)
    {
      LedSet(1, Led1.Checked);
    }

    private void Led2_CheckedChanged(object sender, EventArgs e)
    {
      LedSet(2, Led2.Checked);
    }

    private void Led3_CheckedChanged(object sender, EventArgs e)
    {
      LedSet(3, Led3.Checked);
    }

    private void LedSet(int index, bool value)
    {
      OscMessage oscMS = new OscMessage();
      oscMS.Address = "/appled/" + index.ToString() + "/state";
      oscMS.Values.Add(value?1:0);
      osc.Send(oscMS);
    }
  }
}