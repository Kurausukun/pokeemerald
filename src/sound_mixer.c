#include "global.h"
#include "music_player.h"
#include "sound_mixer.h"
#include "mp2k_common.h"

#define VCOUNT_VBLANK 160
#define TOTAL_SCANLINES 228

static inline void GenerateAudio(struct SoundMixerState *mixer, struct MixerSource *chan, struct WaveData2 *wav, s32 *outBuffer, u16 samplesPerFrame, s32 sampleRateReciprocal);
void SampleMixer(struct SoundMixerState *mixer, u32 scanlineLimit, u16 samplesPerFrame, s32 *outBuffer, u8 dmaCounter, u16 maxBufSize);
static inline bool32 TickEnvelope(struct MixerSource *chan, struct WaveData2 *wav);

void RunMixerFrame(void) {
	struct SoundMixerState *mixer = SOUND_INFO_PTR;
	
	if (mixer->lockStatus != MIXER_UNLOCKED) {
		return;
	}
	mixer->lockStatus = MIXER_LOCKED;
	
	u32 maxScanlines = mixer->maxScanlines;
	if (mixer->maxScanlines != 0) {
		u32 vcount = REG_VCOUNT;
		maxScanlines += vcount;
		if (vcount < VCOUNT_VBLANK) {
			maxScanlines += TOTAL_SCANLINES;
		}
	}
	
	if (mixer->firstPlayerFunc != NULL) {
		mixer->firstPlayerFunc(mixer->firstPlayer);
	}
	
	mixer->cgbMixerFunc();
	
	s32 samplesPerFrame = mixer->samplesPerFrame;
	s32 *outBuffer = mixer->outBuffer;
	s32 dmaCounter = mixer->dmaCounter;
	
	if (dmaCounter > 1) {
		outBuffer += samplesPerFrame * (mixer->framesPerDmaCycle - (dmaCounter - 1));
	}
	
	//MixerRamFunc mixerRamFunc = ((MixerRamFunc)MixerCodeBuffer);
	SampleMixer(mixer, maxScanlines, samplesPerFrame, outBuffer, dmaCounter, MIXED_AUDIO_BUFFER_SIZE);
}



//__attribute__((target("thumb")))
void SampleMixer(struct SoundMixerState *mixer, u32 scanlineLimit, u16 samplesPerFrame, s32 *outBuffer, u8 dmaCounter, u16 maxBufSize) {
	u32 reverb;
	if (reverb = mixer->reverb) {
		// The vanilla reverb effect outputs a mono sound from four sources:
		//  - L/R channels as they were mixer->framesPerDmaCycle frames ago
		//  - L/R channels as they were (mixer->framesPerDmaCycle - 1) frames ago
		s32 *tmp1 = outBuffer;
		s32 *tmp2;
		if (dmaCounter == 2) {
			tmp2 = mixer->outBuffer;
		} else {
			tmp2 = outBuffer + samplesPerFrame;
		}
		uf16 i = 0;
		do {
			int s = tmp1[0] + tmp1[maxBufSize] + tmp2[0] + tmp2[maxBufSize];
			s *= reverb;
			s = FLOOR_DIV_POW2(s, 512);
			s += (s < 0);
			tmp1[0] = tmp1[maxBufSize] = s;
			tmp1++;
			tmp2++;
		}
		while(++i < samplesPerFrame);
	} else {
		// memset(outBuffer, 0, samplesPerFrame);
		// memset(outBuffer + maxBufSize, 0, samplesPerFrame);
		for (int i = 0; i < samplesPerFrame; i++) {
			outBuffer[maxBufSize + i] = outBuffer[i] = 0;
		}
	}
	
	s32 sampleRateReciprocal = mixer->sampleRateReciprocal;
	sf8 numChans = mixer->numChans;
	struct MixerSource *chan = mixer->chans;
	
	for (int i = 0; i < numChans; i++, chan++) {
		struct WaveData2 *wav = chan->wav;
		
		if (scanlineLimit != 0) {
			uf16 vcount = REG_VCOUNT;
			if (vcount < VCOUNT_VBLANK) {
				vcount += TOTAL_SCANLINES;
			}
			if (vcount >= scanlineLimit) {
				goto returnEarly;
			}
		}
		
		if (TickEnvelope(chan, wav)) 
		{
			GenerateAudio(mixer, chan, wav, outBuffer, samplesPerFrame, sampleRateReciprocal);
		}
	}
returnEarly:
	mixer->lockStatus = MIXER_UNLOCKED;
}

