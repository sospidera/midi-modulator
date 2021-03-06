midi-modulator
==============

Fun little tool to swap major/minor tonality of a MIDI file given the original key.

Runs from the terminal using the format: 

[application-name] [input-midi-file-path] [original-song-key]

where [original-song-key] is any valid key signature of the form <tonic><tonality>, or: (C|D|E|F|G|A|B)(b|#)?m?

Example: MidiKeyChange.exe midis/losing-my-religion.mid Am

Currently works on most MIDI files, but throws an error on certain files, potentially because of atypical formatting.

Future work:
- Improve error handling/reporting (command-line argument parsing, file parsing)
- Generalize to modes: Dorian, Lydian, etc.
- Adress pitch bending: currently retains bend-size regardless of new transposed key
