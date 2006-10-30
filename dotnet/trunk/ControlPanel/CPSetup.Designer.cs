namespace ControlPanel
{
  partial class CPSetup
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
      this.components = new System.ComponentModel.Container();
      this.label1 = new System.Windows.Forms.Label();
      this.CPU = new System.Windows.Forms.TextBox();
      this.timer1 = new System.Windows.Forms.Timer(this.components);
      this.SuspendLayout();
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(6, 13);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(36, 17);
      this.label1.TabIndex = 1;
      this.label1.Text = "CPU";
      // 
      // CPU
      // 
      this.CPU.Location = new System.Drawing.Point(163, 8);
      this.CPU.Name = "CPU";
      this.CPU.Size = new System.Drawing.Size(100, 22);
      this.CPU.TabIndex = 2;
      // 
      // timer1
      // 
      this.timer1.Enabled = true;
      this.timer1.Interval = 250;
      this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
      // 
      // CPSetup
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(275, 46);
      this.Controls.Add(this.CPU);
      this.Controls.Add(this.label1);
      this.Name = "CPSetup";
      this.Text = "MC Control Panel";
      this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.CPSetup_FormClosed);
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.TextBox CPU;
    private System.Windows.Forms.Timer timer1;
  }
}

