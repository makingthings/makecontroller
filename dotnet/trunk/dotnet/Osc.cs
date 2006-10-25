using System.Threading;
using System.Text;
using System.Collections;
using System.IO;
using System;


namespace MakingThings
{
  /// <summary>
  /// The OscMessage class represents an OSC address and an arbitrary number of values to be sent to that address.
  /// </summary>
  public class OscMessage
  {
    public OscMessage()
    {
      Values = new ArrayList();
    }
      
   public string Address;
   public ArrayList Values;
  }

  public delegate void OscMessageHandler( OscMessage oscM );

  /// <summary>
  /// The Osc class provides the methods required to send, receive, and manipulate OSC messages.
  /// </summary>
  public class Osc
  {
    public Osc(PacketExchange oscPacketExchange)
    {
      OscPacketExchange = oscPacketExchange;

      AddressTable = new Hashtable();

      OscPacketExchange.Open();

      ReadThread = new Thread(Read);
      ReaderRunning = true;
      ReadThread.Start();
    }

    private void Read()
    {
      while (ReaderRunning)
      {
        byte[] buffer = new byte[1000];
        int length = OscPacketExchange.ReceivePacket(buffer);
        if (length > 0)
        {
          ArrayList messages = Osc.PacketToOscMessages(buffer, length);
          foreach (OscMessage om in messages)
          {
            if (AllMessageHandler != null)
              AllMessageHandler(om);
            OscMessageHandler h = (OscMessageHandler)Hashtable.Synchronized(AddressTable)[om.Address];
            if ( h != null )
              h(om);
          }
        }
      }
    }

    public void Send( OscMessage oscMessage )
    {
      byte[] packet = new byte[1000];
      int length = Osc.OscMessageToPacket( oscMessage, packet, 1000 );
      OscPacketExchange.SendPacket( packet, length);
    }

    public void Send(ArrayList oms)
    {
      byte[] packet = new byte[1000];
      int length = Osc.OscMessagesToPacket(oms, packet, 1000);
      OscPacketExchange.SendPacket(packet, length);
    }

    public void SetAllMessageHandler(OscMessageHandler amh)
    {
      AllMessageHandler = amh;
    }

    public void SetAddressHandler(string key, OscMessageHandler ah)
    {
      Hashtable.Synchronized(AddressTable).Add(key, ah);
    }

    private PacketExchange OscPacketExchange;
    Thread ReadThread;
    private bool ReaderRunning; 
    private OscMessageHandler AllMessageHandler;
    Hashtable AddressTable;

    /// <summary>
    /// OscMessageToString() returns a string containing the Osc address and values in an OscMessage.
    /// </summary>
    /// <param name="message">The OscMessage to be stringified.</param>
    /// <returns>The OscMessage as a string.</returns>
    public static string OscMessageToString(OscMessage message)
    {
      StringBuilder s = new StringBuilder();
      s.Append(message.Address);
      foreach( object o in message.Values )
      {
        s.Append(" ");
        s.Append(o.ToString());
      }
      return s.ToString();
    }

    /// <summary>
    /// Creates an OscMessage from a string - extracts the address and determines each of the values. 
    /// </summary>
    /// <param name="message">The string to be turned into an OscMessage</param>
    /// <returns>The OscMessage.</returns>
    public static OscMessage StringToOscMessage(string message)
    {
      OscMessage oM = new OscMessage();
      // Console.WriteLine("Splitting " + message);
      string[] ss = message.Split(new char[] { ' ' });
      IEnumerator sE = ss.GetEnumerator();
      if (sE.MoveNext())
        oM.Address = (string)sE.Current;
      while ( sE.MoveNext() )
      {
        string s = (string)sE.Current;
        // Console.WriteLine("  <" + s + ">");
        if (s.StartsWith("\""))
        {
          StringBuilder quoted = new StringBuilder();
          bool looped = false;
          if (s.Length > 1)
            quoted.Append(s.Substring(1));
          else
            looped = true;
          while (sE.MoveNext())
          {
            string a = (string)sE.Current;
            // Console.WriteLine("    q:<" + a + ">");
            if (looped)
              quoted.Append(" ");
            if (a.EndsWith("\""))
            {
              quoted.Append(a.Substring(0, a.Length - 1));
              break;
            }
            else
            {
              if (a.Length == 0)
                quoted.Append(" ");
              else
                quoted.Append(a);
            }
            looped = true;
          }
          oM.Values.Add(quoted.ToString());
        }
        else
        {
          if (s.Length > 0)
          {
            try
            {
              int i = int.Parse(s);
              // Console.WriteLine("  i:" + i);
              oM.Values.Add(i);
            }
            catch
            {
              try
              {
                float f = float.Parse(s);
                // Console.WriteLine("  f:" + f);
                oM.Values.Add(f);
              }
              catch
              {
                // Console.WriteLine("  s:" + s);
                oM.Values.Add(s);
              }
            }

          }
        }
      }
      return oM;
    }

