#pragma once
#include <render/types.h>
#include <utils/String.h>
#include <utils/ILogListener.h>

enum EShLanguage;

namespace glslang {
    class TShader;
};

namespace render {
    namespace vulkan {
        class ShaderCompiler : public utils::IWithLogging  {
            public:
                ShaderCompiler();
                ~ShaderCompiler();

                bool init();
                void shutdown();

                glslang::TShader* compileShader(const utils::String& source, EShLanguage type) const;
            
            protected:
                bool m_isInitialized;
        };
    };
};