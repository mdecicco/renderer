#pragma once
#include <render/types.h>

#include <utils/String.h>
#include <utils/ILogListener.h>
#include <glslang/Public/ShaderLang.h>

namespace render {
    namespace vulkan {
        class ShaderCompiler : public ::utils::IWithLogging  {
            public:
                ShaderCompiler();
                ~ShaderCompiler();

                bool init();
                void shutdown();

                glslang::TShader* compileShader(const String& source, EShLanguage type);
            
            protected:
                bool m_isInitialized;
        };
    };
};