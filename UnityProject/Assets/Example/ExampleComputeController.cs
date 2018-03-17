using UnityEngine;
using UnityEngine.UI;

public class ExampleComputeController : MonoBehaviour {

    public int textureSize = 4096;
    public ComputeShader computeShader;
    public RawImage outputRawImage;
    public Text ouputTimings;

    private RenderTexture targetTexture;
    D3D11GPUTimer timer;

    private void Start() {
        timer = D3D11Plugins.CreateGPUTimer();
        targetTexture = new RenderTexture(textureSize, textureSize, 0, RenderTextureFormat.ARGBFloat);
        targetTexture.enableRandomWrite = true;
        targetTexture.Create();
        outputRawImage.texture = targetTexture;
    }

    private void LateUpdate() {
        // Output
        ouputTimings.text = string.Format("Compute:{0}x{0} {1:F2}ms", textureSize, timer.Duration * 1000.0);
        // Dispatch compute shader
        computeShader.SetVector("targetTextureSize", new Vector4(targetTexture.width, targetTexture.height));
        computeShader.SetTexture(0, "targetTexture", targetTexture);
        timer.Begin();
        computeShader.Dispatch(0, targetTexture.width / 16, targetTexture.height / 16, 1);
        timer.End();
    }

}
