	.include "MPlayDef.s"

	.equ	se_m_lock_on_grp, voicegroup128
	.equ	se_m_lock_on_pri, 4
	.equ	se_m_lock_on_rev, reverb_set+50
	.equ	se_m_lock_on_mvl, 127
	.equ	se_m_lock_on_key, 0
	.equ	se_m_lock_on_tbs, 1
	.equ	se_m_lock_on_exg, 0
	.equ	se_m_lock_on_cmp, 1

	.section .rodata
	.global	se_m_lock_on
	.align	2



se_m_lock_on_1:
	.byte	KEYSH , se_m_lock_on_key+0
	.byte	TEMPO , 150*se_m_lock_on_tbs/2
	.byte		VOICE , 40
	.byte		BENDR , 12
	.byte		PAN   , c_v+0
	.byte		VOL   , 110*se_m_lock_on_mvl/mxv
	.byte		BEND  , c_v+0
	.byte		N01   , Cn4 , v100
	.byte	W01
	.byte		        Gn3 , v064
	.byte	W01
	.byte		N03   , Bn3 , v100
	.byte	W01
	.byte		PAN   , c_v+6
	.byte		BEND  , c_v+6
	.byte	W01
	.byte		PAN   , c_v-5
	.byte		BEND  , c_v-6
	.byte	W02
	.byte		PAN   , c_v+6
	.byte		BEND  , c_v+6
	.byte		N01   , Cn4 
	.byte	W01
	.byte		        Gn3 , v064
	.byte	W02
	.byte		PAN   , c_v-5
	.byte		BEND  , c_v-6
	.byte		N01   , Bn3 , v100
	.byte	W01
	.byte		        Gn3 , v064
	.byte	W02
	.byte		PAN   , c_v+0
	.byte		BEND  , c_v+0
	.byte		N06   , Bn3 , v100
	.byte	W06
	.byte	FINE


	.align	2

se_m_lock_on:
	.byte	1	
	.byte	0	
	.byte	se_m_lock_on_pri	
	.byte	se_m_lock_on_rev	

	.4byte	se_m_lock_on_grp

	.4byte	se_m_lock_on_1

	.end
