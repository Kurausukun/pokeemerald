Music and sound hacking in general is one of the most common forms of hacking for these games, yet you generally have to know how the entire sound system works before you can really take advantage of it. This guide is meant to touch upon nearly every aspect of the sound engine in the GBA Pokemon games so that you can fully take advantage of it when creating new music or sound.

## About the Engine

The first thing to know is which engine the game uses at all. GBA games use a variety of different proprietary sound engines, but Pokemon uses the most common engine used by first-party Nintendo games because it is included in the GBA SDK, known as m4a (music 4 advance), mp2k (music player two thousand), or by the name more popular within the community, "Sappy." The engine itself is fully decompiled in the repository, but it is important to note that part of the driver is hand-written assembly. This means that it will never be "decompiled" since it was never compiled to begin with. Generally, you will not need to make any edits to the engine driver itself unless you're doing something really crazy (in which case you probably don't need this guide), but it's important to know that part of it is in ASM.

### The Engine's Settings

The m4a engine is configurable upon initialization; that is to say, you can customize certain aspects of the engine to suit your needs for a particular game. These settings include the amount of reverb the engine produces, whether or not it produces reverb at all, the maximum number of samples that can be played at once (AKA the maximum polyphony for directsound samples), the master volume for the directsound channels, the rate the engine runs at (AKA the sample rate), and the amount of bits for the final DAC (can be anywhere between 6 and 9-bit). All GBA Pokemon games use the same settings for the engine: default reverb value (0), reverb ON, 5 simultaneous directsound channels, master volume 12, default engine frequency (13379Hz), and 8-bit DAC. The call that sets these settings can be found [in this line of `m4a.c`](https://github.com/pret/pokeemerald/blob/master/src/m4a.c#L78). If you want to change any of these settings to better suit your needs for sound, this is where you do it. However, it is not wise to change them without knowing the consequences first; these consequences will be explained in subsequent sections of this guide.

## What Goes into a Song

Before anything else, I think it would be beneficial to talk about every component that is necessary to produce an audible song in-game. We need:
- A sequence to tell the game which notes to play and how to play them. This is accomplished using standard MIDI files.
- A voicegroup to tell the game which instruments to use for a song. These define what MIDI calls "program numbers," which is just a way of referring to an instrument.
- Samples for the voicegroups to point to. These are actual, raw audio files that the game will play back, and the voicegroup tells the engine exactly how to play it back.

The rest of this guide will explain how to modify these components.

## Using the Engine

Now that we know about the engine itself, we can try and use it to create music. Oftentimes, the first thing someone will do when trying to insert a new song into the game is to grab a midi, convert it as-is, place it into the game over some other song, and see how it sounds. However, when they do this, most people will experience the song not sounding right at all; some instruments will be completely wrong, some will be incomprehensible beeping, and others will be missing entirely. To understand why this happens, I think the best thing to look at first is the concept of voice groups.

### Voice Groups

Put simply, a voice group is what determines what instruments a song can play and how those instruments are played. All of the game's voice groups are located in `sound/voicegroups/` and each voice group is split into its own file. When you open these files, you might be intimidated by the somewhat incomprehensible barrage of letters and numbers you see, so let us look at how the files are formatted.

The general format of a voice group is as follows:

```
label::
    entry0
    entry1
    ...
    entry127
```
Now I will explain what each of those are. `label` is the name of the voice group. Think of it like the name of a variable. When the engine needs to identify a voice group, this is what it goes by. In pokeemerald, all voice groups are just named `voicegroup###`, where ### is just some number depending on which voice group it is. Now for the bigger part of the voice groups, `entry`. An `entry` in a voice group is a definition of a specific instrument. You can think of it like a midi "program" number. The program number it occupies is simply defined by the order it is defined in the list. The first entry is program 0, the second entry is program 1, etc. The maximum number of entries for a voice group is 128, so that means you can use programs 0-127, just like midi. Now we will look at what those instruments can be.

### Types of Instruments

An `entry` can be one of the following: a directsound instrument, a keysplit instrument, a drumset/keysplit_all, a square1, a square2, a programmable wave, or a noise. Here is an explanation for each one. The definitions for these macros can be found in [music_voice.inc](https://github.com/pret/pokeemerald/blob/master/asm/macros/music_voice.inc).

First, we will explain `directsound`. "directsound" is just the name the engine uses for samples as opposed to sounds generated from the GB channels (no relation to Microsoft DirectSound, the name is a coincidence). A "directsound" entry specifies that an instrument is a single audio sample. The format of a directsound definition is as follows:
`voice_directsound base_midi_key, pan, sample_data_pointer, attack, decay, sustain, release`

`voice_directsound` must come first in the line to let the engine know we are defining a single directsound instrument. `base_midi_key` is the key at which the note will be played when in a drumkit. Outside of a drumkit, this value has no effect (because the key of the instrument is determined by the note of the song that's playing it in that case). Therefore, for any instrument that is not part of a drumkit, this must always be 60 (middle C); even if you define a different value, it will be treated as if the value were 60. This is a limitation of the engine; you cannot define a regular instrument at anything besides middle C. For this reason, if your instrument sample is not played at middle C, you will have to change it so that it is. The method for accomplishing this will be detailed in a later section; for now, just remember that your sample must be played at middle C unless it is part of a drumkit.

`pan` is how much the instrument is panned to the left/right. This value also only applies to drumkit instruments and has no effect outside of drumkits. A value of 0 means center, negative values are panned to the left, and positive values are panned to the right.

`sample_data_pointer` is the name that is defined for the sample. These definitions are found in `sound/direct_sound_data.inc.`

`attack`, `decay`, `sustain`, and `release` are the standard parameters of an ADSR envelope. These all have values 0-255. For those not familiar, `attack` is how quickly the sample reaches its maximum volume (0 takes the longest, 255 takes the shortest), `decay` is how long it takes to reach the `sustain` value, `sustain` is the volume the note is held at before the `release`, and `release` is how long it takes the note to fade to silence after it stops playing. Note that the curves generated by these values are not linear. In general, unless you know your way around ADSR, it is best to just mimic what the game does depending on the type of instrument being used.

Next is `keysplit`. A keysplit is a way to split an instrument into multiple samples depending on what key is played. Pitching a sample up or down too much will make it sound distorted and ugly, so these are necessary when using instruments that have a large range of pitches. The definition of a keysplit is as follows:
`voice_keysplit voicegroup table`

`voicegroup` is a pointer to a voicegroup that defines which samples are used in the keysplit, and `table` is a pointer to a keysplit table that defines which notes play which samples. Keysplit tables are defined in `sound/keysplit_tables.inc`. The games have 5 keysplit instruments by default: the piano (5 splits), the strings (3 splits), the trumpet (3 splits), the tuba (2 splits), and the French horn (2 splits). As an example, let us look at the entry for the strings. The entry in a voicegroup looks like:
`voice_keysplit voicegroup006, KeySplitTable2`
So now let us look at voicegroup006; this is where we will see which samples it uses. Looking at it, we see:
```
voicegroup006::
	voice_directsound 60, 0, DirectSoundWaveData_sc88pro_string_ensemble_60, 255, 0, 255, 196
	voice_directsound 60, 0, DirectSoundWaveData_sc88pro_string_ensemble_72, 255, 0, 255, 196
	voice_directsound 60, 0, DirectSoundWaveData_sc88pro_string_ensemble_84, 255, 0, 255, 196
```
This tells us that the three samples this keysplit uses are `DirectSoundWaveData_sc88pro_string_ensemble_60`, `DirectSoundWaveData_sc88pro_string_ensemble_72`, and `DirectSoundWaveData_sc88pro_string_ensemble_84`. If we listen to those samples, we can see that they are all string ensembles at three different octaves. To see how the notes are distributed, we must look at the keysplit table. In the definition we saw that it used `KeySplitTable2`, so let us look at that:
```
.set KeySplitTable2, . - 36
	.byte 0  @ 36
	.byte 0  @ 37
	.byte 0  @ 38
	.byte 0  @ 39
...
	.byte 0  @ 66
	.byte 0  @ 67
	.byte 0  @ 68
	.byte 1  @ 69
	.byte 1  @ 70
	.byte 1  @ 71
	.byte 1  @ 72
	.byte 1  @ 73
	.byte 1  @ 74
	.byte 1  @ 75
	.byte 1  @ 76
	.byte 1  @ 77
	.byte 1  @ 78
	.byte 1  @ 79
	.byte 1  @ 80
	.byte 2  @ 81
	.byte 2  @ 82
	.byte 2  @ 83
	.byte 2  @ 84
	.byte 2  @ 85
	.byte 2  @ 86
	.byte 2  @ 87
...
	.byte 2  @ 106
	.byte 2  @ 107
```
First of all, we must take note of the `- 36` at the start; this means that the keysplit starts at midi note 36 (C, two octaves below middle C). The number after `.byte` defines which sample gets played for that particular note. We can see that `.byte 0` is used until entry number 69, which means that the first sample is used for notes 36-68 (C2-G#4, assuming C4 is middle C). `.byte 1` is used for entries 69-80 (A4-G#5), and `.byte 2` is used for entries 81-107 (A5-B7). These are our three splits; the first sample is used for all notes less than 69, the second sample is used for all notes between 69 and 80 (inclusive), and the third sample is used for all notes greater than 80 and less than 107. By following this format, you can make your own keysplit instruments as well.

There is another type of keysplit we can use, although it usually goes by a different name. That would be `voice_keysplit_all`, better known as a drumkit. This works similarly to a regular keysplit, except there is no keysplit table associated. Instead, the argument is simply a pointer to another voicegroup, so the format of a definition is simply `voice_keysplit_all voicegroup###`. Instead of mapping specific ranges like a standard keysplit, every single note is its own "range" in a `keysplit_all`. This is why they are commonly used as drumkits and came to be known as such. In this context, the `base_midi_key` and `pan` arguments finally become relevant. As an example, the first entry in the voicegroup that we point to becomes midi note 0, the second one becomes midi note 1, and etc. all the way up to midi note 127.
