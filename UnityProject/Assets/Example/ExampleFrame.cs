using UnityEngine;
using UnityEngine.UI;

public class ExampleFrame : MonoBehaviour {

    public Text ouputTimings;

    private void LateUpdate() {
        ouputTimings.text = string.Format("FPS:{0:F1} ({1:F2}ms), GPU:{2:F2}ms", 1.0f / Time.deltaTime, Time.deltaTime * 1000.0f, D3D11Plugins.FrameDuration * 1000.0f);
    }
}
