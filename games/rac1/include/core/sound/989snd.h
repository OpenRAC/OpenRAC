#ifndef CORE_SOUND_989SND_H
#define CORE_SOUND_989SND_H

/* Functions defined in src/core/sound/989snd.c. */

#include "common.h"

extern void snd_AutoReverb(int, int, int, int);
extern int snd_BankLoadByLoc(void *, int);
extern void snd_BankLoadFromEE_CB(int, int, unsigned long);
extern void snd_ContinueAllSoundsInGroup(int);
extern void snd_ContinueVAGStream(void *);
extern int snd_FlushSoundCommands(void);
extern int snd_GetDopplerPitchMod(int);
extern int snd_GetMovieNAX(void);
extern void snd_GetVAGStreamTimeRemaining_CB(int, void *, long);
extern int snd_InitVAGStreamingEx(int, int, int, int);
extern void snd_IsVAGStreamBuffered_CB(int, void *, long);
extern void snd_PauseAllSoundsInGroup(int);
extern void snd_PauseVAGStream(int);
extern void snd_PlayVAGStreamByLocEx_CB(int, int, int, int, short, int, int, int, int, void *, long);
extern void snd_PreAllocReverbWorkArea(int, int);
extern void snd_ResetMovieSound(void);
extern void snd_ResolveBankXREFS(void);
extern void snd_SetGroupVoiceRange(int, int, int);
extern void snd_SetMasterVolume(int, int);
extern void snd_SetPlaybackMode(int);
extern void snd_SoundIsStillPlaying_CB(int, void *, long);
extern void snd_StartMovieSound(int, int, int, int, int);
extern void snd_StartSoundSystem(void);
extern void snd_StopAllStreams(void);
extern int snd_StreamSafeCdBreak(void);
extern void snd_StreamSafeCdCallback(void *);
extern int snd_StreamSafeCdGetError(void);
extern int snd_StreamSafeCdRead(int, int, int, void *);
extern int snd_StreamSafeCdSync(int);
extern int snd_StreamSafeCheckCDIdle(int);
extern int snd_UnloadBank(int);
extern int snd_UpdateMovieADPCM(int, int);
extern int snd_close_movie_sound(void);
extern int snd_init_movie_sound(int, int, int, int, int, int);
extern void snd_play_sound_vol_pan_pmpb(int, int, int, int, int, int, int, void *);
extern void snd_reenable_command_flush(void);
extern void snd_reset_state_and_flush_commands(void);
extern void snd_set_mixer_mode(int, int);
extern void snd_set_reverb_ex(int, int, int, int, int);
extern void snd_set_sound_params_cb(int, int, int, int, int, int, void *, long);
extern void snd_stop_sound(int);

#endif /* CORE_SOUND_989SND_H */
