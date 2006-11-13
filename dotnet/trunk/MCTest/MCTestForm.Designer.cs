namespace MakingThings
{
    partial class MCTestForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
          this.InputPanes = new System.Windows.Forms.TabControl();
          this.Usb = new System.Windows.Forms.TabPage();
          this.label6 = new System.Windows.Forms.Label();
          this.Command = new System.Windows.Forms.Label();
          this.UsbSend = new System.Windows.Forms.Button();
          this.UsbCommand = new System.Windows.Forms.TextBox();
          this.UsbPortName = new System.Windows.Forms.TextBox();
          this.Udp = new System.Windows.Forms.TabPage();
          this.UdpCommand = new System.Windows.Forms.TextBox();
          this.UdpSend = new System.Windows.Forms.Button();
          this.label5 = new System.Windows.Forms.Label();
          this.UdpLocalPort = new System.Windows.Forms.TextBox();
          this.UdpRemotePort = new System.Windows.Forms.TextBox();
          this.UdpRemoteHostName = new System.Windows.Forms.TextBox();
          this.label4 = new System.Windows.Forms.Label();
          this.label3 = new System.Windows.Forms.Label();
          this.label2 = new System.Windows.Forms.Label();
          this.Output = new System.Windows.Forms.RichTextBox();
          this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
          this.InputPanes.SuspendLayout();
          this.Usb.SuspendLayout();
          this.Udp.SuspendLayout();
          this.tableLayoutPanel1.SuspendLayout();
          this.SuspendLayout();
          // 
          // InputPanes
          // 
          this.InputPanes.Controls.Add(this.Usb);
          this.InputPanes.Controls.Add(this.Udp);
          this.InputPanes.Location = new System.Drawing.Point(3, 31);
          this.InputPanes.Name = "InputPanes";
          this.InputPanes.SelectedIndex = 0;
          this.InputPanes.Size = new System.Drawing.Size(632, 136);
          this.InputPanes.TabIndex = 0;
          // 
          // Usb
          // 
          this.Usb.Controls.Add(this.label6);
          this.Usb.Controls.Add(this.Command);
          this.Usb.Controls.Add(this.UsbSend);
          this.Usb.Controls.Add(this.UsbCommand);
          this.Usb.Controls.Add(this.UsbPortName);
          this.Usb.Location = new System.Drawing.Point(4, 25);
          this.Usb.Name = "Usb";
          this.Usb.Padding = new System.Windows.Forms.Padding(3);
          this.Usb.Size = new System.Drawing.Size(624, 107);
          this.Usb.TabIndex = 0;
          this.Usb.Text = "USB";
          this.Usb.UseVisualStyleBackColor = true;
          // 
          // label6
          // 
          this.label6.AutoSize = true;
          this.label6.Location = new System.Drawing.Point(10, 20);
          this.label6.Name = "label6";
          this.label6.Size = new System.Drawing.Size(34, 17);
          this.label6.TabIndex = 4;
          this.label6.Text = "Port";
          // 
          // Command
          // 
          this.Command.AutoSize = true;
          this.Command.Location = new System.Drawing.Point(10, 55);
          this.Command.Name = "Command";
          this.Command.Size = new System.Drawing.Size(71, 17);
          this.Command.TabIndex = 3;
          this.Command.Text = "Command";
          // 
          // UsbSend
          // 
          this.UsbSend.Location = new System.Drawing.Point(525, 54);
          this.UsbSend.Name = "UsbSend";
          this.UsbSend.Size = new System.Drawing.Size(75, 23);
          this.UsbSend.TabIndex = 2;
          this.UsbSend.Text = "Send";
          this.UsbSend.UseVisualStyleBackColor = true;
          this.UsbSend.Click += new System.EventHandler(this.UsbSend_Click);
          // 
          // UsbCommand
          // 
          this.UsbCommand.Location = new System.Drawing.Point(90, 55);
          this.UsbCommand.Name = "UsbCommand";
          this.UsbCommand.Size = new System.Drawing.Size(425, 22);
          this.UsbCommand.TabIndex = 1;
          this.UsbCommand.KeyUp += new System.Windows.Forms.KeyEventHandler(this.UsbCommand_KeyUp);
          // 
          // UsbPortName
          // 
          this.UsbPortName.Location = new System.Drawing.Point(90, 20);
          this.UsbPortName.Name = "UsbPortName";
          this.UsbPortName.Size = new System.Drawing.Size(100, 22);
          this.UsbPortName.TabIndex = 0;
          // 
          // Udp
          // 
          this.Udp.Controls.Add(this.UdpCommand);
          this.Udp.Controls.Add(this.UdpSend);
          this.Udp.Controls.Add(this.label5);
          this.Udp.Controls.Add(this.UdpLocalPort);
          this.Udp.Controls.Add(this.UdpRemotePort);
          this.Udp.Controls.Add(this.UdpRemoteHostName);
          this.Udp.Controls.Add(this.label4);
          this.Udp.Controls.Add(this.label3);
          this.Udp.Controls.Add(this.label2);
          this.Udp.Location = new System.Drawing.Point(4, 25);
          this.Udp.Name = "Udp";
          this.Udp.Padding = new System.Windows.Forms.Padding(3);
          this.Udp.Size = new System.Drawing.Size(624, 107);
          this.Udp.TabIndex = 1;
          this.Udp.Text = "UDP";
          this.Udp.UseVisualStyleBackColor = true;
          // 
          // UdpCommand
          // 
          this.UdpCommand.Location = new System.Drawing.Point(90, 55);
          this.UdpCommand.Name = "UdpCommand";
          this.UdpCommand.Size = new System.Drawing.Size(425, 22);
          this.UdpCommand.TabIndex = 8;
          this.UdpCommand.KeyUp += new System.Windows.Forms.KeyEventHandler(this.UdpCommand_KeyUp);
          // 
          // UdpSend
          // 
          this.UdpSend.Location = new System.Drawing.Point(525, 54);
          this.UdpSend.Name = "UdpSend";
          this.UdpSend.Size = new System.Drawing.Size(75, 23);
          this.UdpSend.TabIndex = 7;
          this.UdpSend.Text = "Send";
          this.UdpSend.UseVisualStyleBackColor = true;
          this.UdpSend.Click += new System.EventHandler(this.UdpSend_Click);
          // 
          // label5
          // 
          this.label5.AutoSize = true;
          this.label5.Location = new System.Drawing.Point(10, 55);
          this.label5.Name = "label5";
          this.label5.Size = new System.Drawing.Size(71, 17);
          this.label5.TabIndex = 6;
          this.label5.Text = "Command";
          // 
          // UdpLocalPort
          // 
          this.UdpLocalPort.Location = new System.Drawing.Point(501, 17);
          this.UdpLocalPort.Name = "UdpLocalPort";
          this.UdpLocalPort.Size = new System.Drawing.Size(100, 22);
          this.UdpLocalPort.TabIndex = 5;
          this.UdpLocalPort.Text = "10000";
          this.UdpLocalPort.KeyUp += new System.Windows.Forms.KeyEventHandler(this.UdpLocalPort_KeyUp);
          // 
          // UdpRemotePort
          // 
          this.UdpRemotePort.Location = new System.Drawing.Point(276, 20);
          this.UdpRemotePort.Name = "UdpRemotePort";
          this.UdpRemotePort.Size = new System.Drawing.Size(100, 22);
          this.UdpRemotePort.TabIndex = 4;
          this.UdpRemotePort.Text = "10000";
          this.UdpRemotePort.KeyUp += new System.Windows.Forms.KeyEventHandler(this.UdpRemotePort_KeyUp);
          // 
          // UdpRemoteHostName
          // 
          this.UdpRemoteHostName.Location = new System.Drawing.Point(90, 20);
          this.UdpRemoteHostName.Name = "UdpRemoteHostName";
          this.UdpRemoteHostName.Size = new System.Drawing.Size(100, 22);
          this.UdpRemoteHostName.TabIndex = 3;
          this.UdpRemoteHostName.Text = "192.168.0.200";
          this.UdpRemoteHostName.KeyUp += new System.Windows.Forms.KeyEventHandler(this.UdpRemoteHostName_KeyUp);
          // 
          // label4
          // 
          this.label4.AutoSize = true;
          this.label4.Location = new System.Drawing.Point(400, 20);
          this.label4.Name = "label4";
          this.label4.Size = new System.Drawing.Size(95, 17);
          this.label4.TabIndex = 2;
          this.label4.Text = "Listening Port";
          // 
          // label3
          // 
          this.label3.AutoSize = true;
          this.label3.Location = new System.Drawing.Point(215, 20);
          this.label3.Name = "label3";
          this.label3.Size = new System.Drawing.Size(55, 17);
          this.label3.TabIndex = 1;
          this.label3.Text = "To Port";
          // 
          // label2
          // 
          this.label2.AutoSize = true;
          this.label2.Location = new System.Drawing.Point(10, 20);
          this.label2.Name = "label2";
          this.label2.Size = new System.Drawing.Size(81, 17);
          this.label2.TabIndex = 0;
          this.label2.Text = "To Address";
          // 
          // Output
          // 
          this.Output.Location = new System.Drawing.Point(3, 173);
          this.Output.Name = "Output";
          this.Output.Size = new System.Drawing.Size(632, 326);
          this.Output.TabIndex = 1;
          this.Output.Text = "";
          // 
          // tableLayoutPanel1
          // 
          this.tableLayoutPanel1.ColumnCount = 1;
          this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
          this.tableLayoutPanel1.Controls.Add(this.InputPanes, 0, 1);
          this.tableLayoutPanel1.Controls.Add(this.Output, 0, 2);
          this.tableLayoutPanel1.Location = new System.Drawing.Point(12, 12);
          this.tableLayoutPanel1.Name = "tableLayoutPanel1";
          this.tableLayoutPanel1.RowCount = 3;
          this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 28F));
          this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 30.12048F));
          this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 69.87952F));
          this.tableLayoutPanel1.Size = new System.Drawing.Size(638, 502);
          this.tableLayoutPanel1.TabIndex = 2;
          // 
          // MCTestForm
          // 
          this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
          this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
          this.ClientSize = new System.Drawing.Size(662, 529);
          this.Controls.Add(this.tableLayoutPanel1);
          this.Name = "MCTestForm";
          this.Text = "MC Test";
          this.InputPanes.ResumeLayout(false);
          this.Usb.ResumeLayout(false);
          this.Usb.PerformLayout();
          this.Udp.ResumeLayout(false);
          this.Udp.PerformLayout();
          this.tableLayoutPanel1.ResumeLayout(false);
          this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl InputPanes;
        private System.Windows.Forms.TabPage Usb;
        private System.Windows.Forms.TabPage Udp;
        private System.Windows.Forms.RichTextBox Output;
      private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.TextBox UdpCommand;
        private System.Windows.Forms.Button UdpSend;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox UdpLocalPort;
        private System.Windows.Forms.TextBox UdpRemotePort;
        private System.Windows.Forms.TextBox UdpRemoteHostName;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label Command;
        private System.Windows.Forms.Button UsbSend;
        private System.Windows.Forms.TextBox UsbCommand;
        private System.Windows.Forms.TextBox UsbPortName;
    }
}