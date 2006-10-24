using System.Threading;
using System.Text;
using System.Collections;
using System;


namespace MakingThings
{
  public class OscMessage
  {
    public OscMessage()
    {
      Values = new ArrayList();
    }
      
   public string Address;
   public ArrayList Values;
  }

  public class Osc
  {
    public Osc()
    {
    }

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

    public static OscMessage StringToOscMessage(string message)
    {
      OscMessage oM = new OscMessage();
      Console.WriteLine("Splitting " + message);
      string[] ss = message.Split(new char[] { ' ' });
      IEnumerator sE = ss.GetEnumerator();
      if (sE.MoveNext())
        oM.Address = (string)sE.Current;
      while ( sE.MoveNext() )
      {
        string s = (string)sE.Current;
        Console.WriteLine("  <" + s + ">");
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
            Console.WriteLine("    q:<" + a + ">");
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
              Console.WriteLine("  i:" + i);
              oM.Values.Add(i);
            }
            catch
            {
              try
              {
                float f = float.Parse(s);
                Console.WriteLine("  f:" + f);
                oM.Values.Add(f);
              }
              catch
              {
                Console.WriteLine("  s:" + s);
                oM.Values.Add(s);
              }
            }

          }
        }
      }

      return oM;
    }

    public static ArrayList PacketToOscMessages(byte[] packet, int length)
    {
      ArrayList messages = new ArrayList();
      ExtractMessages(messages, packet, 0, length);
      return messages;
    }

    public static int OscMessagesToPacket(ArrayList messages, byte[] packet, int length)
    {
      return 0;
    }

    public static int OscMessageToPacket(OscMessage oscM, byte[] packet, int length)
    {
      return OscMessageToPacket(oscM, packet, 0, length);
    }

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
            packet[index++] = 0;
            packet[index++] = 0;
            packet[index++] = 0;
            packet[index++] = 0;
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
              float f = 1.1F;
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
      int pad = s.Length % 4;
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