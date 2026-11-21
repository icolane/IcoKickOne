# IcoKickOne

```text
IcoKickOne/
|-- CMakeLists.txt
|-- CMakePresets.json
|-- README.md
|-- include/IcoKickOne/     # Public DSP types and interface
|-- src/dsp/                # Kick synthesis and module processing
|-- src/plugin/             # VST3 processor and parameter state
`-- src/ui/                 # Editor and waveform preview
```

IcoKickOne is an original VST3 kick instrument inspired by the general workflow
shown in the linked kick-builder video. It is not a copy of that commercial
plugin: it has its own interface and procedural DSP, and includes no borrowed
samples, presets, artwork, or code.

## What it does

- Generates a kick from MIDI notes using a pitch envelope, body oscillator,
  transient noise, and a controlled sub tail.
- Provides nine broad kick-character presets, from Schranz and industrial to
  cleaner and hardgroove sounds.
- Exposes pitch, sweep, decay, body, click, rumble, drive, and output controls
  to the host for automation.
- Includes nine independently switchable processing modules, a waveform
  preview, a one-click randomizer, pitch tracking, and output compensation.
- Stores its current parameter state in the DAW's normal VST3 preset/state
  mechanism.

The sound engine is procedural. It does not depend on a large sample library,
and deliberately avoids bit-crushing or high-pitched digital effects.

## Build on Windows

Requirements: MinGW-w64 with GCC, Git, Ninja, and CMake 3.22 or newer. CMake downloads
JUCE 7.0.12 into the local build tree on the first configure. JUCE is licensed
under AGPL-3.0 or a commercial license; this source project is intended to be
used and distributed under AGPL-3.0-or-later.

From this directory, with MinGW-w64 and Ninja on `PATH`:

```powershell
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
```

The VST3 bundle is written under `build/IcoKickOne_artefacts/Release/VST3`.
The build does not copy or install the plugin into a system VST folder.

## Use

Create a MIDI track in Live, load IcoKickOne as a VST3 instrument, and trigger
it with MIDI notes. C2 is the reference note; with pitch tracking on, MIDI
notes transpose the kick relative to it. The preview button auditions C2.

The style selector chooses a starting character. Use the exposed controls to
shape it further; use Randomize for a new parameter variation. The nine module
switches are independently automatable. Saved plugin state is restored by Live
when the set is reopened.

## Download

The Windows x64 and universal macOS VST3 releases are available on the
[GitHub Releases page](https://github.com/icolane/IcoKickOne/releases/latest).
Extract `IcoKickOne.vst3` from the ZIP into a VST3 folder scanned by your DAW.
The macOS build supports macOS 11 or later on both Intel and Apple Silicon.
Each ZIP also contains this README and the license.

## License

IcoKickOne is licensed under the GNU Affero General Public License v3.0 or later
(AGPL-3.0-or-later). See [LICENSE](LICENSE) for the full license text.
