/*************************************************************************/
/*  audio_stream_transitioner.h                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef AUDIO_STREAM_TRANSITIONER_H
#define AUDIO_STREAM_TRANSITIONER_H

#include "core/reference.h"
#include "core/resource.h"
#include "servers/audio/audio_stream.h"

class AudioStreamPlaybackTransitioner;

class AudioStreamTransitioner : public AudioStream {
	GDCLASS(AudioStreamTransitioner, AudioStream)
	OBJ_SAVE_TYPE(AudioStream)

private:
	friend class AudioStreamPlaybackTransitioner;
	uint64_t pos;
	int sample_rate;
	bool stereo;
	int clip_count;
	int transition_count;
	int bpm;
	double time;

	enum {
		MAX_STREAMS = 64,
		MAX_TRANSITIONS = 48
	};

	struct Transition {
		bool t_active;
		int fade_in_beats;
		int fade_out_beats;
	};

	Transition transitions[MAX_TRANSITIONS];
	Transition active_transition;

	Ref<AudioStream> clips[MAX_STREAMS];
	Ref<AudioStream> t_clip;
	bool t_clip_active;
	int active_clip_number;
	int fading_clip_number;
	Set<AudioStreamPlaybackTransitioner *> playbacks;

public:
	void reset();
	void set_position(uint64_t pos);

	void set_bpm(int p_bpm);
	virtual int get_bpm() const;

	void set_list_clip(int clip_number, Ref<AudioStream> p_clip);
	Ref<AudioStream> get_list_clip(int clip_number);

	void set_transition_count(int p_transition_count);
	int get_transition_count();

	void set_clip_count(int p_clip_count);
	int get_clip_count();

	void set_t_clip_active(bool active);
	bool get_t_clip_active();

	void add_transition_clip(Ref<AudioStream> transition_clip);
	Ref<AudioStream> get_transition_clip();

	void set_transition_fade_in(int transition_number, int fade_in);
	int get_transition_fade_in(int transition_number);

	void set_transition_fade_out(int transition_number, int fade_out);
	int get_transition_fade_out(int transition_number);

	void set_active_transition(int transition_number, bool trigger);
	bool get_transition_state(int transition_number);

	void set_active_clip_number(int clip_number);
	int get_active_clip_number();

	void go_to_clip(int clip_number, int transition_number);

	virtual Ref<AudioStreamPlayback> instance_playback();
	virtual String get_stream_name() const;
	virtual float get_length() const { return 0; }
	AudioStreamTransitioner();

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &property) const;
};

class AudioStreamPlaybackTransitioner : public AudioStreamPlayback {
	GDCLASS(AudioStreamPlaybackTransitioner, AudioStreamPlayback)
	friend class AudioStreamTransitioner;

private:
	enum {
		MIX_BUFFER_SIZE = 256
	};
	enum {
		MIX_FRAC_BITS = 13,
		MIX_FRAC_LEN = (1 << MIX_FRAC_BITS),
		MIX_FRAC_MASK = MIX_FRAC_LEN - 1,
	};
	AudioFrame pcm_buffer[MIX_BUFFER_SIZE];
	AudioFrame aux_buffer[MIX_BUFFER_SIZE];

	Ref<AudioStreamTransitioner> transitioner;
	Ref<AudioStreamPlayback> playbacks[AudioStreamTransitioner::MAX_STREAMS];
	Ref<AudioStreamPlayback> t_playback;

	int current;
	int previous;
	int fade_in_samples_total;
	int fade_out_samples_total;
	int fade_in_t_clip_samples_total;
	int fade_out_t_clip_samples_total;
	int transition_samples_total;
	int clip_samples_total;
	int t_clip_samples_total;

	int transition_samples;
	int fade_in_samples;
	int fade_out_samples;
	int fade_in_t_clip_samples;
	int fade_out_t_clip_samples;
	int t_clip_samples;

	int beat_size;
	int fading_beat_size;
	int t_clip_beat_size;

	bool fading;

	bool active;

	virtual void _update_playback_instances();

public:
	virtual void start(float p_from_pos = 0.0);
	virtual void stop();
	virtual bool is_playing() const;
	void clear_buffer(int samples);
	virtual int get_loop_count() const; // times it looped
	virtual float get_playback_position() const;
	virtual void seek(float p_time);
	void add_stream_to_buffer(Ref<AudioStreamPlayback> playback, int samples, float p_rate_scale, float initial_volume, float final_volume);
	virtual void mix(AudioFrame *p_buffer, float p_rate_scale, int p_frames);
	virtual float get_length() const;
	AudioStreamPlaybackTransitioner();
	~AudioStreamPlaybackTransitioner();
};

#endif // AUDIO_STREAM_TRANSITIONER_H
