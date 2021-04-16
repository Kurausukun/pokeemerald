	.include "asm/macros.inc"
	.include "constants/gba_constants.inc"
	.include "constants/m4a_constants.inc"

	.syntax unified

	.text

	thumb_func_start umul3232H32
umul3232H32:
	adr r2, __umul3232H32
	bx r2
	.arm
__umul3232H32:
	umull r2, r3, r0, r1
	add r0, r3, 0
	bx lr
	thumb_func_end umul3232H32

	thumb_func_start SoundMain
SoundMain:
	ldr r0, lt_SOUND_INFO_PTR
	ldr r0, [r0]
	ldr r2, lt_ID_NUMBER
	ldr r3, [r0, o_SoundInfo_ident]
	cmp r2, r3
	beq SoundMain_1
	bx lr @ Exit the function if ident doesn't match ID_NUMBER.
SoundMain_1:
	adds r3, 1
	str r3, [r0, o_SoundInfo_ident]
	push {r4-r7,lr}
	mov r1, r8
	mov r2, r9
	mov r3, r10
	mov r4, r11
	push {r0-r4}
	sub sp, 0x18
	ldrb r1, [r0, o_SoundInfo_maxLines]
	cmp r1, 0 @ if maxLines is 0, there is no maximum
	beq SoundMain_3
	ldr r2, lt_REG_VCOUNT
	ldrb r2, [r2]
	cmp r2, VCOUNT_VBLANK
	bhs SoundMain_2
	adds r2, TOTAL_SCANLINES
SoundMain_2:
	adds r1, r2
SoundMain_3:
	str r1, [sp, 0x14]
	ldr r3, [r0, o_SoundInfo_MPlayMainHead]
	cmp r3, 0
	beq SoundMain_4
	ldr r0, [r0, o_SoundInfo_musicPlayerHead]
	bl call_r3
	ldr r0, [sp, 0x18]
SoundMain_4:
	ldr r3, [r0, o_SoundInfo_CgbSound]
	bl call_r3
	ldr r0, [sp, 0x18]
	ldr r3, [r0, o_SoundInfo_pcmSamplesPerVBlank]
	mov r8, r3
	ldr r5, lt_o_SoundInfo_pcmBuffer
	adds r5, r0
	ldrb r4, [r0, o_SoundInfo_pcmDmaCounter]
	subs r7, r4, 1
	bls SoundMain_5
	ldrb r1, [r0, o_SoundInfo_pcmDmaPeriod]
	subs r1, r7
	mov r2, r8
	muls r2, r1
	adds r5, r2
SoundMain_5:
	str r5, [sp, 0x8]
	ldr r6, lt_PCM_DMA_BUF_SIZE
	ldr r3, lt_SoundMainRAM_Buffer
	bx r3

	.align 2, 0
lt_SOUND_INFO_PTR:        .word SOUND_INFO_PTR
lt_ID_NUMBER:             .word ID_NUMBER
lt_SoundMainRAM_Buffer:   .word SoundMainRAM_Buffer + 1
lt_REG_VCOUNT:            .word REG_VCOUNT
lt_o_SoundInfo_pcmBuffer: .word o_SoundInfo_pcmBuffer
lt_PCM_DMA_BUF_SIZE:      .word PCM_DMA_BUF_SIZE
	thumb_func_end SoundMain

/* HQ-Mixer rev 4.0 created by ipatix (c) 2021
 * licensed under GPLv3, see LICENSE.txt for details */

    .equ    ENABLE_REVERB, 1                        @ <-- if you want faster code or don't like reverb, set this to '0', set to '1' otherwise
    .equ    ENABLE_DMA, 1                           @ <-- Using DMA produces smaller code and has better performance. Disable it if your case does not allow to use DMA.

    /*****************
     * END OF CONFIG *
     *****************/

    /* NO USER SERVICABLE CODE BELOW HERE! YOU HAVE BEEN WARNED */

    /* globals */
    .global SoundMainRAM

    .equ    FRAME_LENGTH_5734, 0x60
    .equ    FRAME_LENGTH_7884, 0x84             @ THIS MODE IS NOT SUPPORTED BY THIS ENGINE BECAUSE IT DOESN'T USE AN 8 ALIGNED BUFFER LENGTH
    .equ    FRAME_LENGTH_10512, 0xB0
    .equ    FRAME_LENGTH_13379, 0xE0            @ DEFAULT
    .equ    FRAME_LENGTH_15768, 0x108
    .equ    FRAME_LENGTH_18157, 0x130
    .equ    FRAME_LENGTH_21024, 0x160
    .equ    FRAME_LENGTH_26758, 0x1C0
    .equ    FRAME_LENGTH_31536, 0x210
    .equ    FRAME_LENGTH_36314, 0x260
    .equ    FRAME_LENGTH_40137, 0x2A0
    .equ    FRAME_LENGTH_42048, 0x2C0

    /* stack variables */
    .equ    ARG_FRAME_LENGTH, 0x0               @ Number of samples per frame/buffer
    .equ    ARG_REMAIN_CHN, 0x4                 @ temporary to count down the channels to process
    .equ    ARG_BUFFER_POS, 0x8                 @ stores the current output buffer pointer
    .equ    ARG_LOOP_START_POS, 0xC             @ stores wave loop start position in channel loop
    .equ    ARG_LOOP_LENGTH, 0x10               @   ''    ''   ''  end position
    .equ    ARG_BUFFER_POS_INDEX_HINT, 0x14     @ if this value is == 2, then this is the last buffer before wraparound
    .equ    ARG_PCM_STRUCT, 0x18                @ pointer to engine the main work area

    /* channel struct */
    .equ    CHN_SAMPLE_STOR, 0x3F               @ [byte] contains the previously loaded sample from the linear interpolation

    /* pulse wave synth configuration offset */
    .equ    SYNTH_TYPE, 0x1                     @ [byte]
    .equ    SYNTH_BASE_WAVE_DUTY, 0x2           @ [byte]
    .equ    SYNTH_WIDTH_CHANGE_1, 0x3           @ [byte]
    .equ    SYNTH_MOD_AMOUNT, 0x4               @ [byte]
    .equ    SYNTH_WIDTH_CHANGE_2, 0x5           @ [byte]

    .equ    MODE_FLGSH_SIGN_REVERSE, 27         @ shift by n bits to get the reverse flag into SIGN

    /* variables of the engine work area */
    .equ    VAR_REVERB, 0x5                     @ [byte] 0-127 = reverb level
    .equ    VAR_MAX_CHN, 0x6                    @ [byte] maximum channels to process
    .equ    VAR_MASTER_VOL, 0x7                 @ [byte] PCM master volume
    .equ    VAR_EXT_NOISE_SHAPE_LEFT, 0xE       @ [byte] normally unused, used here for noise shaping
    .equ    VAR_EXT_NOISE_SHAPE_RIGHT, 0xF      @ [byte] normally unused, used here for noise shaping
    .equ    VAR_DEF_PITCH_FAC, 0x18             @ [word] this value get's multiplied with the samplerate for the inter sample distance
    .equ    VAR_FIRST_CHN, 0x50                 @ [CHN struct] relative offset to channel array
    .equ    VAR_PCM_BUFFER, 0x350

    /* just some more defines */
    .equ    ARM_OP_LEN, 0x4

    /* extensions */
    .equ    BDPCM_BLK_STRIDE, 0x21
    .equ    BDPCM_BLK_SIZE, 0x40
    .equ    BDPCM_BLK_SIZE_MASK, 0x3F
    .equ    BDPCM_BLK_SIZE_SHIFT, 0x6

    .thumb
    .align  2
	.syntax divided

	thumb_func_start SoundMainRAM
