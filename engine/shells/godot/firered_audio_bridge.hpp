#ifndef ENGINE_SHELLS_GODOT_FIRERED_AUDIO_BRIDGE_HPP
#define ENGINE_SHELLS_GODOT_FIRERED_AUDIO_BRIDGE_HPP

extern "C" {
#include "../../interfaces/audio_backend.h"
}

namespace godot {

class FireRedAudioBridge {
public:
    static void consume_runtime_audio(const EngineAudioBuffer &buffer);
};

}

#endif
