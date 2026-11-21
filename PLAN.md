# AllegroScore Development Plan

## Phase 1: Foundation & Architecture
- **Tech Stack**: C++ with Qt 6 for UI and core logic (Cross-platform support).
- **Notation Engine**: **Verovio** planned for MusicXML→SVG rendering. **MockScoreRenderer** is currently used as a placeholder since Verovio isn't installed on this system (no root for apt). The `ScoreRenderer` abstract base class allows a clean drop-in swap to `VerovioScoreRenderer` once Verovio is available.
- **File Format**: Standardize on MusicXML as the internal data representation for AI interoperability.
- **Status**: Basic project structure created with CMake and main UI skeleton with score renderer abstraction.

## Phase 2: AI Integration (The "Composer's Assistant")
- **AI Backend**: Connect to FreeLLMAPI. Preferred models:
    - **Claude 3.5 Sonnet**: Best for complex reasoning, music theory explanations, and structured MusicXML manipulation.
    - **GPT-4o**: Alternative for creative brainstorming and stylistic suggestions.
- **Data Flow**: 
    - User selects a section of the score.
    - Application sends MusicXML snippet + context (style, goal) to AI.
    - AI returns modified MusicXML + Theory Explanation.
- **Overlay System**: Implement the "Different Color" overlay for AI suggestions to allow A/B testing before committing to the main score.

## Phase 3: Core UI & Playback
- **Interface**: Split-pane view with Score Editor and AI Chat Frame.
- **Playback**: Basic MIDI playback using internal synth (e.g., FluidSynth) and standard soundfonts.
- **Tooling**: Basic notation entry (notes, rests, dynamics).

## Phase 4: Prototyping "Bach" & Stylistic Assistants
- Develop prompt templates for specific styles (Bach, Haydn, etc.).
- Implement "Error Scanning" using a hybrid of rule-based logic (for range/voice leading) and AI (for stylistic consistency).

## Phase 5: Testing & Deployment
- Linux build (AppImage/Flatpak).
- Initial macOS/Windows cross-compilation checks.
