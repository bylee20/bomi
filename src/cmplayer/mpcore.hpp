#ifndef MPCORE_HPP
#define MPCORE_HPP

#ifdef __cplusplus
extern "C" {
#define new ____new
enum seek_type {MPSEEK_NONE, MPSEEK_RELATIVE, MPSEEK_ABSOLUTE, MPSEEK_FACTOR};
#endif

#undef bswap_16
#undef bswap_32
#include <core/mp_core.h>

#ifdef __cplusplus
#undef new
#endif

#define run_command mpctx_run_command

enum MpError {NoMpError = 0, UserInterrupted, OpenFileError, OpenDemuxerError, OpenStreamError, NoStreamError, InitVideoFilterError, InitAudioFilterError};
extern void exit_player(struct MPContext *mpctx, enum exit_reason how, int rc);
extern enum MpError terminate_playback(struct MPContext *mpctx, enum MpError error);
enum MpError prepare_to_play_current_file(struct MPContext *mpctx);
enum MpError start_to_play_current_file(struct MPContext *mpctx);
#if defined(__cplusplus) && defined(PLAY_ENGINE_P)
void (*mpctx_paused_changed)(struct MPContext *) = nullptr;
void (*mpctx_play_started)(struct MPContext *) = nullptr;
inline void quit_player(struct MPContext *mpctx, enum exit_reason how) {exit_player(mpctx, how, 0);}
inline void mpctx_delete(struct MPContext *mpctx) {quit_player(mpctx, EXIT_NONE);}
extern int mpv_init(struct MPContext *mpctx, int argc, char **argv);
extern double get_wakeup_period(struct MPContext *mpctx);
extern int play_next_file();
extern int run_playback();
extern void idle_loop(struct MPContext *mpctx);
#else
extern void mpctx_run_command(struct MPContext *, struct mp_cmd *);
extern void (*mpctx_paused_changed)(struct MPContext *);
extern void (*mpctx_play_started)(struct MPContext *);
static inline void play_current_file(struct MPContext *mpctx) {
	prepare_to_play_current_file(mpctx);
	start_to_play_current_file(mpctx);
}
#endif

#ifdef __cplusplus
}
#endif
#endif // MPCORE_HPP