// Returns TRUE if channel is still active after moving envelope forward a frame
//__attribute__((target("thumb")))
static inline bool32 TickEnvelope(struct MixerSource *chan, struct WaveData2 *wav) {
	// MP2K envelope shape
	//                                                                 |
	// (linear)^                                                       |
	// Attack / \Decay (exponential)                                   |
	//       /   \_                                                    |
	//      /      '.,        Sustain                                  |
	//     /          '.______________                                 |
	//    /                           '-.       Echo (linear)          |
	//   /                 Release (exp) ''--..|\                      |
	//  /                                        \                     |
	
	u8 status = chan->status;
	if ((status & 0xC7) == 0) {
		return FALSE;
	}
	
	if ((status & 0x80) == 0) {
		u8 env = chan->envelopeVol;
		
		if (status & 4) {
			// Note-wise echo
			--chan->echoVol;
			if (chan->echoVol <= 0) {
				chan->status = 0;
				return FALSE;
			} else {
				return TRUE;
			}
		} else if (status & 0x40) {
			// Release
			chan->envelopeVol = env * chan->release / 256U;
			u8 echoVol = chan->echoVol;
			if (chan->envelopeVol > echoVol) {
				return TRUE;
			} else if (echoVol == 0) {
				chan->status = 0;
				return FALSE;
			} else {
				chan->status |= 4;
				return TRUE;
			}
		}
		
		switch (status & 3) {
		case 2:
			// Decay
			chan->envelopeVol = env * chan->decay / 256U;
			
			u8 sustain = chan->sustain;
			if (chan->envelopeVol <= sustain && sustain == 0) {
				// Duplicated echo check from Release section above
				if (chan->echoVol == 0) {
					chan->status = 0;
					return FALSE;
				} else {
					chan->status |= 4;
					return TRUE;
				}
			} else if (chan->envelopeVol <= sustain) {
				chan->envelopeVol = sustain;
				--chan->status;
			}
			break;
		case 3:
		{

			unsigned newEnv = env + chan->attack;
		attack:
			if (newEnv > 0xFF) {
				chan->envelopeVol = 0xFF;
				--chan->status;
			} else {
				chan->envelopeVol = newEnv;
			}
			break;
		}
		case 1: // Sustain
		default:
			break;
		}
		
		return TRUE;
	} else if (status & 0x40) {
		// Init and stop cancel each other out
		chan->status = 0;
		return FALSE;
	} else {
		// Init channel
		chan->status = 3;
#ifdef POKEMON_EXTENSIONS
		chan->current = wav->data + chan->ct;
		chan->ct = wav->size - chan->ct;
#else
		chan->current = wav->data;
		chan->ct = wav->size;
#endif
		chan->fw = 0;
		chan->envelopeVol = 0;
		if (wav->loopFlags & 0xC0) {
			chan->status |= 0x10;
		}
		goto attack;
	}
}

