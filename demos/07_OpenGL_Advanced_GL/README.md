## 07_OpenGL_Advanced_Rendering

这是一个基于 **OpenGL 3.3 Core Profile** 的学习型 Demo，参照 LearnOpenGL CN  
**高级 OpenGL（深度测试 / 混合 / 面剔除 / 帧缓冲）** 内容完成。

本 Demo 将多个进阶渲染技术整合在同一场景中，实现了  
**不透明物体 + 半透明物体 + 离屏渲染 + 屏幕空间绘制** 的完整渲染流程。

---

## 主要内容

- 深度测试（Depth Testing）
- 深度缓冲（Depth Buffer）
- 面剔除（Back-face Culling）
- 透明物体混合（Blending）
- 透明物体按距离排序（Back-to-Front Sorting）
- 帧缓冲对象（Framebuffer Object, FBO）
- 屏幕四边形（Screen Quad）
- 双阶段渲染流程（Two-pass Rendering）

---

## Demo 效果预览

<p align="center">
  <img src="resources/assets/test.png" width="600"/>
</p>

> 场景说明：
> - 两个带纹理的立方体
> - 一个纹理地面
> - 多个半透明玻璃窗（按距离排序绘制）
> - 场景首先渲染到 FBO，再通过 Screen Quad 输出到屏幕
> - 片段着色器添加了模糊效果
---

## 核心实现说明

---

### 1. 深度测试（Depth Testing）

```cpp
glEnable(GL_DEPTH_TEST);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

- 使用深度缓冲保证遮挡关系正确
- 每帧同时清除颜色与深度缓冲
- 避免远处物体覆盖近处物体

### 2. 面剔除（Face Culling）
```cpp
glEnable(GL_CULL_FACE);
glCullFace(GL_BACK);
glFrontFace(GL_CCW);
```

- 剔除背面三角形，提高渲染效率
- 半透明物体绘制时关闭面剔除，避免错误消失

```cpp
glDisable(GL_CULL_FACE);
// 绘制透明物体
glEnable(GL_CULL_FACE);
```

### 3. 透明物体混合 （Blending）
```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```
- 使用标准 Alpha 混合公式
- 支持 PNG 透明纹理
- 结合排序实现正确叠加

### 4. 透明物体排序
由于深度缓冲无法正确处理透明物体叠加顺序，
在渲染前对透明物体按与摄像机的距离进行排序：

```cpp
std::map<float, glm::vec3> sorted;

for (unsigned int i = 0; i < windows.size(); i++)
{
    float distance = glm::length(camera.Position - windows[i]);
    sorted[distance] = windows[i];
}
```
由远及近绘制
```cpp
for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
{
    model = glm::translate(glm::mat4(1.0f), it->second);
    shader.setMat4("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
```
- 保证透明物体后画近处，避免覆盖错误
- 实现正确的玻璃叠加效果

### 5. 帧缓冲渲染
```cpp
// 场景渲染到离屏缓冲
glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// 创建颜色纹理附件
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

// 创建深度缓冲附件
glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

// 将fbo纹理绘制到默认缓冲
glBindFramebuffer(GL_FRAMEBUFFER, 0);
glDisable(GL_DEPTH_TEST);

screenShader.use();
glBindVertexArray(quadVAO);
glBindTexture(GL_TEXTURE_2D, texture);
glDrawArrays(GL_TRIANGLES, 0, 6);
```
- 使用一个覆盖整个屏幕的 Quad
- 采样 FBO 中的颜色纹理
- 为后续添加后处理效果打下基础（灰度、反色、卷积核等）

## 基础交互说明（示例）

- **W / A / S / D**：前后左右移动摄像机  
- **鼠标移动**：控制视角方向  
- **滚轮**：缩放视野（FOV）

---

## 使用的技术栈

- **OpenGL 3.3 Core Profile**
- **GLFW**
- **GLAD**
- **GLM**

---

### build
```bash
git clone https://github.com/HYChyc-ai/Graphics_Engineer_Portfolio.git
cd Graphics_Engineer_Portfolio
mkdir build && cd build
cmake ..
