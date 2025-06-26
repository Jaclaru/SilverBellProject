#pragma once

namespace SilverBell::Renderer
{
    class RendererBase
    {
    public:

        RendererBase() = default;

        virtual ~RendererBase() = default;

        // 禁用拷贝构造和赋值操作
        RendererBase(const RendererBase&) = delete;
        RendererBase& operator=(const RendererBase&) = delete;
        // 移动构造和赋值操作
        RendererBase(RendererBase&&) = default;
        RendererBase& operator=(RendererBase&&) = default;


        /**
         * @brief 初始化渲染器
         */
        virtual void Initialize() = 0;

        /**
         * @brief 设置Surface扩展
         */
        virtual void SetSurfaceExt(const char** Exts, int Len) = 0;

        virtual void CleanUp() = 0;
    };

}
