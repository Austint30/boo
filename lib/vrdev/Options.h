//
// Created by austin on 1/30/22.
//

#include <string>

namespace boo {
    struct Options {
        std::string GraphicsPlugin;

        std::string FormFactor{"Hmd"};

        std::string ViewConfiguration{"Stereo"};

        std::string EnvironmentBlendMode{"Opaque"};

        std::string AppSpace{"Local"};
    };
}