    /// <summary>
    /// Takes a packet of bytes and turns it into a list of OscMessages.
    /// </summary>
    /// <param name="packet">The packet to be parsed.</param>
    /// <param name="length">The length of the packet.</param>
    /// <returns>An ArrayList of OscMessages.</returns>
    public static ArrayList PacketToOscMessages(byte[] packet, int length)
    {
      ArrayList messages = new ArrayList();
      ExtractMessages(messages, packet, 0, length);
      return messages;
    }

    /// <summary>
    /// Puts an array of OscMessages into a packet of bytes.
    /// </summary>
    /// <param name="messages">An ArrayList of OscMessages.</param>
    /// <param name="packet">An array of bytes to be populated with the the OscMessages.</param>
    /// <param name="length">The size of the array of bytes.</param>
    /// <returns>The size of the packet</returns>
    public static int OscMessagesToPacket(ArrayList messages, byte[] packet, int length)
    {
      int index = 0;
      if (messages.Count == 1)
        index = OscMessageToPacket((OscMessage)messages[0], packet, 0, length);
      else
      {
        // Write the first bundle bit
        index = InsertString("#bundle", packet, index, length);
        // Write a null timestamp (another 8bytes)
        int c = 8;
        while (( c-- )>0)
          packet[index++]++;
        // Now, put each message preceded by it's length
        foreach (OscMessage oscM in messages)
        {
          int lengthIndex = index;
          index += 4;
          int packetStart = index;
          index = OscMessageToPacket(oscM, packet, index, length);
          int packetSize = index - packetStart;
          packet[lengthIndex++] = (byte)((packetSize >> 24) & 0xFF);
          packet[lengthIndex++] = (byte)((packetSize >> 16) & 0xFF);
          packet[lengthIndex++] = (byte)((packetSize >> 8) & 0xFF);
          packet[lengthIndex++] = (byte)((packetSize) & 0xFF);
        }
      }
      return index;
    }

    /// <summary>
    /// Creates an array of bytes from a single OscMessage.
    /// </summary>
    /// <remarks>A convenience method, not requiring a start index.</remarks>
    /// <param name="oscM">The OscMessage to be turned into an array of bytes.</param>
    /// <param name="packet">The array of bytes to be populated with the OscMessage.</param>
    /// <param name="length">The usable size of the array of bytes.</param>
    /// <returns>The length of the packet</returns>
    public static int OscMessageToPacket(OscMessage oscM, byte[] packet, int length)
    {
      return OscMessageToPacket(oscM, packet, 0, length);
    }

    /// <summary>
    /// Creates an array of bytes from a single OscMessage.
    /// </summary>
    /// <remarks>Can specify where in the array of bytes the OscMessage should be put.</remarks>
    /// <param name="oscM">The OscMessage to be turned into an array of bytes.</param>
    /// <param name="packet">The array of bytes to be populated with the OscMessage.</param>
    /// <param name="start">The start index in the packet where the OscMessage should be put.</param>
    /// <param name="length">The length of the array of bytes.</param>
    /// <returns>The index into the packet after the last OscMessage.</returns>
    private static int OscMessageToPacket(OscMessage oscM, byte[] packet, int start, int length)
    {
      int index = start;
      index = InsertString(oscM.Address, packet, index, length);
      StringBuilder tag = new StringBuilder();
      tag.Append(",");
      int tagIndex = index;
      index += PadSize(1 + oscM.Values.Count);
      foreach (object o in oscM.Values)
      {
        if (o is int)
        {
          int i = (int)o;
          tag.Append("i");
          packet[index++] = (byte)((i >> 24) & 0xFF);
          packet[index++] = (byte)((i >> 16) & 0xFF);
          packet[index++] = (byte)((i >> 8) & 0xFF);
          packet[index++] = (byte)((i) & 0xFF);
        }
        else
        {
          if (o is float)
          {
            float f = (float)o;
            tag.Append("f");
            byte[] buffer = new byte[4];
            MemoryStream ms = new MemoryStream( buffer );
            BinaryWriter bw = new BinaryWriter(ms);
            bw.Write(f);
            packet[index++] = buffer[3];
            packet[index++] = buffer[2];
            packet[index++] = buffer[1];
            packet[index++] = buffer[0];
          }
          else
          {
            if (o is string)
            {
              tag.Append("s");
              index = InsertString(o.ToString(), packet, index, length);
            }
            else
            {
              tag.Append("?");
            }
          }
        }
      }
      InsertString(tag.ToString(), packet, tagIndex, length);
      return index;
    }

