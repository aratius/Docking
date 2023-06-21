using UnityEngine;
using System;  // Needed for Math
using DG.Tweening;

public class Sinus : MonoBehaviour
{
    public double frequency = 440;
    public double _gain = 0.05;
    private double increment;
    private double phase;
    private double sampling_frequency = 48000;

    public double gain
    {
        set {
            DOTween.To(
                () => _gain,
                v => _gain = v,
                value,
                .01f
            );
        }
    }

    void OnAudioFilterRead(float[] data, int channels)
    {
        increment = frequency * 2 * Math.PI / sampling_frequency;

        for (var i = 0; i < data.Length; i = i + channels)
        {
            phase = phase + increment;
            data[i] = (float)(_gain*Math.Sin(phase));
            if (channels == 2) data[i + 1] = data[i];
            if (phase > 2 * Math.PI) phase = 0;
        }
    }
}
