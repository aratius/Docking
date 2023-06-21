using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using System.Net;
using TMPro;
using System.Text.RegularExpressions;
using OscJack;
using Cysharp.Threading.Tasks;

[System.Serializable]
public class Device
{
    public int number;
    public Device(int n) { number = n; }
}

[System.Serializable]
public class Host
{
    public string ip;
    public Host(string _ip) { ip = _ip; }
}

public class App : MonoBehaviour
{

    [SerializeField]
    Sinus m_Sinus;
    [SerializeField]
    List<Button> m_Buttons = new List<Button>();
    [SerializeField]
    TextMeshProUGUI m_DeviceNameText;
    [SerializeField]
    TextMeshProUGUI m_IpText;
    [SerializeField]
    TextMeshProUGUI m_OutPortText;
    [SerializeField]
    TextMeshProUGUI m_InPortText;
    [SerializeField]
    TextMeshProUGUI m_HostIpText;
    [SerializeField]
    Material m_Material;

    OscServer m_OscServer;
    OscClient m_OscClient;
    int m_DeviceNumber = 0;

    readonly int m_OutPort = 10000;
    int m_InPort;

    static bool IsValidIPAddress(string ip)
    {
        string pattern = @"^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$";
        return Regex.IsMatch(ip, pattern);
    }

    static string GetIP()
    {
        string hostname = Dns.GetHostName();

        IPAddress[] ipAddresses = Dns.GetHostAddresses(hostname);

        foreach (IPAddress ipAddress in ipAddresses)
        {
            if(IsValidIPAddress(ipAddress.ToString()))
            {
                Debug.Log(ipAddress.ToString());
                return ipAddress.ToString();
            }
        }
        return "";
    }

    // Start is called before the first frame update
    void Start()
    {
        for(int i = 0; i < m_Buttons.Count; i++)
        {
            int index = i;
            Button button = m_Buttons[i];
            button.onClick.AddListener(() => OnClickButton(index));
            index++;
        }

        MyJsonReader<Host> hostInfo = MyJsonReader<Host>.GetInstance("host.json");
        string host = hostInfo.exists ? hostInfo.data.ip : "127.0.0.1";
        m_OscClient = new OscClient(host, m_OutPort);
        m_HostIpText.text = $"HOST IP : {host}";

        MyJsonReader<Device> deviceInfo = MyJsonReader<Device>.GetInstance("device.json");
        if(deviceInfo.exists) Init(deviceInfo.data.number);
        m_OutPortText.text = $"PORT(OUT) : {m_OutPort}";
        m_IpText.text =  $"IP : {App.GetIP()}";

        m_Material.SetVector("_Resolution", new Vector2(Screen.width, Screen.height));
    }

    // Update is called once per frame
    void Update()
    {
        if(Time.frameCount % 30 == 0)
        {
            if(m_DeviceNumber != 0) m_OscClient.Send($"/ipod/{m_DeviceNumber}/status", 1);
        }

        float t = (float)(DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() % (3600 * 1000)) / 1000f;
        m_Material.SetFloat("_UtcTime", t);
    }

    void OnClickButton(int index)
    {
        Debug.Log($"click {index}");
        Init(index);
    }

    void Init(int n)
    {
        // DeInit ---
        if(m_OscServer != null) m_OscServer.Dispose();
        m_Sinus.gain = 0f;

        // Init ---
        m_DeviceNumber = n;
        m_InPort = 20000 + m_DeviceNumber;
        m_OscServer = new OscServer(m_InPort);
        m_OscServer.MessageDispatcher.AddCallback("/sit", OnReceiveSit);
        m_OscServer.MessageDispatcher.AddCallback("/hand", OnReceiveHand);
        m_InPortText.text = $"PORT(IN) : {m_InPort}";
        m_DeviceNameText.text = $"iPod-{m_DeviceNumber}";

        m_OscClient.Send($"/ipod/{m_DeviceNumber}/ip", App.GetIP());
        m_OscClient.Send($"/ipod/{m_DeviceNumber}/port", m_InPort);

        MyJsonWriter.Write("device.json", JsonUtility.ToJson(new Device(m_DeviceNumber)));
        Select(m_DeviceNumber-1);

        m_Material.SetFloat("_Index", m_DeviceNumber);
    }

    void Select(int index)
    {
        for(int i = 0; i < m_Buttons.Count; i++)
        {
            Image img = m_Buttons[i].gameObject.GetComponent<Image>();
            Color c = img.color;
            c.a = i == index ? 1f : .2f;
            img.color = c;
        }
    }

    async void OnReceiveSit(string address, OscDataHandle data)
    {
        await UniTask.WaitForFixedUpdate();
        int val = data.GetElementAsInt(0);
        Debug.Log(val);
        m_Sinus.gain = val == 1 ? .2f : 0f;
    }

    async void OnReceiveHand(string address, OscDataHandle data)
    {
        await UniTask.WaitForFixedUpdate();

    }

}