SoundMainRAM:
    /* load Reverb level and check if we need to apply it */
    STR     R4, [SP, #ARG_BUFFER_POS_INDEX_HINT]
    /*
     * okay, before the actual mixing starts
     * the volume and envelope calculation takes place
     */
    MOV     R4, R8  @ R4 = buffer length
    /*
     * this stroes the buffer length to a backup location
     */
    STR     R4, [SP, #ARG_FRAME_LENGTH]
    /* init channel loop */
    LDR     R4, [SP, #ARG_PCM_STRUCT]           @ R4 = main work area pointer
    LDR     R0, [R4, #VAR_DEF_PITCH_FAC]        @ R0 = samplingrate pitch factor
    MOV     R12, R0
    LDRB    R0, [R4, #VAR_MAX_CHN]
    ADD     R4, #VAR_FIRST_CHN                  @ R4 = Base channel Offset (Channel #0)

C_channel_state_loop:
    /* this is the main channel processing loop */
    STR     R0, [SP, #ARG_REMAIN_CHN]
    LDR     R3, [R4, #o_SoundChannel_wav]
    LDRB    R6, [R4, #o_SoundChannel_statusFlags]           @ R6 will hold the channel status
    MOVS    R0, #0xC7                       @ check if any of the channel status flags is set
    TST     R0, R6                          @ check if none of the flags is set
    BEQ     C_skip_channel
    /* check channel flags */
    LSL     R0, R6, #25                     @ shift over the SOUND_CHANNEL_SF_START to CARRY
    BCC     C_adsr_echo_check               @ continue with normal channel procedure
    /* check leftmost bit */
    BMI     C_stop_channel                  @ SOUND_CHANNEL_SF_START | SOUND_CHANNEL_SF_STOP -> stop directly
    /* channel init procedure */
    MOVS    R6, #SOUND_CHANNEL_SF_ENV_ATTACK
    /* enabled compression if sample flag is set */
    MOVS    R0, R3                          @ R0 = o_SoundChannel_wav
    ADD     R0, #o_WaveData_data                  @ R0 = wave data offset
    LDR     R2, [R3, #o_WaveData_size]
    CMP     R2, #0
    BEQ     C_channel_init_synth
    LDRB    R5, [R3, #o_WaveData_type]
    LSL     R5, R5, #31
    LDRB    R5, [R4, #o_SoundChannel_type]
    BMI     C_channel_init_comp
    LSL     R5, R5, #27                     @ shift TONEDATA_TYPE_REV flag to SIGN
    BMI     C_channel_init_noncomp_reverse
    /* Pokemon games seem to init channels differently than other m4a games */
C_channel_init_noncomp_forward:
    LDR     R1, [R4, #o_SoundChannel_count]
    ADD     R0, R1
    SUB     R2, R1
    B       C_channel_init_check_loop
C_channel_init_synth:
    MOV     R5, #TONEDATA_TYPE_SPL
    STRB    R5, [R4, #o_SoundChannel_type]
    LDRB    R1, [R3, #(o_WaveData_data + SYNTH_TYPE)]
    CMP     R1, #2
    BNE     C_channel_init_check_loop
    /* start triangular synth wave at 90 degree phase
     * to avoid a pop sound at the start of the wave */
    MOV     R5, #0x40
    LSL     R5, #24
    STR     R5, [R4, #o_SoundChannel_fw]
    MOV     R5, #0
    B       C_channel_init_check_loop_no_fine_pos
C_channel_init_noncomp_reverse:
.if POKE_CHN_INIT==0
    ADD     R0, R2
.else
    ADD     R0, R2
    LDR     R1, [R4, #o_SoundChannel_count]
    SUB     R0, R1
    SUB     R2, R1
.endif
    B       C_channel_init_check_loop
C_channel_init_comp:
    MOV     R0, #TONEDATA_TYPE_CMP
    ORR     R5, R0
    STRB    R5, [R4, #o_SoundChannel_type]
    LSL     R5, R5, #27                     @ shift TONEDATA_TYPE_REV flag to SIGN
    BMI     C_channel_init_comp_reverse
C_channel_init_comp_forward:
.if POKE_CHN_INIT==0
    MOV     R0, #0
.else
    LDR     R0, [R4, #o_SoundChannel_count]
    SUB     R2, R0
.endif
    B       C_channel_init_check_loop
C_channel_init_comp_reverse:
.if POKE_CHN_INIT==0
    MOV     R0, R2
.else
    LDR     R1, [R4, #o_SoundChannel_count]
    SUB     R2, R1
    MOV     R0, R2
.endif
C_channel_init_check_loop:
    MOVS    R5, #0                          @ initial envelope = #0
    STR     R5, [R4, #o_SoundChannel_fw]
C_channel_init_check_loop_no_fine_pos:
    STR     R0, [R4, #o_SoundChannel_currentPointer]
    STR     R2, [R4, #o_SoundChannel_count]
    STRB    R5, [R4, #o_SoundChannel_envelopeVolume]
    MOV     R2, #CHN_SAMPLE_STOR            @ offset is too large to be used in one instruction
    STRB    R5, [R4, R2]
    /* enabled loop if required */
    LDRB    R2, [R3, #o_WaveData_flags]
    LSR     R0, R2, #6
    BEQ     C_adsr_attack
    /* loop enabled here */
    ADD     R6, #SOUND_CHANNEL_SF_LOOP
    B       C_adsr_attack

C_adsr_echo_check:
    /* this is the normal ADSR procedure without init */
    LDRB    R5, [R4, #o_SoundChannel_envelopeVolume]
    LSL     R0, R6, #29                     @ SOUND_CHANNEL_SF_IEC --> bit 31 (sign bit)
    BPL     C_adsr_release_check
    /* pseudo echo handler */
    LDRB    R0, [R4, #o_SoundChannel_pseudoEchoLength]
    SUB     R0, #1
    STRB    R0, [R4, #o_SoundChannel_pseudoEchoLength]
    BHI     C_channel_vol_calc              @ continue normal if channel is still on

C_stop_channel:
    MOVS    R0, #0
    STRB    R0, [R4, #o_SoundChannel_statusFlags]

C_skip_channel:
    /* go to end of the channel loop */
    B       C_end_channel_state_loop

C_adsr_release_check:
    LSL     R0, R6, #25                      @ SOUND_CHANNEL_SF_STOP --> bit 31 (sign bit)
    BPL     C_adsr_decay_check
    /* release handler */
    LDRB    R0, [R4, #o_SoundChannel_release]
    MUL     R5, R5, R0
    LSR     R5, #8
    BLE     C_adsr_released
    /* pseudo echo init handler */
    LDRB    R0, [R4, #o_SoundChannel_pseudoEchoVolume]
    CMP     R5, R0
    BHI     C_channel_vol_calc

C_adsr_released:
    /* if volume released to #0 */
    LDRB    R5, [R4, #o_SoundChannel_pseudoEchoVolume]
    CMP     R5, #0
    BEQ     C_stop_channel
    /* pseudo echo volume handler */
    MOVS    R0, #SOUND_CHANNEL_SF_IEC
    ORR     R6, R0                          @ set the echo flag
    B       C_adsr_save_and_finalize

C_adsr_decay_check:
    /* check if decay is active */
    MOVS    R2, #(SOUND_CHANNEL_SF_ENV_DECAY+SOUND_CHANNEL_SF_ENV_SUSTAIN)
    AND     R2, R6
    CMP     R2, #SOUND_CHANNEL_SF_ENV_DECAY
    BNE     C_adsr_attack_check             @ decay not active yet
    /* decay handler */
    LDRB    R0, [R4, #o_SoundChannel_decay]
    MUL     R5, R5, R0
    LSR     R5, R5, #8
    LDRB    R0, [R4, #o_SoundChannel_sustain]
    CMP     R5, R0
    BHI     C_channel_vol_calc              @ sample didn't decay yet
    /* sustain handler */
    MOVS    R5, R0                          @ current level = sustain level
    BEQ     C_adsr_released                 @ sustain level #0 --> branch
    /* step to next phase otherweise */
    B       C_adsr_next_state

C_adsr_attack_check:
    /* attack handler */
    CMP     R2, #SOUND_CHANNEL_SF_ENV_ATTACK
    BNE     C_channel_vol_calc              @ if it isn't in attack attack phase, it has to be in sustain (keep vol) --> branch

C_adsr_attack:
    /* apply attack summand */
    LDRB    R0, [R4, #o_SoundChannel_attack]
    ADD     R5, R0
    CMP     R5, #0xFF
    BLO     C_adsr_save_and_finalize
    /* cap attack at 0xFF */
    MOVS    R5, #0xFF

C_adsr_next_state:
    /* switch to next adsr phase */
    SUB     R6, #1

C_adsr_save_and_finalize:
    /* store channel status */
    STRB    R6, [R4, #o_SoundChannel_statusFlags]

C_channel_vol_calc:
    /* store the calculated ADSR level */
    STRB    R5, [R4, #o_SoundChannel_envelopeVolume]
    /* apply master volume */
    LDR     R0, [SP, #ARG_PCM_STRUCT]
    LDRB    R0, [R0, #VAR_MASTER_VOL]
    ADD     R0, #1
    MUL     R5, R0
    /* left side volume */
    LDRB    R0, [R4, #o_SoundChannel_leftVolume]
    MUL     R0, R5
    LSR     R0, #13
    MOV     R10, R0                         @ R10 = left volume
    /* right side volume */
    LDRB    R0, [R4, #o_SoundChannel_rightVolume]
    MUL     R0, R5
    LSR     R0, #13
    MOV     R11, R0                         @ R11 = right volume
    /*
     * Now we get closer to actual mixing:
     * For looped samples some additional operations are required
     */
    MOVS    R0, #SOUND_CHANNEL_SF_LOOP
    AND     R0, R6
    BEQ     C_sample_loop_setup_skip
    /* loop setup handler */
    ADD     R3, #o_WaveData_loopStart
    LDMIA   R3!, {R0, R1}                   @ R0 = loop start, R1 = loop end
    LDRB    R2, [R4, #o_SoundChannel_type]
    LSL     R2, R2, #MODE_FLGSH_SIGN_REVERSE
    BCS     C_sample_loop_setup_comp
    ADD     R3, R0                          @ R3 = loop start position (absolute)
    B       C_sample_loop_setup_finish
C_sample_loop_setup_comp:
    MOV     R3, R0
C_sample_loop_setup_finish:
    STR     R3, [SP, #ARG_LOOP_START_POS]
    SUB     R0, R1, R0

C_sample_loop_setup_skip:
    /* do the rest of the setup */
    STR     R0, [SP, #ARG_LOOP_LENGTH]      @ if loop is off --> R0 = 0x0
    LDR     R5, hq_buffer_literal
    LDR     R2, [R4, #o_SoundChannel_count]
    LDR     R3, [R4, #o_SoundChannel_currentPointer]
    LDRB    R0, [R4, #o_SoundChannel_type]
    ADR     R1, C_mixing_setup
    BX      R1

    .align  2
hq_buffer_literal:
    .word   hq_buffer_ptr

    .arm
    .align  2

    /* register usage:
     * R0:  scratch
     * R1:  scratch
     * R2:  sample countdown
     * R3:  sample pointer
     * R4:  sample step
     * R5:  mixing buffer
     * R6:  sampleval base
     * R7:  sample interpos
     * R8:  frame count
     * R9:  scratch
     * R10: scratch
     * R11: volume
     * R12: sampval diff
     * LR:  scratch */
C_mixing_setup:
    /* frequency and mixing loading routine */
    LDRSB   R6, [R4, #CHN_SAMPLE_STOR]
    LDR     R8, [SP, #ARG_FRAME_LENGTH]
    ORRS    R11, R11, R10, LSL#16           @ R11 = 00LL00RR
    BEQ     C_mixing_epilogue               @ volume #0 --> branch and skip channel processing
    /* normal processing otherwise */
    TST     R0, #(TONEDATA_TYPE_CMP|TONEDATA_TYPE_REV)
    BNE     C_mixing_setup_comp_rev
    TST     R0, #TONEDATA_TYPE_FIX
    BNE     C_setup_fixed_freq_mixing
C_mixing_setup_comp_rev:
    STMFD   SP!, {R4, R9, R12}
    ADD     R4, R4, #o_SoundChannel_fw
    LDMIA   R4, {R7, LR}                    @ R7 = Fine Position, LR = Frequency
    MUL     R4, LR, R12                     @ R4 = inter sample steps = output rate factor * samplerate
    TST     R0, #TONEDATA_TYPE_SPL
    BNE     C_setup_synth
    /*
     * Mixing goes with volume ranges 0-127
     * They come in 0-255 --> divide by 2
     */
    MOVS    R11, R11, LSR#1
    ADC     R11, R11, #0x8000
    BIC     R11, R11, #0x8000
    MOV     R1, R7                          @ R1 = inter sample position
    /*
     * There is 2 different mixing codepaths for uncompressed data
     *  path 1: fast mixing, but doesn't supports loop or stop
     *  path 2: not so fast but supports sample loops / stop
     * This checks if there is enough samples aviable for path 1.
     * important: R0 is expected to be #0
     */
    SUB     R10, SP, #0x8
    TST     R0, #TONEDATA_TYPE_FIX
    MOVNE   R4, #0x800000
    MOVS    R0, R0, LSL#(MODE_FLGSH_SIGN_REVERSE)
    UMLAL   R1, R0, R4, R8
    MOV     R1, R1, LSR#23
    ORR     R0, R1, R0, LSL#9
    BCS     C_data_load_comp
    BMI     C_data_load_uncomp_rev
    B       C_data_load_uncomp_for

/* registers:
 * R9: src address (relative to start address)
 * R0: dst address (on stack)
 * R12: delta_lookup_table */
F_decode_compressed:
    STMFD   SP!, {R3, LR}
    MOV     LR, #BDPCM_BLK_SIZE
    LDRB    R2, [R9], #1
    LDRB    R3, [R9], #1
    B       C_bdpcm_decoder_loop_entry

C_bdpcm_decoder_loop:
    LDRB    R3, [R9], #1
    LDRB    R2, [R12, R3, LSR#4]
    ADD     R2, R1, R2
    AND     R3, R3, #0xF
C_bdpcm_decoder_loop_entry:
    LDRB    R1, [R12, R3]
    ADD     R1, R1, R2
bdpcm_instructions:
    NOP
    NOP
    SUBS    LR, #2
    BGT     C_bdpcm_decoder_loop
    LDMFD   SP!, {R3, PC}

bdpcm_instruction_resource_for:
    STRB    R2, [R0], #1
    STRB    R1, [R0], #1
bdpcm_instruction_resource_rev:
    STRB    R2, [R0, #-1]!
    STRB    R1, [R0, #-1]!

delta_lookup_table:
    .byte    0, 1, 4, 9, 16, 25, 36, 49, -64, -49, -36, -25, -16, -9, -4, -1
stack_boundary_literal:
    .word    0x03007900

C_data_load_comp:
    ADRPL   R9, bdpcm_instruction_resource_for
    ADRMI   R9, bdpcm_instruction_resource_rev
    LDMIA   R9, {R12, LR}
    ADR     R9, bdpcm_instructions
    STMIA   R9, {R12, LR}
    ADR     R12, delta_lookup_table
    BMI     C_data_load_comp_rev
C_data_load_comp_for:
    /* TODO having loop support for forward samples would be nice */
    /* LR = end_of_last_block */
    ADD     LR, R3, R0
    ADD     LR, #(1+(BDPCM_BLK_SIZE-1))             @ -1 for alignment, +1 because we need an extra sample for interpolation
    BIC     LR, #BDPCM_BLK_SIZE_MASK
    /* R9 = start_of_first_block >> 6 */
    MOV     R9, R3, LSR#BDPCM_BLK_SIZE_SHIFT
    /* R8 = num_samples */
    SUB     R8, LR, R9, LSL#BDPCM_BLK_SIZE_SHIFT
    /* check if stack would overflow */
    LDR     R1, stack_boundary_literal
    ADD     R1, R8
    CMP     R1, SP
    BHS     C_end_mixing
    /* --- */
    ADD     R1, R3, R0
    SUBS    R0, R2, R0
    STMFD   SP!, {R0, R1}
    SUB     SP, R8
    BGT     C_data_load_comp_for_calc_pos
    /* locate end of sample data block */
    ADD     R1, R3, R2
    /* ugly workaround for unaligned samples */
    ADD     R1, R1, #BDPCM_BLK_SIZE_MASK
    BIC     R1, R1, #BDPCM_BLK_SIZE_MASK
    SUB     R1, LR, R1
    SUB     R8, R1
    ADD     R0, SP, R8
    BL      F_clear_mem
C_data_load_comp_for_calc_pos:
    AND     R3, R3, #BDPCM_BLK_SIZE_MASK
    MOV     R0, SP
C_data_load_comp_decode:
    LDR     R2, [R10, #8]           @ load chn_ptr from previous STMFD
    @ zero flag should be only set when leaving from F_clear_mem (R1 = 0)
    STREQB  R1, [R2, #o_SoundChannel_statusFlags]
    LDR     R2, [R2, #o_SoundChannel_wav]
    ADD     R2, #o_WaveData_data
    MOV     R1, #BDPCM_BLK_STRIDE
    MLA     R9, R1, R9, R2
C_data_load_comp_loop:
    BL      F_decode_compressed
    SUBS    R8, #BDPCM_BLK_SIZE
    BGT     C_data_load_comp_loop
    B       C_select_highspeed_codepath_vla_r3

C_data_load_comp_rev:
    /* LR = end_of_last_block */
    ADD     LR, R3, #(BDPCM_BLK_SIZE-1)
    BIC     LR, #BDPCM_BLK_SIZE_MASK
    /* R9 = start_of_first_block >> 6 */
    SUB     R9, R3, R0
    SUB     R9, #1  @ one extra sample for LERP
    MOV     R9, R9, LSR#BDPCM_BLK_SIZE_SHIFT
    /* R8 = num_samples */
    SUB     R8, LR, R9, LSL#BDPCM_BLK_SIZE_SHIFT
    /* check if stack would overflow */
    LDR     LR, stack_boundary_literal
    ADD     LR, R8
    CMP     LR, SP
    BHS     C_end_mixing
    /* --- */
    SUB     LR, R3, R0
    SUBS    R0, R2, R0
    STMFD   SP!, {R0, LR}
    MOV     R0, SP
    SUB     SP, R8
    BGT     C_data_load_comp_rev_calc_pos
    SUB     R1, R3, R2
    SUB     R1, R1, R9, LSL#BDPCM_BLK_SIZE_SHIFT
    SUB     R8, R1
    ADD     R0, SP, R8
    BL      F_clear_mem
C_data_load_comp_rev_calc_pos:
    RSB     R3, R3, #0
    AND     R3, R3, #BDPCM_BLK_SIZE_MASK
    B       C_data_load_comp_decode

C_data_load_uncomp_rev:
    /* LR = end_of_last_block */
    ADD     LR, R3, #0x3
    BIC     LR, #0x3
    /* R9 = start_of_first_block */
    SUB     R9, R3, R0
    SUB     R9, #1
    BIC     R9, #0x3
    /* R8 = num_samples */
    SUB     R8, LR, R9
    /* check if stack would overflow */
    LDR     R1, stack_boundary_literal
    ADD     R1, R8
    CMP     R1, SP
    BHS     C_end_mixing
    /* --- */
    SUB     R1, R3, R0
    SUBS    R0, R2, R0
    STMFD   SP!, {R0, R1}
    MOV     R0, SP
    SUB     SP, R8
    BGT     C_data_load_uncomp_rev_loop
    SUB     R1, R3, R2
    SUB     R1, R9
    SUB     R8, R1
    ADD     R0, SP, R8
    BL      F_clear_mem
    LDR     R2, [R10, #8]           @ load chn_ptr from previous STMFD
    @ R1 should be zero here
    STRB    R1, [R2, #o_SoundChannel_statusFlags]
C_data_load_uncomp_rev_loop:
    LDMIA   R9!, {R1}
    EOR     R2, R1, R1, ROR#16
    MOV     R2, R2, LSR#8
    BIC     R2, R2, #0xFF00
    EOR     R1, R2, R1, ROR#8
    STMDB   R0!, {R1}
    SUBS    R8, #4
    BGT     C_data_load_uncomp_rev_loop
    RSB     R3, R3, #0
    B       C_select_highspeed_codepath_vla_r3_and3

C_data_load_uncomp_for:
    CMP     R2, R0                          @ actual comparison
    BLE     C_unbuffered_mixing       @ if not enough samples are available for path 1 --> branch
    /*
     * This is the mixer path 1.
     * The interesting thing here is that the code will
     * buffer enough samples on stack if enough space
     * on stack is available (or goes over the limit of 0x400 bytes)
     */
    SUB     R2, R2, R0
    LDR     R9, stack_boundary_literal
    ADD     R9, R0
    CMP     R9, SP
    ADD     R9, R3, R0
    /*
     * R2 = remaining samples after processing
     * R9 = final sample position
     * SP = original stack location
     * These values will get reloaded after channel processing
     * due to the lack of registers.
     */
    STMFD   SP!, {R2, R9}
    CMPLO   R0, #0x400                      @ > 0x400 bytes --> read directly from ROM rather than buffered
    BHS     C_select_highspeed_codepath

    BIC     R1, R3, #3
    ADD     R0, R0, #7
.if ENABLE_DMA==1
    /*
     * The code below inits the DMA to read word aligned
     * samples from ROM to stack
     */
    MOV     R9, #0x04000000
    ADD     R9, #0x000000D4
    MOV     R0, R0, LSR#2
    SUB     SP, SP, R0, LSL#2
    ORR     LR, R0, #0x84000000
    STMIA   R9, {R1, SP, LR}                @ actually starts the DMA
.else
    /*
     * This alternative path doesn't use DMA but copies with CPU instead
     */
    BIC     R0, R0, #0x3
    SUB     SP, SP, R0
    MOV     LR, SP
    STMFD   SP!, {R3-R10}
    ANDS    R10, R0, #0xE0
    RSB     R10, R10, #0xF0
    ADD     PC, PC, R10, LSR#2
C_copy_loop:
    LDMIA   R1!, {R3-R10}
    STMIA   LR!, {R3-R10}
    LDMIA   R1!, {R3-R10}
    STMIA   LR!, {R3-R10}
    LDMIA   R1!, {R3-R10}
    STMIA   LR!, {R3-R10}
    LDMIA   R1!, {R3-R10}
    STMIA   LR!, {R3-R10}
    LDMIA   R1!, {R3-R10}
    STMIA   LR!, {R3-R10}
    LDMIA   R1!, {R3-R10}
    STMIA   LR!, {R3-R10}
    LDMIA   R1!, {R3-R10}
    STMIA   LR!, {R3-R10}
    LDMIA   R1!, {R3-R10}
    STMIA   LR!, {R3-R10}
    SUBS    R0, #0x100
    BPL     C_copy_loop
    ANDS    R0, R0, #0x1C
    BEQ     C_copy_end
C_copy_loop_rest:
    LDMIA   R1!, {R3}
    STMIA   LR!, {R3}
    SUBS    R0, #0x4
    BGT     C_copy_loop_rest
C_copy_end:
    LDMFD   SP!, {R3-R10}
.endif
C_select_highspeed_codepath_vla_r3_and3:
    AND     R3, R3, #3
C_select_highspeed_codepath_vla_r3:
    ADD     R3, R3, SP
C_select_highspeed_codepath:
    STMFD   SP!, {R10}                      @ save original SP for VLA
    /*
     * This code decides which piece of code to load
     * depending on playback-rate / default-rate ratio.
     * Modes > 1.0 run with different volume levels.
     * R4 = inter sample step
     */
    ADR     R0, high_speed_code_resource    @ loads the base pointer of the code
    SUBS    R4, R4, #0x800000
    MOVPL   R11, R11, LSL#1                 @  if >= 1.0*   0-127 --> 0-254 volume level
    ADDPL   R0, R0, #(ARM_OP_LEN*6)         @               6 instructions further
    SUBPLS  R4, R4, #0x800000               @  if >= 2.0*
    ADDPL   R0, R0, #(ARM_OP_LEN*6)
    ADDPL   R4, R4, #0x800000
    LDR     R2, previous_fast_code
    CMP     R0, R2                          @ code doesn't need to be reloaded if it's already in place
    BEQ     C_skip_fast_mixing_creation
    /* This loads the needed code to RAM */
    STR     R0, previous_fast_code
    LDMIA   R0, {R0-R2, R8-R10}             @ load 6 opcodes
    ADR     LR, fast_mixing_instructions

C_fast_mixing_creation_loop:
    /* paste code to destination, see below for patterns */
    STMIA   LR, {R0, R1}
    ADD     LR, LR, #(ARM_OP_LEN*38)
    STMIA   LR, {R0, R1}
    SUB     LR, LR, #(ARM_OP_LEN*35)
    STMIA   LR, {R2, R8-R10}
    ADD     LR, LR, #(ARM_OP_LEN*38)
    STMIA   LR, {R2, R8-R10}
    SUB     LR, LR, #(ARM_OP_LEN*32)
    ADDS    R5, R5, #0x40000000         @ do that for 4 blocks
    BCC     C_fast_mixing_creation_loop

C_skip_fast_mixing_creation:
    LDR     R8, [SP]                        @ restore R8 with the frame length
    LDR     R8, [R8, #(ARG_FRAME_LENGTH + 0x8 + 0xC)]
    MOV     R2, #0xFF000000                 @ load the fine position overflow bitmask
    LDRSB   R12, [R3]
    SUB     R12, R12, R6
C_fast_mixing_loop:
    /* This is the actual processing and interpolation code loop; NOPs will be replaced by the code above */
    LDMIA   R5, {R0, R1, R10, LR}       @ load 4 stereo samples to Registers
    MUL     R9, R7, R12
fast_mixing_instructions:
    NOP                                 @ Block #1
    NOP
    MLANE   R0, R11, R9, R0
    NOP
    NOP
    NOP
    NOP
    BIC     R7, R7, R2, ASR#1
    MULNE   R9, R7, R12
    NOP                                 @ Block #2
    NOP
    MLANE   R1, R11, R9, R1
    NOP
    NOP
    NOP
    NOP
    BIC     R7, R7, R2, ASR#1
    MULNE   R9, R7, R12
    NOP                                 @ Block #3
    NOP
    MLANE   R10, R11, R9, R10
    NOP
    NOP
    NOP
    NOP
    BIC     R7, R7, R2, ASR#1
    MULNE   R9, R7, R12
    NOP                                 @ Block #4
    NOP
    MLANE   LR, R11, R9, LR
    NOP
    NOP
    NOP
    NOP
    BIC     R7, R7, R2, ASR#1
    STMIA   R5!, {R0, R1, R10, LR}      @ write 4 stereo samples

    LDMIA   R5, {R0, R1, R10, LR}       @ load the next 4 stereo samples
    MULNE   R9, R7, R12
    NOP                                 @ Block #1
    NOP
    MLANE   R0, R11, R9, R0
    NOP
    NOP
    NOP
    NOP
    BIC     R7, R7, R2, ASR#1
    MULNE   R9, R7, R12
    NOP                                 @ Block #2
    NOP
    MLANE   R1, R11, R9, R1
    NOP
    NOP
    NOP
    NOP
    BIC     R7, R7, R2, ASR#1
    MULNE   R9, R7, R12
    NOP                                 @ Block #3
    NOP
    MLANE   R10, R11, R9, R10
    NOP
    NOP
    NOP
    NOP
    BIC     R7, R7, R2, ASR#1
    MULNE   R9, R7, R12
    NOP                                 @ Block #4
    NOP
    MLANE   LR, R11, R9, LR
    NOP
    NOP
    NOP
    NOP
    BIC     R7, R7, R2, ASR#1
    STMIA   R5!, {R0, R1, R10, LR}      @ write 4 stereo samples
    SUBS    R8, R8, #8
    BGT     C_fast_mixing_loop
    /* restore previously saved values */
    LDMFD   SP, {SP}                        @ reload original stack pointer from VLA
C_skip_fast_mixing:
    LDMFD   SP!, {R2, R3}
    B       C_end_mixing

/* Various variables for the cached mixer */

    .align    2
previous_fast_code:
    .word   0x0 /* mark as invalid initially */

/* Those instructions below are used by the high speed loop self modifying code */
high_speed_code_resource:
    /* Block for Mix Freq < 1.0 * Output Frequency */
    MOV     R9, R9, ASR#22
    ADDS    R9, R9, R6, LSL#1
    ADDS    R7, R7, R4
    ADDPL   R6, R12, R6
    LDRPLSB R12, [R3, #1]!
    SUBPLS  R12, R12, R6

    /* Block for Mix Freq > 1.0 AND < 2.0 * Output Frequency */
    ADDS    R9, R6, R9, ASR#23
    ADD     R6, R12, R6
    ADDS    R7, R7, R4
    LDRPLSB R6, [R3, #1]!
    LDRSB   R12, [R3, #1]!
    SUBS    R12, R12, R6

    /* Block for Mix Freq > 2.0 * Output Frequency */
    ADDS    R9, R6, R9, ASR#23
    ADD     R7, R7, R4
    ADD     R3, R3, R7, LSR#23
    LDRSB   R6, [R3]
    LDRSB   R12, [R3, #1]!
    SUBS    R12, R12, R6

/* incase a loop or end occurs during mixing, this code is used */
C_unbuffered_mixing:
    LDRSB   R12, [R3]
    SUB     R12, R12, R6
    ADD     R5, R5, R8, LSL#2               @ R5 = End of HQ buffer

/* This below is the unbuffered mixing loop. R6 = base sample, R12 diff to next */
C_unbuffered_mixing_loop:

    MUL     R9, R7, R12
    MOV     R9, R9, ASR#22
    ADDS    R9, R9, R6, LSL#1
    LDRNE   R0, [R5, -R8, LSL#2]
    MLANE   R0, R11, R9, R0
    STRNE   R0, [R5, -R8, LSL#2]
    ADD     R7, R7, R4
    MOVS    R9, R7, LSR#23
    BEQ     C_unbuffered_mixing_skip_load   @ skip the mixing load if it isn't required

    SUBS    R2, R2, R9
    BLE     C_unbuffered_mixing_loop_or_end
C_unbuffered_mixing_loop_continue:
    SUBS    R9, R9, #1
    ADDEQ   R6, R12, R6
    LDRNESB R6, [R3, R9]!
    LDRSB   R12, [R3, #1]!
    SUB     R12, R12, R6
    BIC     R7, R7, #0x3F800000

C_unbuffered_mixing_skip_load:
    SUBS    R8, R8, #1                      @ reduce the sample count for the buffer by #1
    BGT     C_unbuffered_mixing_loop

C_end_mixing:
    LDMFD   SP!, {R4, R9, R12}
    STR     R7, [R4, #o_SoundChannel_fw]
    STRB    R6, [R4, #CHN_SAMPLE_STOR]
    B       C_mixing_end_store

C_unbuffered_mixing_loop_or_end:
    /* This loads the loop information end loops incase it should */
    LDR     R0, [SP, #(ARG_LOOP_LENGTH+0xC)]
    CMP     R0, #0                          @ check if loop is enabled; if Loop is enabled R6 is != 0
    SUBNE   R3, R3, R0
    ADDNE   R2, R2, R0
    BNE     C_unbuffered_mixing_loop_continue
    LDMFD   SP!, {R4, R9, R12}
    B       C_mixing_end_and_stop_channel   @ R0 == 0 (if this branches)

C_fixed_mixing_loop_or_end:
    LDR     R2, [SP, #ARG_LOOP_LENGTH+0x8]
    MOVS    R0, R2                          @ copy it to R6 and check whether loop is disabled
    LDRNE   R3, [SP, #ARG_LOOP_START_POS+0x8]
    BNE     C_fixed_mixing_loop_continue

    LDMFD   SP!, {R4, R9}

C_mixing_end_and_stop_channel:
    STRB    R0, [R4]                        @ update channel flag with chn halt
    B       C_mixing_epilogue

/* These are used for the fixed freq mixer */
fixed_mixing_code_resource:
    MOVS    R6, R10, LSL#24
    MOVS    R6, R6, ASR#24
    MOVS    R6, R10, LSL#16
    MOVS    R6, R6, ASR#24
    MOVS    R6, R10, LSL#8
    MOVS    R6, R6, ASR#24
    MOVS    R6, R10, ASR#24
    LDMIA   R3!, {R10}                          @ load chunk of samples
    MOVS    R6, R10, LSL#24
    MOVS    R6, R6, ASR#24
    MOVS    R6, R10, LSL#16
    MOVS    R6, R6, ASR#24
    MOVS    R6, R10, LSL#8
    MOVS    R6, R6, ASR#24

C_setup_fixed_freq_mixing:
    STMFD   SP!, {R4, R9}

C_fixed_mixing_length_check:
    MOV     LR, R2                          @ sample countdown
    CMP     R2, R8
    MOVGT   LR, R8                          @ min(buffer_size, sample_countdown)
    SUB     LR, LR, #1
    MOVS    LR, LR, LSR#2
    BEQ     C_fixed_mixing_process_rest     @ <= 3 samples to process

    SUB     R8, R8, LR, LSL#2               @ subtract the amount of samples we need to process from the buffer length
    SUB     R2, R2, LR, LSL#2               @ subtract the amount of samples we need to process from the remaining samples
    ADR     R1, fixed_mixing_instructions
    ADR     R0, fixed_mixing_code_resource
    MOV     R9, R3, LSL#30
    ADD     R0, R0, R9, LSR#27              @ alignment * 8 + resource offset = new resource offset
    LDMIA   R0!, {R6, R7, R9, R10}          @ load and write instructions
    STMIA   R1, {R6, R7}
    ADD     R1, R1, #0xC
    STMIA   R1, {R9, R10}
    ADD     R1, R1, #0xC
    LDMIA   R0, {R6, R7, R9, R10}
    STMIA   R1, {R6, R7}
    ADD     R1, R1, #0xC
    STMIA   R1, {R9, R10}
    LDMIA   R3!, {R10}                      @ load 4 samples from ROM

C_fixed_mixing_loop:
    LDMIA    R5, {R0, R1, R7, R9}       @ load 4 samples from hq buffer

fixed_mixing_instructions:
    NOP
    NOP
    MLANE   R0, R11, R6, R0             @ add new sample if neccessary
    NOP
    NOP
    MLANE   R1, R11, R6, R1
    NOP
    NOP
    MLANE   R7, R11, R6, R7
    NOP
    NOP
    MLANE   R9, R11, R6, R9
    STMIA   R5!, {R0, R1, R7, R9}       @ write samples to the mixing buffer
    SUBS    LR, LR, #1
    BNE     C_fixed_mixing_loop

    SUB     R3, R3, #4                      @ we'll need to load this block again, so rewind a bit

C_fixed_mixing_process_rest:
    MOV     R1, #4                          @ repeat the loop #4 times to completley get rid of alignment errors

C_fixed_mixing_unaligned_loop:
    LDR     R0, [R5]
    LDRSB   R6, [R3], #1
    MLA     R0, R11, R6, R0
    STR     R0, [R5], #4
    SUBS    R2, R2, #1
    BEQ     C_fixed_mixing_loop_or_end
C_fixed_mixing_loop_continue:
    SUBS    R1, R1, #1
    BGT     C_fixed_mixing_unaligned_loop

    SUBS    R8, R8, #4
    BGT     C_fixed_mixing_length_check     @ repeat the mixing procedure until the buffer is filled

    LDMFD   SP!, {R4, R9}

C_mixing_end_store:
    STR     R2, [R4, #o_SoundChannel_count]
    STR     R3, [R4, #o_SoundChannel_currentPointer]

C_mixing_epilogue:
    ADR     R0, (C_end_channel_state_loop+1)
    BX      R0

    .thumb

C_end_channel_state_loop:
    LDR     R0, [SP, #ARG_REMAIN_CHN]
    SUB     R0, #1
    BLE     C_main_mixer_return

    ADD     R4, #0x40
    B       C_channel_state_loop

C_main_mixer_return:
    LDR     R3, [SP, #ARG_PCM_STRUCT]
    LDRB    R4, [R3, #VAR_EXT_NOISE_SHAPE_LEFT]
    LSL     R4, R4, #16
    LDRB    R5, [R3, #VAR_EXT_NOISE_SHAPE_RIGHT]
    LSL     R5, R5, #16
.if ENABLE_REVERB==1
    LDRB    R2, [R3, #VAR_REVERB]
    LSR     R2, R2, #2
    LDR     R1, [SP, #ARG_BUFFER_POS_INDEX_HINT]
    CMP     R1, #2
.else
    MOV     R2, #0
    MOV     R3, #0
.endif
    ADR     R0, C_downsampler
    BX      R0

    .arm
    .align  2

C_downsampler:
    LDR     R8, [SP, #ARG_FRAME_LENGTH]
    LDR     R9, [SP, #ARG_BUFFER_POS]
.if ENABLE_REVERB==1
    ORR     R2, R2, R2, LSL#16
    MOVNE   R3, R8
    ADDEQ   R3, R3, #VAR_PCM_BUFFER
    SUBEQ   R3, R3, R9
.endif
    LDR     R10, hq_buffer_literal
    MOV     R11, #0xFF00
    MOV     LR, #0xC0000000

C_downsampler_loop:
    LDMIA   R10, {R0, R1}
    ADD     R12, R4, R0         @ left sample #1
    ADDS    R4, R12, R12
    EORVS   R12, LR, R4, ASR#31
    AND     R4, R12, #0x007F0000
    AND     R6, R11, R12, LSR#15

    ADD     R12, R5, R0, LSL#16 @ right sample #1
    ADDS    R5, R12, R12
    EORVS   R12, LR, R5, ASR#31
    AND     R5, R12, #0x007F0000
    AND     R7, R11, R12, LSR#15

    ADD     R12, R4, R1         @ left sample #2
    ADDS    R4, R12, R12
    EORVS   R12, LR, R4, ASR#31
    AND     R4, R12, #0x007F0000
    AND     R12, R11, R12, LSR#15
    ORR     R6, R12, R6, LSR#8

    ADD     R12, R5, R1, LSL#16 @ right sample #2
    ADDS    R5, R12, R12
    EORVS   R12, LR, R5, ASR#31
    AND     R5, R12, #0x007F0000
    AND     R12, R11, R12, LSR#15
    ORR     R7, R12, R7, LSR#8

.if ENABLE_REVERB==1
    LDRSH   R12, [R9, R3]!

    MOV     R1, R12, ASR#8
    MOV     R12, R12, LSL#24
    MOV     R0, R12, ASR#24

    ADD     R9, R9, #PCM_DMA_BUF_SIZE    @ \ LDRSH  R12, [R9, #0x630]!
    LDRSH   R12, [R9]                   @ / is unfortunately not a valid instruction

    ADD     R1, R1, R12, ASR#8
    MOV     R12, R12, LSL#24
    ADD     R0, R0, R12, ASR#24

    LDRSH   R12, [R9, -R3]!

    ADD     R1, R1, R12, ASR#8
    MOV     R12, R12, LSL#24
    ADD     R0, R0, R12, ASR#24

    STRH    R6, [R9]                    @ \ STRH  R6, [R9], #-0x630
    SUB     R9, R9, #PCM_DMA_BUF_SIZE    @ / is unfortunately not a valid instruction
    LDRSH   R12, [R9]
    STRH    R7, [R9], #2

    ADD     R1, R1, R12, ASR#8
    MOV     R12, R12, LSL#24
    ADD     R0, R0, R12, ASR#24

    MUL     R1, R2, R1
    MUL     R0, R2, R0

    STMIA   R10!, {R0, R1}
.else /* if ENABLE_REVERB==0 */
    MOV     R0, #PCM_DMA_BUF_SIZE
    STRH    R6, [R9, R0]
    STRH    R7, [R9], #2

    STMIA   R10!, {R2, R3}
.endif
    SUBS    R8, #2
    BGT     C_downsampler_loop

    ADR     R0, (C_downsampler_return+1)
    BX      R0

    .pool

    .align  1
    .thumb

C_downsampler_return:
    LDR     R0, [SP, #ARG_PCM_STRUCT]
    LSR     R4, #16
    STRB    R4, [R0, #VAR_EXT_NOISE_SHAPE_LEFT]
    LSR     R5, #16
    STRB    R5, [R0, #VAR_EXT_NOISE_SHAPE_RIGHT]
    LDR     R3, =0x68736D53                     @ this is used to indicate the interrupt handler the rendering was finished properly
    STR     R3, [R0]
    ADD     SP, SP, #0x1C
    POP     {R0-R7}
    MOV     R8, R0
    MOV     R9, R1
    MOV     R10, R2
    MOV     R11, R3
    POP     {PC}

    .pool

    .arm
    .align  2

C_setup_synth:
    LDRB    R12, [R3, #SYNTH_TYPE]
    CMP     R12, #0
    BNE     C_check_synth_saw

    /* modulating pulse wave */
    LDRB    R6, [R3, #SYNTH_WIDTH_CHANGE_1]
    ADD     R2, R2, R6, LSL#24
    LDRB    R6, [R3, #SYNTH_WIDTH_CHANGE_2]
    ADDS    R6, R2, R6, LSL#24
    MVNMI   R6, R6
    MOV     R10, R6, LSR#8
    LDRB    R1, [R3, #SYNTH_MOD_AMOUNT]
    LDRB    R0, [R3, #SYNTH_BASE_WAVE_DUTY]
    MOV     R0, R0, LSL#24
    MLA     R6, R10, R1, R0                 @ calculate the final duty cycle with the offset, and intensity * rotating duty cycle amount
    STMFD   SP!, {R2, R3, R9, R12}

C_synth_pulse_loop:
    LDMIA   R5, {R0-R3, R9, R10, R12, LR} @ load 8 samples
    CMP     R7, R6                      @ Block #1
    ADDLO   R0, R0, R11, LSL#6
    SUBHS   R0, R0, R11, LSL#6
    ADDS    R7, R7, R4, LSL#3
    CMP     R7, R6                      @ Block #2
    ADDLO   R1, R1, R11, LSL#6
    SUBHS   R1, R1, R11, LSL#6
    ADDS    R7, R7, R4, LSL#3
    CMP     R7, R6                      @ Block #3
    ADDLO   R2, R2, R11, LSL#6
    SUBHS   R2, R2, R11, LSL#6
    ADDS    R7, R7, R4, LSL#3
    CMP     R7, R6                      @ Block #4
    ADDLO   R3, R3, R11, LSL#6
    SUBHS   R3, R3, R11, LSL#6
    ADDS    R7, R7, R4, LSL#3
    CMP     R7, R6                      @ Block #5
    ADDLO   R9, R9, R11, LSL#6
    SUBHS   R9, R9, R11, LSL#6
    ADDS    R7, R7, R4, LSL#3
    CMP     R7, R6                      @ Block #6
    ADDLO   R10, R10, R11, LSL#6
    SUBHS   R10, R10, R11, LSL#6
    ADDS    R7, R7, R4, LSL#3
    CMP     R7, R6                      @ Block #7
    ADDLO   R12, R12, R11, LSL#6
    SUBHS   R12, R12, R11, LSL#6
    ADDS    R7, R7, R4, LSL#3
    CMP     R7, R6                      @ Block #8
    ADDLO   LR, LR, R11, LSL#6
    SUBHS   LR, LR, R11, LSL#6
    ADDS    R7, R7, R4, LSL#3

    STMIA   R5!, {R0-R3, R9, R10, R12, LR} @ write 8 samples
    SUBS    R8, R8, #8
    BGT     C_synth_pulse_loop

    LDMFD   SP!, {R2, R3, R9, R12}
    B       C_end_mixing

C_check_synth_saw:
    /*
     * This is actually not a true saw wave
     * but looks pretty similar
     * (has a jump in the middle of the wave)
     */
    SUBS    R12, R12, #1
    BNE     C_synth_triangle

    MOV     R6, #0x300
    MOV     R11, R11, LSR#1
    BIC     R11, R11, #0xFF00
    MOV     R12, #0x70

C_synth_saw_loop:

    LDMIA   R5, {R0, R1, R10, LR}       @ load 4 samples from memory
    ADDS    R7, R7, R4, LSL#3           @ Block #1 (some oscillator type code)
    RSB     R9, R12, R7, LSR#24
    MOV     R6, R7, LSL#1
    SUB     R9, R9, R6, LSR#27
    ADDS    R2, R9, R2, ASR#1
    MLANE   R0, R11, R2, R0

    ADDS    R7, R7, R4, LSL#3           @ Block #2
    RSB     R9, R12, R7, LSR#24
    MOV     R6, R7, LSL#1
    SUB     R9, R9, R6, LSR#27
    ADDS    R2, R9, R2, ASR#1
    MLANE   R1, R11, R2, R1

    ADDS    R7, R7, R4, LSL#3           @ Block #3
    RSB     R9, R12, R7, LSR#24
    MOV     R6, R7, LSL#1
    SUB     R9, R9, R6, LSR#27
    ADDS    R2, R9, R2, ASR#1
    MLANE   R10, R11, R2, R10

    ADDS    R7, R7, R4, LSL#3           @ Block #4
    RSB     R9, R12, R7, LSR#24
    MOV     R6, R7, LSL#1
    SUB     R9, R9, R6, LSR#27
    ADDS    R2, R9, R2, ASR#1
    MLANE   LR, R11, R2, LR

    STMIA   R5!, {R0, R1, R10, LR}
    SUBS    R8, R8, #4
    BGT     C_synth_saw_loop

    B       C_end_mixing

C_synth_triangle:
    MOV     R6, #0x80
    MOV     R12, #0x180

C_synth_triangle_loop:
    LDMIA   R5, {R0, R1, R10, LR}       @ load samples from work buffer
    ADDS    R7, R7, R4, LSL#3           @ Block #1
    RSBPL   R9, R6, R7, ASR#23
    SUBMI   R9, R12, R7, LSR#23
    MLA     R0, R11, R9, R0

    ADDS    R7, R7, R4, LSL#3           @ Block #2
    RSBPL   R9, R6, R7, ASR#23
    SUBMI   R9, R12, R7, LSR#23
    MLA     R1, R11, R9, R1

    ADDS    R7, R7, R4, LSL#3           @ Block #3
    RSBPL   R9, R6, R7, ASR#23
    SUBMI   R9, R12, R7, LSR#23
    MLA     R10, R11, R9, R10

    ADDS    R7, R7, R4, LSL#3           @ Block #4
    RSBPL   R9, R6, R7, ASR#23
    SUBMI   R9, R12, R7, LSR#23
    MLA     LR, R11, R9, LR

    STMIA   R5!, {R0, R1, R10, LR}
    SUBS    R8, R8, #4                  @ subtract #4 from the remainging samples
    BGT     C_synth_triangle_loop

    B       C_end_mixing

/* R0: base addr
 * R1: len in bytes */
F_clear_mem:
    STMFD   SP!, {R0, R2-R5, LR}
    MOV     R2, #0
    MOV     R3, #0
    MOV     R4, #0
    MOV     R5, #0
    AND     LR, R1, #0x30
    RSB     LR, LR, #0x30
    ADD     PC, PC, LR, LSR#2
C_clear_loop:
    STMIA   R0!, {R2-R5}
    STMIA   R0!, {R2-R5}
    STMIA   R0!, {R2-R5}
    STMIA   R0!, {R2-R5}
    SUBS    R1, R1, #0x40
    BPL     C_clear_loop
    ANDS    R1, R1, #0xC
    LDMEQFD SP!, {R0, R2-R5, PC}
C_clear_loop_rest:
    STMIA   R0!, {R2}
    SUBS    R1, R1, #4
    BGT     C_clear_loop_rest
    LDMFD   SP!, {R0, R2-R5, PC}

SoundMainRAM_End:
	.syntax unified
	thumb_func_end SoundMainRAM

	thumb_func_start SoundMainBTM
SoundMainBTM:
	mov r12, r4
	movs r1, 0
	movs r2, 0
	movs r3, 0
	movs r4, 0
	stm r0!, {r1-r4}
	stm r0!, {r1-r4}
	stm r0!, {r1-r4}
	stm r0!, {r1-r4}
	mov r4, r12
	bx lr
	thumb_func_end SoundMainBTM

	thumb_func_start RealClearChain
RealClearChain:
	ldr r3, [r0, o_SoundChannel_track]
	cmp r3, 0
	beq _081DD5E2
	ldr r1, [r0, o_SoundChannel_nextChannelPointer]
	ldr r2, [r0, o_SoundChannel_prevChannelPointer]
	cmp r2, 0
	beq _081DD5D6
	str r1, [r2, o_SoundChannel_nextChannelPointer]
	b _081DD5D8
_081DD5D6:
	str r1, [r3, o_MusicPlayerTrack_chan]
_081DD5D8:
	cmp r1, 0
	beq _081DD5DE
	str r2, [r1, o_SoundChannel_prevChannelPointer]
_081DD5DE:
	movs r1, 0
	str r1, [r0, o_SoundChannel_track]
_081DD5E2:
	bx lr
	thumb_func_end RealClearChain

	thumb_func_start ply_fine
ply_fine:
	push {r4,r5,lr}
	adds r5, r1, 0
	ldr r4, [r5, o_MusicPlayerTrack_chan]
	cmp r4, 0
	beq ply_fine_done
ply_fine_loop:
	ldrb r1, [r4, o_SoundChannel_statusFlags]
	movs r0, SOUND_CHANNEL_SF_ON
	tst r0, r1
	beq ply_fine_ok
	movs r0, SOUND_CHANNEL_SF_STOP
	orrs r1, r0
	strb r1, [r4, o_SoundChannel_statusFlags]
ply_fine_ok:
	adds r0, r4, 0
	bl RealClearChain
	ldr r4, [r4, o_SoundChannel_nextChannelPointer]
	cmp r4, 0
	bne ply_fine_loop
ply_fine_done:
	movs r0, 0
	strb r0, [r5, o_MusicPlayerTrack_flags]
	pop {r4,r5}
	pop {r0}
	bx r0
	thumb_func_end ply_fine

	thumb_func_start MPlayJumpTableCopy
MPlayJumpTableCopy:
	mov r12, lr
	movs r1, 0x24
	ldr r2, lt_MPlayJumpTableTemplate
MPlayJumpTableCopy_Loop:
	ldr r3, [r2]
	bl chk_adr_r2
	stm r0!, {r3}
	adds r2, 0x4
	subs r1, 0x1
	bgt MPlayJumpTableCopy_Loop
	bx r12
	thumb_func_end MPlayJumpTableCopy

	.align 2, 0
	.thumb_func
ldrb_r3_r2:
	ldrb r3, [r2]

@ This attempts to protect against reading anything from the BIOS ROM
@ besides the jump table template.
@ It assumes that the jump table template is located at the end of the ROM.
	.thumb_func
chk_adr_r2:
	push {r0}
	lsrs r0, r2, 25
	bne chk_adr_r2_done @ if adr >= 0x2000000 (i.e. not in BIOS ROM), accept it
	ldr r0, lt_MPlayJumpTableTemplate
	cmp r2, r0
	blo chk_adr_r2_reject @ if adr < gMPlayJumpTableTemplate, reject it
	lsrs r0, r2, 14
	beq chk_adr_r2_done @ if adr < 0x40000 (i.e. in BIOS ROM), accept it
chk_adr_r2_reject:
	movs r3, 0
chk_adr_r2_done:
	pop {r0}
	bx lr

	.align 2, 0
lt_MPlayJumpTableTemplate: .word gMPlayJumpTableTemplate

	thumb_func_start ld_r3_tp_adr_i
ld_r3_tp_adr_i:
	ldr r2, [r1, o_MusicPlayerTrack_cmdPtr]
_081DD64A:
	adds r3, r2, 0x1
	str r3, [r1, o_MusicPlayerTrack_cmdPtr]
	ldrb r3, [r2]
	b chk_adr_r2
	thumb_func_end ld_r3_tp_adr_i

	thumb_func_start ply_goto
ply_goto:
	push {lr}
ply_goto_1:
	ldr r2, [r1, o_MusicPlayerTrack_cmdPtr]
	ldrb r0, [r2, 0x3]
	lsls r0, 8
	ldrb r3, [r2, 0x2]
	orrs r0, r3
	lsls r0, 8
	ldrb r3, [r2, 0x1]
	orrs r0, r3
	lsls r0, 8
	bl ldrb_r3_r2
	orrs r0, r3
	str r0, [r1, o_MusicPlayerTrack_cmdPtr]
	pop {r0}
	bx r0
	thumb_func_end ply_goto

	thumb_func_start ply_patt
ply_patt:
	ldrb r2, [r1, o_MusicPlayerTrack_patternLevel]
	cmp r2, 3
	bhs ply_patt_done
	lsls r2, 2
	adds r3, r1, r2
	ldr r2, [r1, o_MusicPlayerTrack_cmdPtr]
	adds r2, 0x4
	str r2, [r3, o_MusicPlayerTrack_patternStack]
	ldrb r2, [r1, o_MusicPlayerTrack_patternLevel]
	adds r2, 1
	strb r2, [r1, o_MusicPlayerTrack_patternLevel]
	b ply_goto
ply_patt_done:
	b ply_fine
	thumb_func_end ply_patt

	thumb_func_start ply_pend
ply_pend:
	ldrb r2, [r1, o_MusicPlayerTrack_patternLevel]
	cmp r2, 0
	beq ply_pend_done
	subs r2, 1
	strb r2, [r1, o_MusicPlayerTrack_patternLevel]
	lsls r2, 2
	adds r3, r1, r2
	ldr r2, [r3, o_MusicPlayerTrack_patternStack]
	str r2, [r1, o_MusicPlayerTrack_cmdPtr]
ply_pend_done:
	bx lr
	thumb_func_end ply_pend

	thumb_func_start ply_rept
ply_rept:
	push {lr}
	ldr r2, [r1, o_MusicPlayerTrack_cmdPtr]
	ldrb r3, [r2]
	cmp r3, 0
	bne ply_rept_1
	adds r2, 1
	str r2, [r1, o_MusicPlayerTrack_cmdPtr]
	b ply_goto_1
ply_rept_1:
	ldrb r3, [r1, o_MusicPlayerTrack_repN]
	adds r3, 1
	strb r3, [r1, o_MusicPlayerTrack_repN]
	mov r12, r3
	bl ld_r3_tp_adr_i
	cmp r12, r3
	bhs ply_rept_2
	b ply_goto_1
ply_rept_2:
	movs r3, 0
	strb r3, [r1, o_MusicPlayerTrack_repN]
	adds r2, 5
	str r2, [r1, o_MusicPlayerTrack_cmdPtr]
	pop {r0}
	bx r0
	thumb_func_end ply_rept

	thumb_func_start ply_prio
ply_prio:
	mov r12, lr
	bl ld_r3_tp_adr_i
	strb r3, [r1, o_MusicPlayerTrack_priority]
	bx r12
	thumb_func_end ply_prio

	thumb_func_start ply_tempo
ply_tempo:
	mov r12, lr
	bl ld_r3_tp_adr_i
	lsls r3, 1
	strh r3, [r0, o_MusicPlayerInfo_tempoD]
	ldrh r2, [r0, o_MusicPlayerInfo_tempoU]
	muls r3, r2
	lsrs r3, 8
	strh r3, [r0, o_MusicPlayerInfo_tempoI]
	bx r12
	thumb_func_end ply_tempo

	thumb_func_start ply_keysh
ply_keysh:
	mov r12, lr
	bl ld_r3_tp_adr_i
	strb r3, [r1, o_MusicPlayerTrack_keyShift]
	ldrb r3, [r1, o_MusicPlayerTrack_flags]
	movs r2, 0xC
	orrs r3, r2
	strb r3, [r1, o_MusicPlayerTrack_flags]
	bx r12
	thumb_func_end ply_keysh

	thumb_func_start ply_voice
ply_voice:
	mov r12, lr
	ldr r2, [r1, o_MusicPlayerTrack_cmdPtr]
	ldrb r3, [r2]
	adds r2, 1
	str r2, [r1, o_MusicPlayerTrack_cmdPtr]
	lsls r2, r3, 1
	adds r2, r3
	lsls r2, 2
	ldr r3, [r0, o_MusicPlayerInfo_tone]
	adds r2, r3
	ldr r3, [r2, o_ToneData_type]
	bl chk_adr_r2
	str r3, [r1, o_MusicPlayerTrack_ToneData_type]
	ldr r3, [r2, o_ToneData_wav]
	bl chk_adr_r2
	str r3, [r1, o_MusicPlayerTrack_ToneData_wav]
	ldr r3, [r2, o_ToneData_attack]
	bl chk_adr_r2
	str r3, [r1, o_MusicPlayerTrack_ToneData_attack]
	bx r12
	thumb_func_end ply_voice

	thumb_func_start ply_vol
ply_vol:
	mov r12, lr
	bl ld_r3_tp_adr_i
	strb r3, [r1, o_MusicPlayerTrack_vol]
	ldrb r3, [r1, o_MusicPlayerTrack_flags]
	movs r2, MPT_FLG_VOLCHG
	orrs r3, r2
	strb r3, [r1, o_MusicPlayerTrack_flags]
	bx r12
	thumb_func_end ply_vol

	thumb_func_start ply_pan
ply_pan:
	mov r12, lr
	bl ld_r3_tp_adr_i
	subs r3, C_V
	strb r3, [r1, o_MusicPlayerTrack_pan]
	ldrb r3, [r1, o_MusicPlayerTrack_flags]
	movs r2, MPT_FLG_VOLCHG
	orrs r3, r2
	strb r3, [r1, o_MusicPlayerTrack_flags]
	bx r12
	thumb_func_end ply_pan

	thumb_func_start ply_bend
ply_bend:
	mov r12, lr
	bl ld_r3_tp_adr_i
	subs r3, C_V
	strb r3, [r1, o_MusicPlayerTrack_bend]
	ldrb r3, [r1, o_MusicPlayerTrack_flags]
	movs r2, MPT_FLG_PITCHG
	orrs r3, r2
	strb r3, [r1, o_MusicPlayerTrack_flags]
	bx r12
	thumb_func_end ply_bend

	thumb_func_start ply_bendr
ply_bendr:
	mov r12, lr
	bl ld_r3_tp_adr_i
	strb r3, [r1, o_MusicPlayerTrack_bendRange]
	ldrb r3, [r1, o_MusicPlayerTrack_flags]
	movs r2, MPT_FLG_PITCHG
	orrs r3, r2
	strb r3, [r1, o_MusicPlayerTrack_flags]
	bx r12
	thumb_func_end ply_bendr

	thumb_func_start ply_lfodl
ply_lfodl:
	mov r12, lr
	bl ld_r3_tp_adr_i
	strb r3, [r1, o_MusicPlayerTrack_lfoDelay]
	bx r12
	thumb_func_end ply_lfodl

	thumb_func_start ply_modt
ply_modt:
	mov r12, lr
	bl ld_r3_tp_adr_i
	ldrb r0, [r1, o_MusicPlayerTrack_modT]
	cmp r0, r3
	beq _081DD7AA
	strb r3, [r1, o_MusicPlayerTrack_modT]
	ldrb r3, [r1, o_MusicPlayerTrack_flags]
	movs r2, MPT_FLG_VOLCHG | MPT_FLG_PITCHG
	orrs r3, r2
	strb r3, [r1, o_MusicPlayerTrack_flags]
_081DD7AA:
	bx r12
	thumb_func_end ply_modt

	thumb_func_start ply_tune
ply_tune:
	mov r12, lr
	bl ld_r3_tp_adr_i
	subs r3, C_V
	strb r3, [r1, o_MusicPlayerTrack_tune]
	ldrb r3, [r1, o_MusicPlayerTrack_flags]
	movs r2, MPT_FLG_PITCHG
	orrs r3, r2
	strb r3, [r1, o_MusicPlayerTrack_flags]
	bx r12
	thumb_func_end ply_tune

	thumb_func_start ply_port
ply_port:
	mov r12, lr
	ldr r2, [r1, o_MusicPlayerTrack_cmdPtr]
	ldrb r3, [r2]
	adds r2, 1
	ldr r0, =REG_SOUND1CNT_L @ sound register base address
	adds r0, r3
	bl _081DD64A
	strb r3, [r0]
	bx r12
	.pool
	thumb_func_end ply_port

	thumb_func_start m4aSoundVSync
m4aSoundVSync:
	ldr r0, lt2_SOUND_INFO_PTR
	ldr r0, [r0]

	@ Exit the function if ident is not ID_NUMBER or ID_NUMBER+1.
	ldr r2, lt2_ID_NUMBER
	ldr r3, [r0, o_SoundInfo_ident]
	subs r3, r2
	cmp r3, 1
	bhi m4aSoundVSync_Done

	@ Decrement the PCM DMA counter. If it reaches 0, we need to do a DMA.
	ldrb r1, [r0, o_SoundInfo_pcmDmaCounter]
	subs r1, 1
	strb r1, [r0, o_SoundInfo_pcmDmaCounter]
	bgt m4aSoundVSync_Done

	@ Reload the PCM DMA counter.
	ldrb r1, [r0, o_SoundInfo_pcmDmaPeriod]
	strb r1, [r0, o_SoundInfo_pcmDmaCounter]

	ldr r2, =REG_DMA1

	ldr r1, [r2, 0x8] @ DMA1CNT
	lsls r1, 7
	bcc m4aSoundVSync_SkipDMA1 @ branch if repeat bit isn't set

	ldr r1, =((DMA_ENABLE | DMA_START_NOW | DMA_32BIT | DMA_SRC_INC | DMA_DEST_FIXED) << 16) | 4
	str r1, [r2, 0x8] @ DMA1CNT

m4aSoundVSync_SkipDMA1:
	ldr r1, [r2, 0xC + 0x8] @ DMA2CNT
	lsls r1, 7
	bcc m4aSoundVSync_SkipDMA2 @ branch if repeat bit isn't set

	ldr r1, =((DMA_ENABLE | DMA_START_NOW | DMA_32BIT | DMA_SRC_INC | DMA_DEST_FIXED) << 16) | 4
	str r1, [r2, 0xC + 0x8] @ DMA2CNT

m4aSoundVSync_SkipDMA2:

	@ turn off DMA1/DMA2
	movs r1, DMA_32BIT >> 8
	lsls r1, 8
	strh r1, [r2, 0xA]       @ DMA1CNT_H
	strh r1, [r2, 0xC + 0xA] @ DMA2CNT_H

	@ turn on DMA1/DMA2 direct-sound FIFO mode
	movs r1, (DMA_ENABLE | DMA_START_SPECIAL | DMA_32BIT | DMA_REPEAT) >> 8
	lsls r1, 8 @ LSB is 0, so DMA_SRC_INC is used (destination is always fixed in FIFO mode)
	strh r1, [r2, 0xA]       @ DMA1CNT_H
	strh r1, [r2, 0xC + 0xA] @ DMA2CNT_H

m4aSoundVSync_Done:
	bx lr

	.pool
	thumb_func_end m4aSoundVSync

	thumb_func_start MPlayMain
MPlayMain:
	ldr r2, lt2_ID_NUMBER
	ldr r3, [r0, o_MusicPlayerInfo_ident]
	cmp r2, r3
	beq _081DD82E
	bx lr
_081DD82E:
	adds r3, 0x1
	str r3, [r0, o_MusicPlayerInfo_ident]
	push {r0,lr}
	ldr r3, [r0, o_MusicPlayerInfo_func]
	cmp r3, 0
	beq _081DD840
	ldr r0, [r0, o_MusicPlayerInfo_intp]
	bl call_r3
_081DD840:
	pop {r0}
	push {r4-r7}
	mov r4, r8
	mov r5, r9
	mov r6, r10
	mov r7, r11
	push {r4-r7}
	adds r7, r0, 0
	ldr r0, [r7, o_MusicPlayerInfo_status]
	cmp r0, 0
	bge _081DD858
	b _081DDA6C
_081DD858:
	ldr r0, lt2_SOUND_INFO_PTR
	ldr r0, [r0]
	mov r8, r0
	adds r0, r7, 0
	bl FadeOutBody
	ldr r0, [r7, o_MusicPlayerInfo_status]
	cmp r0, 0
	bge _081DD86C
	b _081DDA6C
_081DD86C:
	ldrh r0, [r7, o_MusicPlayerInfo_tempoC]
	ldrh r1, [r7, o_MusicPlayerInfo_tempoI]
	adds r0, r1
	b _081DD9BC
_081DD874:
	ldrb r6, [r7, o_MusicPlayerInfo_trackCount]
	ldr r5, [r7, o_MusicPlayerInfo_tracks]
	movs r3, 0x1
	movs r4, 0
_081DD87C:
	ldrb r0, [r5, o_MusicPlayerTrack_flags]
	movs r1, MPT_FLG_EXIST
	tst r1, r0
	bne _081DD886
	b _081DD998
_081DD886:
	mov r10, r3
	orrs r4, r3
	mov r11, r4
	ldr r4, [r5, o_MusicPlayerTrack_chan]
	cmp r4, 0
	beq _081DD8BA
_081DD892:
	ldrb r1, [r4, o_SoundChannel_statusFlags]
	movs r0, SOUND_CHANNEL_SF_ON
	tst r0, r1
	beq _081DD8AE
	ldrb r0, [r4, o_SoundChannel_gateTime]
	cmp r0, 0
	beq _081DD8B4
	subs r0, 0x1
	strb r0, [r4, o_SoundChannel_gateTime]
	bne _081DD8B4
	movs r0, SOUND_CHANNEL_SF_STOP
	orrs r1, r0
	strb r1, [r4, o_SoundChannel_statusFlags]
	b _081DD8B4
_081DD8AE:
	adds r0, r4, 0
	bl ClearChain
_081DD8B4:
	ldr r4, [r4, o_SoundChannel_nextChannelPointer]
	cmp r4, 0
	bne _081DD892
_081DD8BA:
	ldrb r3, [r5, o_MusicPlayerTrack_flags]
	movs r0, MPT_FLG_START
	tst r0, r3
	beq _081DD938
	adds r0, r5, 0
	bl Clear64byte
	movs r0, MPT_FLG_EXIST
	strb r0, [r5, o_MusicPlayerTrack_flags]
	movs r0, 0x2
	strb r0, [r5, o_MusicPlayerTrack_bendRange]
	movs r0, 0x40
	strb r0, [r5, o_MusicPlayerTrack_volX]
	movs r0, 0x16
	strb r0, [r5, o_MusicPlayerTrack_lfoSpeed]
	movs r0, 0x1
	adds r1, r5, 0x6
	strb r0, [r1, o_MusicPlayerTrack_ToneData_type - 0x6]
	b _081DD938
_081DD8E0:
	ldr r2, [r5, o_MusicPlayerTrack_cmdPtr]
	ldrb r1, [r2]
	cmp r1, 0x80
	bhs _081DD8EC
	ldrb r1, [r5, o_MusicPlayerTrack_runningStatus]
	b _081DD8F6
_081DD8EC:
	adds r2, 0x1
	str r2, [r5, o_MusicPlayerTrack_cmdPtr]
	cmp r1, 0xBD
	bcc _081DD8F6
	strb r1, [r5, o_MusicPlayerTrack_runningStatus]
_081DD8F6:
	cmp r1, 0xCF
	bcc _081DD90C
	mov r0, r8
	ldr r3, [r0, o_SoundInfo_plynote]
	adds r0, r1, 0
	subs r0, 0xCF
	adds r1, r7, 0
	adds r2, r5, 0
	bl call_r3
	b _081DD938
_081DD90C:
	cmp r1, 0xB0
	bls _081DD92E
	adds r0, r1, 0
	subs r0, 0xB1
	strb r0, [r7, o_MusicPlayerInfo_cmd]
	mov r3, r8
	ldr r3, [r3, o_SoundInfo_MPlayJumpTable]
	lsls r0, 2
	ldr r3, [r3, r0]
	adds r0, r7, 0
	adds r1, r5, 0
	bl call_r3
	ldrb r0, [r5, o_MusicPlayerTrack_flags]
	cmp r0, 0
	beq _081DD994
	b _081DD938
_081DD92E:
	ldr r0, lt_gClockTable
	subs r1, 0x80
	adds r1, r0
	ldrb r0, [r1]
	strb r0, [r5, o_MusicPlayerTrack_wait]
_081DD938:
	ldrb r0, [r5, o_MusicPlayerTrack_wait]
	cmp r0, 0
	beq _081DD8E0
	subs r0, 0x1
	strb r0, [r5, o_MusicPlayerTrack_wait]
	ldrb r1, [r5, o_MusicPlayerTrack_lfoSpeed]
	cmp r1, 0
	beq _081DD994
	ldrb r0, [r5, o_MusicPlayerTrack_mod]
	cmp r0, 0
	beq _081DD994
	ldrb r0, [r5, o_MusicPlayerTrack_lfoDelayC]
	cmp r0, 0
	beq _081DD95A
	subs r0, 0x1
	strb r0, [r5, o_MusicPlayerTrack_lfoDelayC]
	b _081DD994
_081DD95A:
	ldrb r0, [r5, o_MusicPlayerTrack_lfoSpeedC]
	adds r0, r1
	strb r0, [r5, o_MusicPlayerTrack_lfoSpeedC]
	adds r1, r0, 0
	subs r0, 0x40
	lsls r0, 24
	bpl _081DD96E
	lsls r2, r1, 24
	asrs r2, 24
	b _081DD972
_081DD96E:
	movs r0, 0x80
	subs r2, r0, r1
_081DD972:
	ldrb r0, [r5, o_MusicPlayerTrack_mod]
	muls r0, r2
	asrs r2, r0, 6
	ldrb r0, [r5, o_MusicPlayerTrack_modM]
	eors r0, r2
	lsls r0, 24
	beq _081DD994
	strb r2, [r5, o_MusicPlayerTrack_modM]
	ldrb r0, [r5]
	ldrb r1, [r5, o_MusicPlayerTrack_modT]
	cmp r1, 0
	bne _081DD98E
	movs r1, MPT_FLG_PITCHG
	b _081DD990
_081DD98E:
	movs r1, MPT_FLG_VOLCHG
_081DD990:
	orrs r0, r1
	strb r0, [r5, o_MusicPlayerTrack_flags]
_081DD994:
	mov r3, r10
	mov r4, r11
_081DD998:
	subs r6, 0x1
	ble _081DD9A4
	movs r0, MusicPlayerTrack_size
	adds r5, r0
	lsls r3, 1
	b _081DD87C
_081DD9A4:
	ldr r0, [r7, o_MusicPlayerInfo_clock]
	adds r0, 0x1
	str r0, [r7, o_MusicPlayerInfo_clock]
	cmp r4, 0
	bne _081DD9B6
	movs r0, 0x80
	lsls r0, 24
	str r0, [r7, o_MusicPlayerInfo_status]
	b _081DDA6C
_081DD9B6:
	str r4, [r7, o_MusicPlayerInfo_status]
	ldrh r0, [r7, o_MusicPlayerInfo_tempoC]
	subs r0, 150
_081DD9BC:
	strh r0, [r7, o_MusicPlayerInfo_tempoC]
	cmp r0, 150
	bcc _081DD9C4
	b _081DD874
_081DD9C4:
	ldrb r2, [r7, o_MusicPlayerInfo_trackCount]
	ldr r5, [r7, o_MusicPlayerInfo_tracks]
_081DD9C8:
	ldrb r0, [r5, o_MusicPlayerTrack_flags]
	movs r1, 0x80
	tst r1, r0
	beq _081DDA62
	movs r1, MPT_FLG_VOLCHG | MPT_FLG_PITCHG
	tst r1, r0
	beq _081DDA62
	mov r9, r2
	adds r0, r7, 0
	adds r1, r5, 0
	bl TrkVolPitSet
	ldr r4, [r5, o_MusicPlayerTrack_chan]
	cmp r4, 0
	beq _081DDA58
_081DD9E6:
	ldrb r1, [r4, o_SoundChannel_statusFlags]
	movs r0, SOUND_CHANNEL_SF_ON
	tst r0, r1
	bne _081DD9F6
	adds r0, r4, 0
	bl ClearChain
	b _081DDA52
_081DD9F6:
	ldrb r0, [r4, o_SoundChannel_type]
	movs r6, TONEDATA_TYPE_CGB
	ands r6, r0
	ldrb r3, [r5, o_MusicPlayerTrack_flags]
	movs r0, MPT_FLG_VOLCHG
	tst r0, r3
	beq _081DDA14
	bl ChnVolSetAsm
	cmp r6, 0
	beq _081DDA14
	ldrb r0, [r4, o_CgbChannel_modify]
	movs r1, 0x1
	orrs r0, r1
	strb r0, [r4, o_CgbChannel_modify]
_081DDA14:
	ldrb r3, [r5, o_MusicPlayerTrack_flags]
	movs r0, MPT_FLG_PITCHG
	tst r0, r3
	beq _081DDA52
	ldrb r1, [r4, o_SoundChannel_key]
	movs r0, o_MusicPlayerTrack_keyM
	ldrsb r0, [r5, r0]
	adds r2, r1, r0
	bpl _081DDA28
	movs r2, 0
_081DDA28:
	cmp r6, 0
	beq _081DDA46
	mov r0, r8
	ldr r3, [r0, o_SoundInfo_MidiKeyToCgbFreq]
	adds r1, r2, 0
	ldrb r2, [r5, o_MusicPlayerTrack_pitM]
	adds r0, r6, 0
	bl call_r3
	str r0, [r4, o_CgbChannel_frequency]
	ldrb r0, [r4, o_CgbChannel_modify]
	movs r1, CGB_CHANNEL_MO_PIT
	orrs r0, r1
	strb r0, [r4, o_CgbChannel_modify]
	b _081DDA52
_081DDA46:
	adds r1, r2, 0
	ldrb r2, [r5, o_MusicPlayerTrack_pitM]
	ldr r0, [r4, o_SoundChannel_wav]
	bl MidiKeyToFreq
	str r0, [r4, o_SoundChannel_frequency]
_081DDA52:
	ldr r4, [r4, o_SoundChannel_nextChannelPointer]
	cmp r4, 0
	bne _081DD9E6
_081DDA58:
	ldrb r0, [r5, o_MusicPlayerTrack_flags]
	movs r1, 0xF0
	ands r0, r1
	strb r0, [r5, o_MusicPlayerTrack_flags]
	mov r2, r9
_081DDA62:
	subs r2, 0x1
	ble _081DDA6C
	movs r0, MusicPlayerTrack_size
	adds r5, r0
	bgt _081DD9C8
_081DDA6C:
	ldr r0, lt2_ID_NUMBER
	str r0, [r7, o_MusicPlayerInfo_ident]
	pop {r0-r7}
	mov r8, r0
	mov r9, r1
	mov r10, r2
	mov r11, r3
	pop {r3}

call_r3:
	bx r3

	.align 2, 0
lt_gClockTable:     .word gClockTable
lt2_SOUND_INFO_PTR: .word SOUND_INFO_PTR
lt2_ID_NUMBER:      .word ID_NUMBER
	thumb_func_end MPlayMain

	thumb_func_start TrackStop
TrackStop:
	push {r4-r6,lr}
	adds r5, r1, 0
	ldrb r1, [r5, o_MusicPlayerTrack_flags]
	movs r0, MPT_FLG_EXIST
	tst r0, r1
	beq TrackStop_Done
	ldr r4, [r5, o_MusicPlayerTrack_chan]
	cmp r4, 0
	beq TrackStop_3
	movs r6, 0
TrackStop_Loop:
	ldrb r0, [r4, o_SoundChannel_statusFlags]
	cmp r0, 0
	beq TrackStop_2
	ldrb r0, [r4, o_SoundChannel_type]
	movs r3, TONEDATA_TYPE_CGB
	ands r0, r3
	beq TrackStop_1
	ldr r3, =SOUND_INFO_PTR
	ldr r3, [r3]
	ldr r3, [r3, o_SoundInfo_CgbOscOff]
	bl call_r3
TrackStop_1:
	strb r6, [r4, o_SoundChannel_statusFlags]
TrackStop_2:
	str r6, [r4, o_SoundChannel_track]
	ldr r4, [r4, o_SoundChannel_nextChannelPointer]
	cmp r4, 0
	bne TrackStop_Loop
TrackStop_3:
	str r4, [r5, o_MusicPlayerTrack_chan]
TrackStop_Done:
	pop {r4-r6}
	pop {r0}
	bx r0
	.pool
	thumb_func_end TrackStop

	thumb_func_start ChnVolSetAsm
ChnVolSetAsm:
	ldrb r1, [r4, o_SoundChannel_velocity]
	movs r0, o_SoundChannel_rhythmPan
	ldrsb r2, [r4, r0]
	movs r3, 0x80
	adds r3, r2
	muls r3, r1
	ldrb r0, [r5, o_MusicPlayerTrack_volMR]
	muls r0, r3
	asrs r0, 14
	cmp r0, 0xFF
	bls _081DDAE8
	movs r0, 0xFF
_081DDAE8:
	strb r0, [r4, o_SoundChannel_rightVolume]
	movs r3, 0x7F
	subs r3, r2
	muls r3, r1
	ldrb r0, [r5, o_MusicPlayerTrack_volML]
	muls r0, r3
	asrs r0, 14
	cmp r0, 0xFF
	bls _081DDAFC
	movs r0, 0xFF
_081DDAFC:
	strb r0, [r4, o_SoundChannel_leftVolume]
	bx lr
	thumb_func_end ChnVolSetAsm

	thumb_func_start ply_note
ply_note:
	push {r4-r7,lr}
	mov r4, r8
	mov r5, r9
	mov r6, r10
	mov r7, r11
	push {r4-r7}
	sub sp, 0x18
	str r1, [sp]
	adds r5, r2, 0
	ldr r1, =SOUND_INFO_PTR
	ldr r1, [r1]
	str r1, [sp, 0x4]
	ldr r1, =gClockTable
	adds r0, r1
	ldrb r0, [r0]
	strb r0, [r5, o_MusicPlayerTrack_gateTime]
	ldr r3, [r5, o_MusicPlayerTrack_cmdPtr]
	ldrb r0, [r3]
	cmp r0, 0x80
	bhs _081DDB46
	strb r0, [r5, o_MusicPlayerTrack_key]
	adds r3, 0x1
	ldrb r0, [r3]
	cmp r0, 0x80
	bhs _081DDB44
	strb r0, [r5, o_MusicPlayerTrack_velocity]
	adds r3, 0x1
	ldrb r0, [r3]
	cmp r0, 0x80
	bhs _081DDB44
	ldrb r1, [r5, o_MusicPlayerTrack_gateTime]
	adds r1, r0
	strb r1, [r5, o_MusicPlayerTrack_gateTime]
	adds r3, 0x1
_081DDB44:
	str r3, [r5, o_MusicPlayerTrack_cmdPtr]
_081DDB46:
	movs r0, 0
	str r0, [sp, 0x14]
	adds r4, r5, 0
	adds r4, o_MusicPlayerTrack_ToneData_type
	ldrb r2, [r4]
	movs r0, TONEDATA_TYPE_RHY | TONEDATA_TYPE_SPL
	tst r0, r2
	beq _081DDB98
	ldrb r3, [r5, o_MusicPlayerTrack_key]
	movs r0, TONEDATA_TYPE_SPL
	tst r0, r2
	beq _081DDB66
	ldr r1, [r5, o_MusicPlayerTrack_ToneData_keySplitTable]
	adds r1, r3
	ldrb r0, [r1]
	b _081DDB68
_081DDB66:
	adds r0, r3, 0
_081DDB68:
	lsls r1, r0, 1
	adds r1, r0
	lsls r1, 2
	ldr r0, [r5, o_MusicPlayerTrack_ToneData_wav]
	adds r1, r0
	mov r9, r1
	mov r6, r9
	ldrb r1, [r6]
	movs r0, TONEDATA_TYPE_SPL | TONEDATA_TYPE_RHY
	tst r0, r1
	beq _081DDB80
	b _081DDCEA
_081DDB80:
	movs r0, TONEDATA_TYPE_RHY
	tst r0, r2
	beq _081DDB9C
	ldrb r1, [r6, o_ToneData_pan_sweep]
	movs r0, 0x80
	tst r0, r1
	beq _081DDB94
	subs r1, TONEDATA_P_S_PAN
	lsls r1, 1
	str r1, [sp, 0x14]
_081DDB94:
	ldrb r3, [r6, o_SoundChannel_type]
	b _081DDB9C
_081DDB98:
	mov r9, r4
	ldrb r3, [r5, o_MusicPlayerTrack_key]
_081DDB9C:
	str r3, [sp, 0x8]
	ldr r6, [sp]
	ldrb r1, [r6, o_MusicPlayerInfo_priority]
	ldrb r0, [r5, o_MusicPlayerTrack_priority]
	adds r0, r1
	cmp r0, 0xFF
	bls _081DDBAC
	movs r0, 0xFF
_081DDBAC:
	str r0, [sp, 0x10]
	mov r6, r9
	ldrb r0, [r6, o_ToneData_type]
	movs r6, TONEDATA_TYPE_CGB
	ands r6, r0
	str r6, [sp, 0xC]
	beq _081DDBEC
	ldr r0, [sp, 0x4]
	ldr r4, [r0, o_SoundInfo_cgbChans]
	cmp r4, 0
	bne _081DDBC4
	b _081DDCEA
_081DDBC4:
	subs r6, 0x1
	lsls r0, r6, 6
	adds r4, r0
	ldrb r1, [r4, o_CgbChannel_statusFlags]
	movs r0, SOUND_CHANNEL_SF_ON
	tst r0, r1
	beq _081DDC40
	movs r0, SOUND_CHANNEL_SF_STOP
	tst r0, r1
	bne _081DDC40
	ldrb r1, [r4, o_CgbChannel_priority]
	ldr r0, [sp, 0x10]
	cmp r1, r0
	bcc _081DDC40
	beq _081DDBE4
	b _081DDCEA
_081DDBE4:
	ldr r0, [r4, o_CgbChannel_track]
	cmp r0, r5
	bcs _081DDC40
	b _081DDCEA
_081DDBEC:
	ldr r6, [sp, 0x10]
	adds r7, r5, 0
	movs r2, 0
	mov r8, r2
	ldr r4, [sp, 0x4]
	ldrb r3, [r4, o_SoundInfo_maxChans]
	adds r4, o_SoundInfo_chans
_081DDBFA:
	ldrb r1, [r4, o_SoundChannel_statusFlags]
	movs r0, SOUND_CHANNEL_SF_ON
	tst r0, r1
	beq _081DDC40
	movs r0, SOUND_CHANNEL_SF_STOP
	tst r0, r1
	beq _081DDC14
	cmp r2, 0
	bne _081DDC18
	adds r2, 0x1
	ldrb r6, [r4, o_SoundChannel_priority]
	ldr r7, [r4, o_SoundChannel_track]
	b _081DDC32
_081DDC14:
	cmp r2, 0
	bne _081DDC34
_081DDC18:
	ldrb r0, [r4, o_SoundChannel_priority]
	cmp r0, r6
	bcs _081DDC24
	adds r6, r0, 0
	ldr r7, [r4, o_SoundChannel_track]
	b _081DDC32
_081DDC24:
	bhi _081DDC34
	ldr r0, [r4, o_SoundChannel_track]
	cmp r0, r7
	bls _081DDC30
	adds r7, r0, 0
	b _081DDC32
_081DDC30:
	bcc _081DDC34
_081DDC32:
	mov r8, r4
_081DDC34:
	adds r4, SoundChannel_size
	subs r3, 0x1
	bgt _081DDBFA
	mov r4, r8
	cmp r4, 0
	beq _081DDCEA
_081DDC40:
	adds r0, r4, 0
	bl ClearChain
	movs r1, 0
	str r1, [r4, o_SoundChannel_prevChannelPointer]
	ldr r3, [r5, o_MusicPlayerTrack_chan]
	str r3, [r4, o_SoundChannel_nextChannelPointer]
	cmp r3, 0
	beq _081DDC54
	str r4, [r3, o_SoundChannel_prevChannelPointer]
_081DDC54:
	str r4, [r5, o_MusicPlayerTrack_chan]
	str r5, [r4, o_SoundChannel_track]
	ldrb r0, [r5, o_MusicPlayerTrack_lfoDelay]
	strb r0, [r5, o_MusicPlayerTrack_lfoDelayC]
	cmp r0, r1
	beq _081DDC66
	adds r1, r5, 0
	bl clear_modM
_081DDC66:
	ldr r0, [sp]
	adds r1, r5, 0
	bl TrkVolPitSet
	ldr r0, [r5, o_MusicPlayerTrack_gateTime]
	str r0, [r4, o_SoundChannel_gateTime]
	ldr r0, [sp, 0x10]
	strb r0, [r4, o_SoundChannel_priority]
	ldr r0, [sp, 0x8]
	strb r0, [r4, o_SoundChannel_key]
	ldr r0, [sp, 0x14]
	strb r0, [r4, o_SoundChannel_rhythmPan]
	mov r6, r9
	ldrb r0, [r6, o_ToneData_type]
	strb r0, [r4, o_SoundChannel_type]
	ldr r7, [r6, o_ToneData_wav]
	str r7, [r4, o_SoundChannel_wav]
	ldr r0, [r6, o_ToneData_attack]
	str r0, [r4, o_SoundChannel_attack]
	ldrh r0, [r5, o_MusicPlayerTrack_pseudoEchoVolume]
	strh r0, [r4, o_SoundChannel_pseudoEchoVolume]
	bl ChnVolSetAsm
	ldrb r1, [r4, o_SoundChannel_key]
	movs r0, o_MusicPlayerTrack_keyM
	ldrsb r0, [r5, r0]
	adds r3, r1, r0
	bpl _081DDCA0
	movs r3, 0
_081DDCA0:
	ldr r6, [sp, 0xC]
	cmp r6, 0
	beq _081DDCCE
	mov r6, r9
	ldrb r0, [r6, o_ToneData_length]
	strb r0, [r4, o_CgbChannel_length]
	ldrb r1, [r6, o_ToneData_pan_sweep]
	movs r0, 0x80
	tst r0, r1
	bne _081DDCBA
	movs r0, 0x70
	tst r0, r1
	bne _081DDCBC
_081DDCBA:
	movs r1, 0x8
_081DDCBC:
	strb r1, [r4, o_CgbChannel_sweep]
	ldrb r2, [r5, o_MusicPlayerTrack_pitM]
	adds r1, r3, 0
	ldr r0, [sp, 0xC]
	ldr r3, [sp, 0x4]
	ldr r3, [r3, o_SoundInfo_MidiKeyToCgbFreq]
	bl call_r3
	b _081DDCDC
_081DDCCE:
	ldr r0, [r5, o_MusicPlayerTrack_unk_3C]
	str r0, [r4, o_SoundChannel_count]
	ldrb r2, [r5, o_MusicPlayerTrack_pitM]
	adds r1, r3, 0
	adds r0, r7, 0
	bl MidiKeyToFreq
_081DDCDC:
	str r0, [r4, o_SoundChannel_frequency]
	movs r0, SOUND_CHANNEL_SF_START
	strb r0, [r4, o_SoundChannel_statusFlags]
	ldrb r1, [r5, o_MusicPlayerTrack_flags]
	movs r0, 0xF0
	ands r0, r1
	strb r0, [r5, o_MusicPlayerTrack_flags]
_081DDCEA:
	add sp, 0x18
	pop {r0-r7}
	mov r8, r0
	mov r9, r1
	mov r10, r2
	mov r11, r3
	pop {r0}
	bx r0
	.pool
	thumb_func_end ply_note

	thumb_func_start ply_endtie
ply_endtie:
	push {r4,r5}
	ldr r2, [r1, o_MusicPlayerTrack_cmdPtr]
	ldrb r3, [r2]
	cmp r3, 0x80
	bhs _081DDD16
	strb r3, [r1, o_MusicPlayerTrack_key]
	adds r2, 0x1
	str r2, [r1, o_MusicPlayerTrack_cmdPtr]
	b _081DDD18
_081DDD16:
	ldrb r3, [r1, o_MusicPlayerTrack_key]
_081DDD18:
	ldr r1, [r1, o_MusicPlayerTrack_chan]
	cmp r1, 0
	beq _081DDD40
	movs r4, SOUND_CHANNEL_SF_START | SOUND_CHANNEL_SF_ENV
	movs r5, SOUND_CHANNEL_SF_STOP
_081DDD22:
	ldrb r2, [r1, o_SoundChannel_statusFlags]
	tst r2, r4
	beq _081DDD3A
	tst r2, r5
	bne _081DDD3A
	ldrb r0, [r1, o_SoundChannel_midiKey]
	cmp r0, r3
	bne _081DDD3A
	movs r0, SOUND_CHANNEL_SF_STOP
	orrs r2, r0
	strb r2, [r1, o_SoundChannel_statusFlags]
	b _081DDD40
_081DDD3A:
	ldr r1, [r1, o_SoundChannel_nextChannelPointer]
	cmp r1, 0
	bne _081DDD22
_081DDD40:
	pop {r4,r5}
	bx lr
	thumb_func_end ply_endtie

	thumb_func_start clear_modM
clear_modM:
	movs r2, 0
	strb r2, [r1, o_MusicPlayerTrack_modM]
	strb r2, [r1, o_MusicPlayerTrack_lfoSpeedC]
	ldrb r2, [r1, o_MusicPlayerTrack_modT]
	cmp r2, 0
	bne _081DDD54
	movs r2, MPT_FLG_PITCHG
	b _081DDD56
_081DDD54:
	movs r2, MPT_FLG_VOLCHG
_081DDD56:
	ldrb r3, [r1, o_MusicPlayerTrack_flags]
	orrs r3, r2
	strb r3, [r1, o_MusicPlayerTrack_flags]
	bx lr
	thumb_func_end clear_modM

	thumb_func_start ld_r3_tp_adr_i
ld_r3_tp_adr_i_unchecked:
	ldr r2, [r1, o_MusicPlayerTrack_cmdPtr]
	adds r3, r2, 1
	str r3, [r1, o_MusicPlayerTrack_cmdPtr]
	ldrb r3, [r2]
	bx lr
	thumb_func_end ld_r3_tp_adr_i

	thumb_func_start ply_lfos
ply_lfos:
	mov r12, lr
	bl ld_r3_tp_adr_i_unchecked
	strb r3, [r1, o_MusicPlayerTrack_lfoSpeed]
	cmp r3, 0
	bne _081DDD7C
	bl clear_modM
_081DDD7C:
	bx r12
	thumb_func_end ply_lfos

	thumb_func_start ply_mod
ply_mod:
	mov r12, lr
	bl ld_r3_tp_adr_i_unchecked
	strb r3, [r1, o_MusicPlayerTrack_mod]
	cmp r3, 0
	bne _081DDD90
	bl clear_modM
_081DDD90:
	bx r12
	thumb_func_end ply_mod

	.align 2, 0 @ Don't pad with nop.

    .bss
gUnknown_03001300:
    .space 0x40
    .size gUnknown_03001300, .-gUnknown_03001300

    .global gMPlayTrack_BGM
gMPlayTrack_BGM:
    .space 0x320
    .size gMPlayTrack_BGM, .-gMPlayTrack_BGM

    .global gMPlayTrack_SE1
gMPlayTrack_SE1:
    .space 0xF0
    .size gMPlayTrack_SE1, .-gMPlayTrack_SE1

    .global gMPlayTrack_SE2
gMPlayTrack_SE2:
    .space 0x2D0
    .size gMPlayTrack_SE2, .-gMPlayTrack_SE2

    .global gMPlayTrack_SE3
gMPlayTrack_SE3:
    .space 0x50
    .size gMPlayTrack_SE3, .-gMPlayTrack_SE3