    /// <summary>
    /// Receive a raw packet of bytes and extract OscMessages from it.
    /// </summary>
    /// <remarks>The packet may be an OSC message or a bundle.</remarks>
    /// <param name="messages">An ArrayList to be populated with the OscMessages.</param>
    /// <param name="packet">The packet of bytes to be parsed.</param>
    /// <param name="start">The index of where to start looking in the packet.</param>
    /// <param name="length">The length of the packet.</param>
    /// <returns>The index after the last OscMessage read.</returns>
    private static int ExtractMessages(ArrayList messages, byte[] packet, int start, int length)
    {
      int index = start;
      switch ( (char)packet[ start ] )
      {
        case '/':
          index = ExtractMessage( messages, packet, index, length );
          break;
        case '#':
          string bundleString = ExtractString(packet, start, length);
          if ( bundleString == "#bundle" )
          {
            // skip the "bundle" and the timestamp
            index+=16;
            while ( index < length )
            {
              int messageSize = ( packet[index++] << 24 ) + ( packet[index++] << 16 ) + ( packet[index++] << 8 ) + packet[index++];
              int newIndex = ExtractMessages( messages, packet, index, length ); 
              index += messageSize;
            }            
          }
          break;
      }
      return index;
    }

    private static int ExtractMessage(ArrayList messages, byte[] packet, int start, int length)
    {
      OscMessage oscM = new OscMessage();
      oscM.Address = ExtractString(packet, start, length);
      int index = start + PadSize(oscM.Address.Length+1);
      string typeTag = ExtractString(packet, index, length);
      index += PadSize(typeTag.Length + 1);
      //oscM.Values.Add(typeTag);
      foreach (char c in typeTag)
      {
        switch (c)
        {
          case ',':
            break;
          case 's':
            {
              string s = ExtractString(packet, index, length);
              index += PadSize(s.Length + 1);
              oscM.Values.Add(s);
              break;
            }
          case 'i':
            {
              int i = ( packet[index++] << 24 ) + ( packet[index++] << 16 ) + ( packet[index++] << 8 ) + packet[index++];
              oscM.Values.Add(i);
              break;
            }
          case 'f':
            {
              byte[] buffer = new byte[4];
              buffer[3] = packet[index++];
              buffer[2] = packet[index++];
              buffer[1] = packet[index++];
              buffer[0] = packet[index++];
              MemoryStream ms = new MemoryStream(buffer);
              BinaryReader br = new BinaryReader(ms);
              float f = br.ReadSingle();
              oscM.Values.Add(f);
              break;
            }
        }
      }
      messages.Add( oscM );
      return index;
    }

    private static string ExtractString(byte[] packet, int start, int length)
    {
      StringBuilder sb = new StringBuilder();
      int index = start;
      while (packet[index] != 0 && index < length)
        sb.Append((char)packet[index++]);
      return sb.ToString();
    }

    private static int InsertString(string s, byte[] packet, int start, int length)
    {
      int index = start;
      foreach (char c in s)
      {
        packet[index++] = (byte)c;
        if (index == length)
          return index;
      }
      packet[index++] = 0;
      int pad = (s.Length+1) % 4;
      if (pad != 0)
      {
        while (pad-- > 0)
          packet[index++] = 0;
      }
      return index;
    }

    private static int PadSize(int rawSize)
    {
      int pad = rawSize % 4;
      if (pad == 0)
        return rawSize;
      else
        return rawSize + (4 - pad);
    }
  }
}