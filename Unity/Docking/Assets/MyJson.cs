using System;
using System.IO;
using System.Collections.Generic;
using UnityEngine;


public class MyJsonReader<T>
{

  private static Dictionary<string, MyJsonReader<T>> m_Instances = new Dictionary<string, MyJsonReader<T>>();

  T m_Data;
  bool m_Exists;

  public T data => m_Data;
  public bool exists => m_Exists;

  private MyJsonReader(string path)
  {
    Load(path);
  }

  public static MyJsonReader<T> GetInstance(string path)
  {
    if (m_Instances.ContainsKey(path)) return m_Instances[path];
    MyJsonReader<T> m = new MyJsonReader<T>(path);
    m_Instances.Add(path, m);
    return m;
  }

  void Load(string path)
  {

#if UNITY_EDITOR
    string datapath = $"{Application.dataPath}/{path}";
#else
    string datapath = $"{Application.persistentDataPath}/{path}";
#endif
    Debug.Log(datapath);
    if(File.Exists(datapath))
    {
      StreamReader reader = new StreamReader(datapath);
      string datastr = reader.ReadToEnd();
      reader.Close();

      T d = JsonUtility.FromJson<T>(datastr);
      Debug.Log(datastr);
      Debug.Log(d);
      m_Data = d;
      m_Exists = true;
    }
    else
    {
      m_Exists = false;
    }
  }

}

public class MyJsonWriter
{

  public static void Write(string filename, string text)
  {
#if UNITY_EDITOR
    string datapath = $"{Application.dataPath}/{filename}";
#else
    string datapath = $"{Application.persistentDataPath}/{filename}";
#endif
    StreamWriter writer = new StreamWriter(datapath);
    writer.Write(text);
    writer.Close();
  }
}