//__attribute__((target("thumb")))
static inline void GenerateAudio(struct SoundMixerState *mixer, struct MixerSource *chan, struct WaveData2 *wav, s32 *outBuffer, u16 samplesPerFrame, s32 sampleRateReciprocal) {/*, [[[]]]) {*/
	uf8 v = chan->envelopeVol * (mixer->masterVol + 1) / 16U;
	chan->envelopeVolR = chan->rightVol * v / 256U;
	chan->envelopeVolL = chan->leftVol * v / 256U;
	
	s32 loopLen = 0;
	s8 *loopStart;
	if (chan->status & 0x10) {
		loopStart = wav->data + wav->loopStart;
		loopLen = wav->size - wav->loopStart;
	}
	s32 samplesLeftInWav = chan->ct;
	s8 *current = chan->current;
	signed envR = chan->envelopeVolR;
	signed envL = chan->envelopeVolL;
#ifdef POKEMON_EXTENSIONS
	// TODO
	if (chan->type & 0x30) {
		GeneratePokemonSampleAudio([[[]]]);
		// set finePos, ct, current
		return;
	}
#endif
	if (chan->type & 8) {
		for (u16 i = 0; i < samplesPerFrame; i++, outBuffer++) {
			sf8 c = *(current++);
			
			outBuffer[0] += (c * envR) << 16;
			outBuffer[MIXED_AUDIO_BUFFER_SIZE] += (c * envL, 256) << 16;
			
			if (--samplesLeftInWav == 0) {
				samplesLeftInWav = loopLen;
				if (loopLen != 0) {
					current = loopStart;
				} else {
					chan->status = 0;
					return;
				}
			}
		}
		
		chan->ct = samplesLeftInWav;
		chan->current = current;
	} else {
		// These are 9.23 fixed width decimals, just like sampleRateReciprocal
		// finePos is expected to have any value on [0.0, 1.0) but may contain a very large positive or
		// negative number in some extraordinary circumstances when romSamplesPerOutputSample is huge.
		// sampleRateReciprocal can be at most 1463 / (2^23) -- when the sample rate is 5734 Hz
		// chan->freq depends on the sample in question; it can be at most 0.943667 * the C14 frequency
		// of the sample.
		// Because of this, we know that romSamplesPerOutputSample would normally max out at 353'429
		// (0x56495) and it has more than enough potential to overflow important calculations.
		// Be smart about how high your samples' sample rates are. I don't have a concrete number to
		// suggest at this time.
		s32 finePos = chan->fw;
		s32 romSamplesPerOutputSample = chan->freq * sampleRateReciprocal;
		
		sf16 b = current[0];
		sf16 m = current[1] - b;
		current += 1;
		
		for (u16 i = 0; i < samplesPerFrame; i++, outBuffer++) {
			// Use linear interpolation to calculate a value between the current sample in the wav
			// and the next sample. Also cancel out the 9.23 stuff
			s32 sample = FLOOR_DIV_POW2(finePos * m, 1 << 23) + b;
			
			outBuffer[0] += (sample * envR) << 16;
			outBuffer[MIXED_AUDIO_BUFFER_SIZE] += (sample * envL) << 16;
			
			finePos += romSamplesPerOutputSample;
			unsigned newCoarsePos = (u32)finePos / (1U << 23);
			if (newCoarsePos != 0) {
				// It's possible that finePos will have overflowed or become a very large positive
				// number because the two most significant bits are not actually cleared in the original
				// assembly code.
#ifdef ORIGINAL_COARSE_POSITION_CLEARING
				finePos = finePos & ~(0x7F << 23);
#else
				finePos = finePos & ((1 << 23) - 1);
#endif
				
				samplesLeftInWav -= newCoarsePos;
				if (samplesLeftInWav <= 0) {
					if (loopLen != 0) {
						current = loopStart;
						newCoarsePos = -samplesLeftInWav;
						samplesLeftInWav += loopLen;
						while (samplesLeftInWav <= 0) {
							newCoarsePos -= loopLen;
							samplesLeftInWav += loopLen;
						}
						b = current[newCoarsePos];
						m = current[newCoarsePos + 1] - b;
						current += newCoarsePos + 1;
					} else {
						chan->status = 0;
						return;
					}
				} else {
					b = current[newCoarsePos - 1];
					m = current[newCoarsePos] - b;
					current += newCoarsePos;
				}
			}
		}
		
		chan->fw = finePos;
		chan->ct = samplesLeftInWav;
		chan->current = current - 1;
	}
}
