/* This file is part of FLUID.MID.
 * Copyright (c) 2020 bsdf.

 * FLUID.MID is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * FLUID.MID is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with FLUID.MID.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "fluid.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <deadbeef/deadbeef.h>
#include <fluidsynth.h>

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
  DB_fileinfo_t info;
  DB_playItem_t *it;
} fmidi_info_t;

static const char * exts[] = { "mid", NULL };

static fluid_settings_t *settings;
static fluid_synth_t *synth;
static fluid_player_t *player;

int
get_midi_divisions(const char *file)
{
  FILE *fp = fopen(file, "rb");
  if (fp == NULL)
    return -1;

  if (fseek(fp, 12, SEEK_SET) != 0)
    return -1;

  int divisions = (fgetc(fp) << 8) | fgetc(fp);
  fclose(fp);

  return divisions;
}

static DB_fileinfo_t *
fmidi_open (uint32_t hints)
{
  trace("in fmidi_open\n");
  DB_fileinfo_t *_info = malloc(sizeof(fmidi_info_t));
  fmidi_info_t *info = (fmidi_info_t *) _info;
  memset(info, 0, sizeof(fmidi_info_t));
  return _info;
}

#define DEFAULT_SOUNDFONT "/usr/share/sounds/sf2/OPL-3_FM_128M.sf2"

static int
fmidi_seek_sample (DB_fileinfo_t *_info, int sample)
{
  trace("in fmidi_seek_sample\n");
  return 0;
}

static int
fmidi_init (DB_fileinfo_t *_info, DB_playItem_t *it)
{
  trace("in fmidi_init\n");
  int ret = 0;
  settings = new_fluid_settings();
  synth = new_fluid_synth(settings);
  player = new_fluid_player(synth);

  /* fluid_set_log_function(FLUID_DBG, fluid_default_log_function, NULL); */

  int font = fluid_synth_sfload(synth, DEFAULT_SOUNDFONT, 1);
  if (font == FLUID_FAILED)
    {
      trace("failed to load soundfont.\n");
      ret = -1;
    }

  deadbeef->pl_lock();
  const char *uri = strdupa(deadbeef->pl_find_meta (it, ":URI"));
  deadbeef->pl_unlock();

  fluid_player_add(player, uri);
  fluid_player_play(player);

  _info->fmt.bps = 16;
  _info->fmt.channels = 2;
  _info->fmt.samplerate = 44100;
  for (int i = 0; i < _info->fmt.channels; i++)
    {
      _info->fmt.channelmask |= 1 << i;
    }
  _info->readpos = 0;
  _info->plugin = &plugin;

  fmidi_seek_sample(_info, 0);

  return ret;
}

static void
fmidi_free (DB_fileinfo_t *_info)
{
  trace("in fmidi_free\n");
  delete_fluid_player(player);
  delete_fluid_synth(synth);
  delete_fluid_settings(settings);
}

static int
fmidi_read (DB_fileinfo_t *_info, char *bytes, int size)
{
  int status = fluid_player_get_status(player);
  if (status != FLUID_PLAYER_PLAYING)
    {
      return 0;
    }

  /* fluid_player_set_bpm(player, 160); */
  if (fluid_synth_write_s16(synth, size / 4, bytes, 0, 2, bytes, 1, 2) == FLUID_FAILED)
    {
      trace("error calling fluid_synth_write_s16\n");
      return 0;
    }

  return size;
}

static int
fmidi_seek (DB_fileinfo_t *_info, float time)
{
  trace("in fmidi_seek\n");
  return 0;
}

static DB_playItem_t *
fmidi_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname)
{
  if (fluid_is_midifile(fname) == 0)
    {
      trace("file is not a midi\n");
      return NULL;
    }

  int ppq = get_midi_divisions(fname);

  fluid_settings_t *settings = new_fluid_settings();
  fluid_synth_t *synth = new_fluid_synth(settings);
  fluid_player_t *player = new_fluid_player(synth);

  fluid_player_add(player, fname);
  fluid_player_play(player);

  // read some tdata so that ticks doesn't return 0
  uint32_t buf[1];
  fluid_synth_write_s16(synth, 1, buf, 0, 2, buf, 1, 2);

  int ticks = fluid_player_get_total_ticks(player);
  int bpm = fluid_player_get_bpm(player);
  float tps = 60.0 / (bpm * ppq);
  float secs = ticks * tps;

  delete_fluid_player(player);
  delete_fluid_synth(synth);
  delete_fluid_settings(settings);

  DB_playItem_t *it = deadbeef->pl_item_alloc_init(fname, plugin.plugin.id);
  deadbeef->pl_add_meta(it, "title", NULL);
  deadbeef->pl_add_meta(it, ":FILETYPE", "MID");
  deadbeef->plt_set_item_duration (plt, it, secs);
  after = deadbeef->plt_insert_item(plt, after, it);
  deadbeef->pl_item_unref(it);

  return after;
}

static int
fmidi_start (void)
{
  trace("in fmidi_start\n");
  return 0;
}

static int
fmidi_stop (void)
{
  trace("in fmidi_stop\n");
  return 0;
}

static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "FLUID.MID",
    .plugin.id = "fmidi",
    .plugin.descr = "fluidsynth MIDI decoder",
    .plugin.copyright = "copyright message - author(s), license, etc",
    .plugin.start = fmidi_start,
    .plugin.stop = fmidi_stop,
    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .open = fmidi_open,
    .init = fmidi_init,
    .free = fmidi_free,
    .read = fmidi_read,
    .seek = fmidi_seek,
    .seek_sample = fmidi_seek_sample,
    .insert = fmidi_insert,
    .exts = exts,
};

DB_plugin_t *
fmidi_load (DB_functions_t *api)
{
  deadbeef = api;
  return DB_PLUGIN (&plugin);
}
