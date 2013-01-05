/*
 * This file is part of mplayer2.
 *
 * mplayer2 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mplayer2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with mplayer2; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#include <stdio.h>
#include "config.h"
#include "macosx_finder_args.h"

static play_tree_t *files = NULL;

play_tree_t *play_tree_from_filenames(NSArray *filenames);
void macosx_wait_fileopen_events(void);
void macosx_redirect_output_to_logfile(const char *filename);
bool psn_matches_current_process(const char *psn_arg_to_check);
NSArray *argv_to_filenames(int argc, char **argv);

@interface FileOpenDelegate : NSObject
- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames;
@end

@implementation FileOpenDelegate
- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames
{
    files = play_tree_from_filenames(filenames);
    [NSApp stop:nil]; // stop the runloop (give back control to mplayer2 code)
}
@end

play_tree_t *play_tree_from_filenames(NSArray *filenames) {
    NSArray *sorted_filenames = [filenames
        sortedArrayUsingSelector:@selector(compare:)];
    play_tree_t *new_tree = play_tree_new();
    play_tree_t *last_entry = nil;
    for (NSString *filename in sorted_filenames) {
        play_tree_t *entry = play_tree_new();
        play_tree_add_file(entry, [filename UTF8String]);

        if (last_entry)
          play_tree_append_entry(new_tree, entry);
        else
          play_tree_set_child(new_tree, entry);

        last_entry = entry;
    }
    return new_tree;
}

void macosx_wait_fileopen_events()
{
    NSApp = [NSApplication sharedApplication];
    [NSApp setDelegate: [[[FileOpenDelegate alloc] init] autorelease]];
    [NSApp run]; // block until we recive the fileopen events
}

void macosx_redirect_output_to_logfile(const char *filename)
{
    NSString *log_path = [NSHomeDirectory() stringByAppendingPathComponent:
        [@"Library/Logs/" stringByAppendingFormat:@"%s.log", filename]];
    freopen([log_path fileSystemRepresentation], "a", stdout);
    freopen([log_path fileSystemRepresentation], "a", stderr);
}

bool psn_matches_current_process(const char *psn_arg_to_check)
{
    ProcessSerialNumber psn;
    char psn_arg[5+10+1+10+1];

    GetCurrentProcess(&psn);
    snprintf(psn_arg, 5+10+1+10+1, "-psn_%u_%u",
             psn.highLongOfPSN, psn.lowLongOfPSN);
    psn_arg[5+10+1+10]=0;

    return strcmp(psn_arg, psn_arg_to_check) == 0;
}

NSArray *argv_to_filenames(int argc, char **argv)
{
    NSMutableArray *filenames = [NSMutableArray arrayWithCapacity:argc-1];
    for (int i=1; i < argc; i++) {
        NSString *filename = [NSString stringWithUTF8String: argv[i]];
        [filenames addObject: filename];
    }
    return filenames;
}

play_tree_t *macosx_finder_args(m_config_t *config, int argc, char **argv)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#ifndef CONFIG_SDL
    if (argc == 2 && psn_matches_current_process(argv[1])) {
        macosx_redirect_output_to_logfile("mplayer2");
        m_config_set_option0(config, "quiet", NULL, false);
        macosx_wait_fileopen_events();
    }

    // SDL's bootstrap code (include SDL.h in mplayer.c) renames main() to
    // SDL_main() with a macro and defines it's own main(). There it starts a
    // minimal Cocoa Application to wait for file open events.
    // After the events come in, SDL will call the original main() with argv
    // set to the list of files to open.

    // NSProcessInfo still holds the original arguments of the process, so we
    // can use that to figure out if we got called from the Finder.
#else
    NSArray *args = [[NSProcessInfo processInfo] arguments];
    if ([args count] == 2) {
        const char *psn = [[args objectAtIndex:1] UTF8String];

        if (psn_matches_current_process(psn)) {
            macosx_redirect_output_to_logfile("mplayer2");
            m_config_set_option0(config, "quiet", NULL, false);
            files = play_tree_from_filenames(argv_to_filenames(argc, argv));
        }
    }
#endif
    [pool release];
    return files;
}
