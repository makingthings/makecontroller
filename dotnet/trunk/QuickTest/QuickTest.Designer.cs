namespace QuickTest
{
  partial class QuickTest
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
      this.TrimPotIndicator = new System.Windows.Forms.ProgressBar();
      this.label1 = new System.Windows.Forms.Label();
      this.label3 = new System.Windows.Forms.Label();
      this.Led0 = new System.Windows.Forms.CheckBox();
      this.Led1 = new System.Windows.Forms.CheckBox();
      this.Led2 = new System.Windows.Forms.CheckBox();
      this.Led3 = new System.Windows.Forms.CheckBox();
      this.timer1 = new System.Windows.Forms.Timer(this.components);
      this.SuspendLayout();
      // 
      // TrimPotIndicator
      // 
      this.TrimPotIndicator.Location = new System.Drawing.Point(127, 12);
      this.TrimPotIndicator.Maximum = 1023;
      this.TrimPotIndicator.Name = "TrimPotIndicator";
      this.TrimPotIndicator.Size = new System.Drawing.Size(170, 17);
      this.TrimPotIndicator.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
      this.TrimPotIndicator.TabIndex = 0;
      this.TrimPotIndicator.Value = 512;
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(12, 12);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(61, 17);
      this.label1.TabIndex = 1;
      this.label1.Text = "Trim Pot";
      // 
      // label3
      // 
      this.label3.AutoSize = true;
      this.label3.Location = new System.Drawing.Point(12, 40);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(91, 17);
      this.label3.TabIndex = 2;
      this.label3.Text = "LED Controls";
      // 
      // Led0
      // 
      this.Led0.AutoSize = true;
      this.Led0.Location = new System.Drawing.Point(127, 41);
      this.Led0.Name = "Led0";
      this.Led0.Size = new System.Drawing.Size(38, 21);
      this.Led0.TabIndex = 3;
      this.Led0.Text = "0";
      this.Led0.UseVisualStyleBackColor = true;
      this.Led0.CheckedChanged += new System.EventHandler(this.Led0_CheckedChanged);
      // 
      // Led1
      // 
      this.Led1.AutoSize = true;
      this.Led1.Location = new System.Drawing.Point(171, 41);
      this.Led1.Name = "Led1";
      this.Led1.Size = new System.Drawing.Size(38, 21);
      this.Led1.TabIndex = 4;
      this.Led1.Text = "1";
      this.Led1.UseVisualStyleBackColor = true;
      this.Led1.CheckedChanged += new System.EventHandler(this.Led1_CheckedChanged);
      // 
      // Led2
      // 
      this.Led2.AutoSize = true;
      this.Led2.Location = new System.Drawing.Point(215, 41);
      this.Led2.Name = "Led2";
      this.Led2.Size = new System.Drawing.Size(38, 21);
      this.Led2.TabIndex = 5;
      this.Led2.Text = "2";
      this.Led2.UseVisualStyleBackColor = true;
      this.Led2.CheckedChanged += new System.EventHandler(this.Led2_CheckedChanged);
      // 
      // Led3
      // 
      this.Led3.AutoSize = true;
      this.Led3.Location = new System.Drawing.Point(259, 41);
      this.Led3.Name = "Led3";
      this.Led3.Size = new System.Drawing.Size(38, 21);
      this.Led3.TabIndex = 6;
      this.Led3.Text = "3";
      this.Led3.UseVisualStyleBackColor = true;
      this.Led3.CheckedChanged += new System.EventHandler(this.Led3_CheckedChanged);
      // 
      // timer1
      // 
      this.timer1.Enabled = true;
      this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
      // 
      // QuickTest
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(321, 80);
      this.Controls.Add(this.Led3);
      this.Controls.Add(this.Led2);
      this.Controls.Add(this.Led1);
      this.Controls.Add(this.Led0);
      this.Controls.Add(this.label3);
      this.Controls.Add(this.label1);
      this.Controls.Add(this.TrimPotIndicator);
      this.Name = "QuickTest";
      this.Text = "MC Quick Test";
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.ProgressBar TrimPotIndicator;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.CheckBox Led0;
    private System.Windows.Forms.CheckBox Led1;
    private System.Windows.Forms.CheckBox Led2;
    private System.Windows.Forms.CheckBox Led3;
    private System.Windows.Forms.Timer timer1;
  }
}

