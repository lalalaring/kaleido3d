Introduction 
=========

**kaleido3d** is a cross-platform graphics library.

>* Next-Gen Graphics API Agnostic Layer (NGFX) for `Metal`, `Vulkan`, `DX12`
>* Provide a Physically Based Renderer
>* Multi-Threaded Rendering 
>* Use GLTF as Internal Mesh Format
>* Cross platform, Support Windows/UWP, Android, iOS, MacOS & Linux
>* Offline Rendering Support via `Compute Shader`
>* Experimental Real Time Ray-tracing Support
>* Provide a cross-platform Build System (Xbuild), even build ios targets on Windows
>* Provide iOS/Android Profiling Toolchain (Cross platform)
>* Real-time Device Viewport for Preview

----------

Prerequisites
=========

* Python 2.7
* CMake 3.x
* Visual Studio 2017
* iOS Build Environment
* NDK 14+
* Xcode 7+

---

Wiki
=====

[See Wikis](https://github.com/DsoTsin/kaleido3d/wiki)

---

NGFX Snippets
=======

> **Draw a triangle**

``` cpp
void DrawFrame()
{
  auto currentImage = m_pSwapChain->GetCurrentTexture();
  auto ImageDesc = currentImage->GetDesc();
  k3d::ColorAttachmentDesc ColorAttach;
  ColorAttach.pTexture = currentImage;
  // specify renderpass action, optimize for mobile platform's PLS (Pixel Local Storage)
  ColorAttach.LoadAction = k3d::ELA_Clear;
  ColorAttach.StoreAction = k3d::ESA_Store;
  ColorAttach.ClearColor = Vec4f(1, 1, 1, 1);

  k3d::RenderPassDesc Desc;
  Desc.ColorAttachments.Append(ColorAttach);

  auto commandBuffer = m_pQueue->ObtainCommandBuffer(k3d::ECMDUsage_OneShot);
  // command encoder is similar to apple's metal, look up renderpass object from cache
  // for vulkan, it performs the call "BeginRenderPass"
  auto renderCmd = commandBuffer->RenderCommandEncoder(Desc);
  renderCmd->SetBindingGroup(m_BindingGroup);
  k3d::Rect rect{ 0, 0, ImageDesc.TextureDesc.Width, ImageDesc.TextureDesc.Height };
  renderCmd->SetScissorRect(rect);
  renderCmd->SetViewport(k3d::ViewportDesc(ImageDesc.TextureDesc.Width, ImageDesc.TextureDesc.Height));
  renderCmd->SetPipelineState(0, m_pPso);
  renderCmd->SetIndexBuffer(m_TriMesh->IBO());
  renderCmd->SetVertexBuffer(0, m_TriMesh->VBO());
  renderCmd->DrawIndexedInstanced(k3d::DrawIndexedInstancedParam(3, 1));
  renderCmd->EndEncode();

  commandBuffer->Present(m_pSwapChain, m_pFence);
  commandBuffer->Commit(m_pFence);
}
```

> [RHI Objects were found here.](Document/ApiDifferences.md)

---

Unit Tests
=======

## 1.Triangle(Basic)

![Triangle Screenshot](Document/images/sample_triangle_screenshot.png)

## 2.TexturedCube(Basic)

![Cube Screenshot](Document/images/sample_texturedcube_screenshot.png)

## 3.Compute Particles(Intermediate)

![Compute Sample](Document/images/sample_compute_particles.png)

---

Contact
=========

If you have any suggestion, please contact me via [**email**][12] . 


---

Discuss [![Join the chat at https://gitter.im/TsinStudio/kaleido3d](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/TsinStudio/kaleido3d?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
=========

Please join the [gitter chat](https://gitter.im/TsinStudio/kaleido3d) or [QQ Group][13] to discuss on this project.
Framework development discussions and thorough bug reports are collected on [Issues](https://github.com/TsinStudio/kaleido3d/issues).

---


Credits
=====

**ThirdParty Libraries:**

* OpenSSL (Core)
* PCRE2 (Core)
* Snappy (Core)
* ZLib (Core)
* Vulkan Memory Allocator (NGFX)
* FreeType (ShaderCompiler)
* Glslang (ShaderCompiler)
* SPIRV-CROSS (ShaderCompiler)
* SPIRV-TOOLS (ShaderCompiler)
* LLVM (Reflection)
* Clang (Reflection)
* GoogleV8 (Script)
* GoogleTest (Unit Test)
* rapidjson
* imobiledevice (MobileDevice)


[1]: http://www.cmake.org
[2]: https://developer.nvidia.com/gameworksdownload
[3]: https://github.com/miloyip/rapidjson
[4]: https://code.csdn.net/tomicyo/kaleido3d_dep
[5]: https://github.com/google/protobuf
[6]: https://www.threadingbuildingblocks.org/
[7]: https://github.com/KhronosGroup/glslang
[8]: https://github.com/Tomicyo/kaleido3d_dep
[9]: https://vulkan.lunarg.com/
[10]: https://github.com/KhronosGroup/SPIRV-Cross
[11]: https://www.freetype.org
[12]: mailto:dsotsen@gmail.com
[13]: https://jq.qq.com/?_wv=1027&k=45tL869
