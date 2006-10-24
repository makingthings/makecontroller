using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace MakingThings
{
  public partial class MCTestForm : Form
  {
    public MCTestForm( MCTest mcTest )
    {
      this.mcTest = mcTest;
      InitializeComponent();
    }

    private void UsbSend_Click(object sender, EventArgs e)
    {
      mcTest.usbSend(UsbCommand.Text);
    }

    private void UdpSend_Click(object sender, EventArgs e)
    {
      mcTest.udpSend(UdpCommand.Text);
    }

    private void UdpCommand_KeyUp(object sender, KeyEventArgs e)
    {
      if (e.KeyCode == Keys.Return)
        UdpSend_Click(sender, e);
      e.SuppressKeyPress = true;
    }

    private void UsbCommand_KeyUp(object sender, KeyEventArgs e)
    {
      if (e.KeyCode == Keys.Return)
        UsbSend_Click(sender, e);
      e.SuppressKeyPress = true;
    }

    public void SetUsbPortName( string value )
    {
      UsbPortName.Text = value;
    }

    public void WriteLine(String line)
    {
      // InvokeRequired required compares the thread ID of the
      // calling thread to the thread ID of the creating thread.
      // If these threads are different, it returns true.
      if (Output.InvokeRequired)
      {
        WriteLineCallback d = new WriteLineCallback(WriteLine);
        this.Invoke(d, new object[] { line });
      }
      else
      {
        Output.AppendText(line + "\n");
      }
    }

    MCTest mcTest;

    // This delegate enables asynchronous calls for setting
    // the text property on a TextBox control.
    delegate void WriteLineCallback(string text);

  }
}