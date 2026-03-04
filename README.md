# Graphics Engineer Portfolio

## OpenGL Learning Demos

### 01. 第一个彩色三角形
- VAO / VBO / Shader 基础
- [进入项目](./demos/01_OpenGL_Triangle)

### 02. 纹理与坐标变换
- 使用stb_image加载纹理
- 纹理环绕与过滤方式
- 结合基础着色器管线
- 基本图像坐标变换：平移，旋转，放缩
- [进入项目](./demos/02_OpenGL_Texture)

### 03. 多立方体动画&摄像机类
- 理解 **Model / View / Projection** 矩阵在渲染管线中的作用
- 掌握 **基于时间的动画（Time-based Animation）**
- 实现 **多个物体的独立变换**
- 整合 **摄像机系统（Camera）**

- [进入项目](./demos/03_OpenGL_3D_Space)

### 04. 基础光照&材质
- 实现 **环境光** 、 **漫反射光** 以及 **镜面反射光** 在渲染中的应用
- 实现各方向向量和 **物体表面法线** 在着色器中的计算
- 理解材质与光照不同分量的影响值不同，实现着色器中单独的 **光照与材质结构体**
- 实现 **不同光源** 下物体的颜色输出

- [进入项目](./demos/04_OpenGL_Light)

### 05. 动态光源&多光照
- **Phong 光照模型** 完整实现
- 材质（Material）与光源（Light）结构体设计
- 点光源（Point Light）
- **聚光灯**（Spot Light，摄像机手电筒效果）
- 聚光灯 **平滑边缘**（Soft Edge）
- **多光源** 光照结果叠加
- **法线矩阵**（Normal Matrix）解决旋转导致的高光错误
- **动态光源运动**（绕物体旋转并上下起伏）

- [进入项目](./demos/05_OpenGL_Advanced_Light)
  
### 06. 高级渲染技术（深度 / 混合 / 帧缓冲）
- **深度测试**（Depth Testing）与深度缓冲
- **面剔除**（Back-face Culling）提升渲染效率
- **透明物体混合**（Blending）
- 透明物体 **按距离排序**（Back-to-Front Rendering）
- **帧缓冲对象**（Framebuffer Object, FBO）
- **屏幕四边形**（Screen Quad）二次绘制
- 双阶段渲染流程（Two-pass Rendering Pipeline）
- 为后续后期处理打下基础

- [进入项目](./demos/07_OpenGL_Advanced_GL)

> 后续 demo 持续更新中……
