namespace DeskTimer
{
  partial class DeskTimerForm
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
      this.Reset = new System.Windows.Forms.Button();
      this.WorkingText = new System.Windows.Forms.TextBox();
      this.AwayText = new System.Windows.Forms.TextBox();
      this.label1 = new System.Windows.Forms.Label();
      this.label2 = new System.Windows.Forms.Label();
      this.Timer = new System.Windows.Forms.Timer(this.components);
      this.SuspendLayout();
      // 
      // Reset
      // 
      this.Reset.Location = new System.Drawing.Point(106, 75);
      this.Reset.Name = "Reset";
      this.Reset.Size = new System.Drawing.Size(75, 23);
      this.Reset.TabIndex = 0;
      this.Reset.Text = "Reset";
      this.Reset.UseVisualStyleBackColor = true;
      this.Reset.Click += new System.EventHandler(this.Reset_Click);
      // 
      // WorkingText
      // 
      this.WorkingText.Location = new System.Drawing.Point(12, 33);
      this.WorkingText.Name = "WorkingText";
      this.WorkingText.Size = new System.Drawing.Size(100, 22);
      this.WorkingText.TabIndex = 1;
      this.WorkingText.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
      // 
      // AwayText
      // 
      this.AwayText.Location = new System.Drawing.Point(180, 33);
      this.AwayText.Name = "AwayText";
      this.AwayText.Size = new System.Drawing.Size(100, 22);
      this.AwayText.TabIndex = 2;
      this.AwayText.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(34, 9);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(60, 17);
      this.label1.TabIndex = 3;
      this.label1.Text = "Working";
      // 
      // label2
      // 
      this.label2.AutoSize = true;
      this.label2.Location = new System.Drawing.Point(214, 9);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(41, 17);
      this.label2.TabIndex = 4;
      this.label2.Text = "Away";
      // 
      // Timer
      // 
      this.Timer.Enabled = true;
      this.Timer.Interval = 1000;
      this.Timer.Tick += new System.EventHandler(this.Timer_Tick);
      // 
      // DeskTimerForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(294, 110);
      this.Controls.Add(this.label2);
      this.Controls.Add(this.label1);
      this.Controls.Add(this.AwayText);
      this.Controls.Add(this.WorkingText);
      this.Controls.Add(this.Reset);
      this.Name = "DeskTimerForm";
      this.Text = "MCK Timer";
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Button Reset;
    private System.Windows.Forms.TextBox WorkingText;
    private System.Windows.Forms.TextBox AwayText;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.Timer Timer;
  }
}

