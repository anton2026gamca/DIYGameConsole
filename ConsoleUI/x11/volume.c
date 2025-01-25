#include <stdio.h>
#include <alsa/asoundlib.h>
#include "time.h"

#define VOLUMEBAR_DURATION 1.5
struct Volume {
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card; // Use "default" for default sound card
    const char *selem_name; // Use "Master" for main volume control
    snd_mixer_elem_t *elem;
    long raw_value;
    int value;
    bool is_muted;
    struct timespec last_change_time;
};
struct Volume volume;

int VolumeInit()
{
    volume.card = "default";
    volume.selem_name = "Master";

    // Open mixer
    if (snd_mixer_open(&volume.handle, 0) < 0) {
        fprintf(stderr, "ERROR: VOLUME: Could not open mixer.\n");
        return 1;
    }

    // Attach mixer to sound card
    if (snd_mixer_attach(volume.handle, volume.card) < 0) {
        fprintf(stderr, "ERROR: VOLUME: Could not attach mixer to card.\n");
        snd_mixer_close(volume.handle);
        return 1;
    }

    // Load mixer elements
    if (snd_mixer_selem_register(volume.handle, NULL, NULL) < 0) {
        fprintf(stderr, "ERROR: VOLUME: Could not register mixer elements.\n");
        snd_mixer_close(volume.handle);
        return 1;
    }
    if (snd_mixer_load(volume.handle) < 0) {
        fprintf(stderr, "ERROR: VOLUME: Could not load mixer elements.\n");
        snd_mixer_close(volume.handle);
        return 1;
    }

    // Allocate simple element ID
    snd_mixer_selem_id_malloc(&volume.sid);
    snd_mixer_selem_id_set_name(volume.sid, volume.selem_name);

    // Find mixer element
    volume.elem = snd_mixer_find_selem(volume.handle, volume.sid);
    if (!volume.elem) {
        fprintf(stderr, "ERROR: VOLUME: Could not find mixer element.\n");
        snd_mixer_selem_id_free(volume.sid);
        snd_mixer_close(volume.handle);
        return 1;
    }

    volume.raw_value = 0;
#ifdef VOLUME_DEBUG
    printf("Monitoring volume changes on '%s'...\n", volume.selem_name);
#endif
    return 0;
}

void VolumeUpdate()
{
    if (snd_mixer_handle_events(volume.handle) < 0)
    {
        fprintf(stderr, "ERROR: VOLUME: Could not handle mixer events.\n");
    }

    int is_not_muted = 0;
    snd_mixer_selem_get_playback_volume(volume.elem, SND_MIXER_SCHN_FRONT_LEFT, &volume.raw_value);
    snd_mixer_selem_get_playback_switch(volume.elem, SND_MIXER_SCHN_FRONT_LEFT, &is_not_muted);

    if (volume.value != (int)(volume.raw_value / 655))
    {
        clock_gettime(CLOCK_REALTIME, &volume.last_change_time);
        volume.value = (int)(volume.raw_value / 655);
#ifdef VOLUME_DEBUG
        printf("Volume changed: %ld%%\n", volume.value);
#endif
    }
    if (volume.is_muted != !is_not_muted)
    {
        clock_gettime(CLOCK_REALTIME, &volume.last_change_time);
        volume.is_muted = !is_not_muted;
#ifdef VOLUME_DEBUG
        if (volume.is_muted)
            printf("Volume muted\n");
        else
            printf("Volume unmuted\n");
#endif
    }
}

void ChangeVolumeBy(int change)
{
    char *sign = "+";
    if (change < 0)
    {
        change = -change;
        sign = "-";
    }

    char command[25];
#ifndef VOLUME_DEBUG
    sprintf(command, "amixer sset %s %d%%%s > /dev/null 2>&1", volume.selem_name, change, sign);
#else
    sprintf(command, "amixer sset %s %d%%%s", volume.selem_name, change, sign);
#endif
    system(command);
}

void VolumeDeinit()
{
    snd_mixer_selem_id_free(volume.sid);
    snd_mixer_close(volume.handle);
}