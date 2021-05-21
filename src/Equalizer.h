/**
 *
 * 10-Band Audio Equalizer
 * Code shamelessly stolen from libvlc
 **/

#pragma once

#include <string>
#include <array>
#include <cmath>
#include <string.h>
#include <mutex>
#include <stdio.h>

namespace libmediafy {
    const int NUM_BANDS = 10;
    const int NUM_PRESETS = 18;
    const float EQZ_IN_FACTOR = 0.25f;
    const float frequency_table[10] = {
            31.25, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000,
    };

    struct EqualizerParams {
        std::string name;
        int nb_bands;
        float preamp;
        std::array<float, NUM_BANDS> amp;
    };

    struct EqualizerConfig {
        int band_count;
        struct {
            float frequency;
            float alpha;
            float beta;
            float gamma;
        } band[NUM_BANDS];
    };

    struct EqualizerState {
        /* Filter static config */
        int nb_bands;
        std::array<float, NUM_BANDS> alpha;
        std::array<float, NUM_BANDS> beta;
        std::array<float, NUM_BANDS> gamma;

        /* Filter dyn config */
        std::array<float, NUM_BANDS> amp;   /* Per band amp */
        float gamp;   /* Global preamp */
        bool two_pass;

        /* Filter state */
        float x[32][2];
        float y[32][128][2];

        /* Second filter state */
        float x2[32][2];
        float y2[32][128][2];
    };

    class Equalizer {
    public:
        /// The only real constructor
        Equalizer(int sample_rate) : sample_rate(sample_rate) {
            printf("Equalizer constructor\n");
            two_pass = false;
            current_params = presets[0];
            reset(sample_rate);
            printf("Equalizer constructor passing by:::\n");
        }

        void reset(int sample_rate) {
            this->sample_rate = sample_rate;
            init_config();
            set_params(current_params);
        }

        // This one's trash!
        Equalizer(Equalizer&) = delete;

        void set_two_pass(bool twopass) {
            std::unique_lock<std::mutex> lock{eq_lock};
            this->two_pass = twopass;
        }
        bool is_two_pass() {
            std::unique_lock<std::mutex> lock{eq_lock};
            return this->two_pass;
        }
        const EqualizerParams& preset(int index) {
            std::unique_lock<std::mutex> lock{eq_lock};
            return presets[index];
        }
        int preset_count() {
            std::unique_lock<std::mutex> lock{eq_lock};
            return NUM_PRESETS;
        }

        std::string preset_name(int index) {
            std::unique_lock<std::mutex> lock{eq_lock};
            if (index < 0 || index >= NUM_PRESETS) return "";
            return presets[index].name;
        }

        float preset_frequency(int preset_index, int freq_index) {
            std::unique_lock<std::mutex> lock{eq_lock};
            if (preset_index < 0 || freq_index < 0 || preset_index >= NUM_PRESETS || freq_index >= NUM_BANDS) return 0.0;
            return presets[preset_index].amp[freq_index];
        }

        int bands_count() {
            std::unique_lock<std::mutex> lock{eq_lock};
            return NUM_BANDS;
        }

        void set_params(EqualizerParams params) {
            std::unique_lock<std::mutex> lock{eq_lock};
            for (int i = 0; i < params.amp.size(); i++) {
                state.amp[i] = EqzConvertdB(params.amp[i]);
            }
            state.nb_bands = params.nb_bands;
            set_preamp_internal(params.preamp);
            current_params = params;
        }

        float get_preamp() {
            std::unique_lock<std::mutex> lock{eq_lock};
            return state.gamp;
        }

        void set_preamp(float preamp) {
            std::unique_lock<std::mutex> lock{eq_lock};
            set_preamp_internal(preamp);
        }

        void set_frequency(int index, float value) {
            std::unique_lock<std::mutex> lock{eq_lock};
            if (index >= state.amp.size() || index < 0 || value < -20.f || value > 20.f) return;
            state.amp[index] = EqzConvertdB(value);
            current_params.amp[index] = value;
        }

        float get_frequency(int index) {
            std::unique_lock<std::mutex> lock{eq_lock};
            if (index < 0 || index >= state.amp.size()) return 0.f;
            return current_params.amp[index];
        }

        bool filter(float* in, int nb_samples, int nb_channels) {
            std::unique_lock<std::mutex> lock{eq_lock};
            if (!in) return false;
            for (int i = 0; i < nb_samples; i++) {
                for (int ch = 0; ch < nb_channels; ch++) {
                    const float x = in[ch];
                    float o = 0.f;

                    for(int j = 0; j < state.nb_bands; j++ ) {
                        float y = state.alpha[j] * ( x - state.x[ch][1] ) +
                                  state.gamma[j] * state.y[ch][j][0] -
                                  state.beta[j]  * state.y[ch][j][1];

                        state.y[ch][j][1] = state.y[ch][j][0];
                        state.y[ch][j][0] = y;

                        o += y * state.amp[j];
                    }
                    state.x[ch][1] = state.x[ch][0];
                    state.x[ch][0] = x;

                    /* Second filter */
                    if( two_pass ) {
                        const float x2 = EQZ_IN_FACTOR * x + o;
                        o = 0.0f;
                        for(int j = 0; j < state.nb_bands; j++ )
                        {
                            float y = state.alpha[j] * ( x2 - state.x2[ch][1] ) +
                                      state.gamma[j] * state.y2[ch][j][0] -
                                      state.beta[j]  * state.y2[ch][j][1];

                            state.y2[ch][j][1] = state.y2[ch][j][0];
                            state.y2[ch][j][0] = y;

                            o += y * state.amp[j];
                        }
                        state.x2[ch][1] = state.x2[ch][0];
                        state.x2[ch][0] = x2;

                        /* We add source PCM + filtered PCM */
                        in[ch] = state.gamp * state.gamp *( EQZ_IN_FACTOR * x2 + o );
                    }
                    else
                    {
                        /* We add source PCM + filtered PCM */
                        in[ch] = state.gamp *( EQZ_IN_FACTOR * x + o );
                    }
                }
                in += nb_channels;
            }
            return true;
        }

    private:
        std::mutex eq_lock{};
        bool two_pass;
        EqualizerParams current_params;
        EqualizerState state{};
        int sample_rate;

        void set_preamp_internal(float preamp) {
            if (preamp < -20.f || preamp > 20.f) return; // TODO
            float val;
            if (preamp < -20.f) val = .1f;
            else if (preamp < 20.f) val = powf(10.f, preamp / 20.f);
            else val = 10.f;
            state.gamp = val;
            current_params.preamp = preamp;
        }

        void init_config() {
            EqualizerConfig config{};
            EqzCoeffs(sample_rate, 1.0f, &config);
            state.nb_bands = config.band_count;
            for (int i = 0; i < state.nb_bands; i++) {
                state.alpha[i] = config.band[i].alpha;
                state.beta[i] = config.band[i].beta;
                state.gamma[i] = config.band[i].gamma;
            }
            state.gamp = 1.0f;
            memset(&state.amp[0], 0, state.amp.size());

            for (int ch = 0; ch < 32; ch++) {
                state.x[ch][0]  =
                state.x[ch][1]  =
                state.x2[ch][0] =
                state.x2[ch][1] = 0.0f;

                for(int i = 0; i < state.nb_bands; i++ )
                {
                    state.y[ch][i][0]  =
                    state.y[ch][i][1]  =
                    state.y2[ch][i][0] =
                    state.y2[ch][i][1] = 0.0f;
                }
            }
        }

        /* Equalizer coefficient calculation function based on equ-xmms */
        static void EqzCoeffs(int i_rate, float f_octave_percent, EqualizerConfig* config) {
            const float *f_freq_table_10b = frequency_table;
            float f_rate = (float) i_rate;
            float f_nyquist_freq = 0.5f * f_rate;
            float f_octave_factor = powf( 2.0f, 0.5f * f_octave_percent );
            float f_octave_factor_1 = 0.5f * ( f_octave_factor + 1.0f );
            float f_octave_factor_2 = 0.5f * ( f_octave_factor - 1.0f );

            config->band_count = NUM_BANDS;

            for( int i = 0; i < NUM_BANDS; i++ ) {
                float f_freq = f_freq_table_10b[i];

                config->band[i].frequency = f_freq;

                if( f_freq <= f_nyquist_freq ) {
                    float f_theta_1 = ( 2.0f * (float) M_PI * f_freq ) / f_rate;
                    float f_theta_2 = f_theta_1 / f_octave_factor;
                    float f_sin     = sinf( f_theta_2 );
                    float f_sin_prd = sinf( f_theta_2 * f_octave_factor_1 )
                                      * sinf( f_theta_2 * f_octave_factor_2 );
                    float f_sin_hlf = f_sin * 0.5f;
                    float f_den     = f_sin_hlf + f_sin_prd;

                    config->band[i].alpha = f_sin_prd / f_den;
                    config->band[i].beta  = ( f_sin_hlf - f_sin_prd ) / f_den;
                    config->band[i].gamma = f_sin * cosf( f_theta_1 ) / f_den;
                }
                else
                {
                    /* Any frequency beyond the Nyquist frequency is no good... */
                    config->band[i].alpha =
                    config->band[i].beta  =
                    config->band[i].gamma = 0.0f;
                }
            }
        }

        inline float EqzConvertdB( float db ) {
            /* Map it to gain,
            * (we do as if the input of iir is /EQZ_IN_FACTOR, but in fact it's the non iir data that is *EQZ_IN_FACTOR)
            * db = 20*log( out / in ) with out = in + amp*iir(i/EQZ_IN_FACTOR)
            * or iir(i) == i for the center freq so
            * db = 20*log( 1 + amp/EQZ_IN_FACTOR )
            * -> amp = EQZ_IN_FACTOR*(10^(db/20) - 1)
            **/

            if( db < -20.0f )
                db = -20.0f;
            else if(  db > 20.0f )
                db = 20.0f;
            return EQZ_IN_FACTOR * ( powf( 10.0f, db / 20.0f ) - 1.0f );
        }

        const EqualizerParams presets[NUM_PRESETS] {
                {
                        "Flat", 10, 12.0f,
                        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                },
                {
                        "Classical", 10, 12.0f,
                        { -1.11022e-15f, -1.11022e-15f, -1.11022e-15f, -1.11022e-15f,
                                                  -1.11022e-15f, -1.11022e-15f, -7.2f, -7.2f, -7.2f, -9.6f }
                },
                {
                        "Club", 10, 6.0f,
                        { -1.11022e-15f, -1.11022e-15f, 8.0f, 5.6f, 5.6f, 5.6f, 3.2f,
                                -1.11022e-15f, -1.11022e-15f, -1.11022e-15f }
                },
                {
                        "Dance", 10, 5.0f,
                        { 9.6f, 7.2f, 2.4f, -1.11022e-15f, -1.11022e-15f, -5.6f, -7.2f, -7.2f,
                                -1.11022e-15f, -1.11022e-15f }
                },
                {
                        "Full Bass", 10, 5.0f,
                        { -8.0f, 9.6f, 9.6f, 5.6f, 1.6f, -4.0f, -8.0f, -10.4f, -11.2f, -11.2f }
                },
                {
                        "Full Bass & Treble", 10, 4.0f,
                        { 7.2f, 5.6f, -1.11022e-15f, -7.2f, -4.8f, 1.6f, 8.0f, 11.2f,
                                12.0f, 12.0f }
                },
                {
                        "Full Treble", 10, 3.0f,
                        { -9.6f, -9.6f, -9.6f, -4.0f, 2.4f, 11.2f, 16.0f, 16.0f, 16.0f, 16.8f }
                },
                {
                        "Headphones", 10, 4.0f,
                        { 4.8f, 11.2f, 5.6f, -3.2f, -2.4f, 1.6f, 4.8f, 9.6f, 12.8f, 14.4f }
                },
                {
                        "Large Hall", 10, 5.0f,
                        { 10.4f, 10.4f, 5.6f, 5.6f, -1.11022e-15f, -4.8f, -4.8f, -4.8f,
                                -1.11022e-15f, -1.11022e-15f }
                },
                {
                        "Live", 10, 7.0f,
                        { -4.8f, -1.11022e-15f, 4.0f, 5.6f, 5.6f, 5.6f, 4.0f, 2.4f,
                                2.4f, 2.4f }
                },
                {
                        "Party", 10, 6.0f,
                        { 7.2f, 7.2f, -1.11022e-15f, -1.11022e-15f, -1.11022e-15f,
                                -1.11022e-15f, -1.11022e-15f, -1.11022e-15f, 7.2f, 7.2f }
                },
                {
                        "Pop", 10, 6.0f,
                        { -1.6f, 4.8f, 7.2f, 8.0f, 5.6f, -1.11022e-15f, -2.4f, -2.4f,
                                -1.6f, -1.6f }
                },
                {
                        "Reggae", 10, 8.0f,
                        { -1.11022e-15f, -1.11022e-15f, -1.11022e-15f, -5.6f, -1.11022e-15f,
                                6.4f, 6.4f, -1.11022e-15f, -1.11022e-15f, -1.11022e-15f }
                },
                {
                        "Rock", 10, 5.0f,
                        { 8.0f, 4.8f, -5.6f, -8.0f, -3.2f, 4.0f, 8.8f, 11.2f, 11.2f, 11.2f }
                },
                {
                        "Ska", 10, 6.0f,
                        { -2.4f, -4.8f, -4.0f, -1.11022e-15f, 4.0f, 5.6f, 8.8f, 9.6f,
                                11.2f, 9.6f }
                },
                {
                        "Soft", 10, 5.0f,
                        { 4.8f, 1.6f, -1.11022e-15f, -2.4f, -1.11022e-15f, 4.0f, 8.0f, 9.6f,
                                11.2f, 12.0f }
                },
                {
                        "Soft Rock", 10, 7.0f,
                        { 4.0f, 4.0f, 2.4f, -1.11022e-15f, -4.0f, -5.6f, -3.2f, -1.11022e-15f,
                                2.4f, 8.8f }
                },
                {
                        "Techno", 10, 5.0f,
                        { 8.0f, 5.6f, -1.11022e-15f, -5.6f, -4.8f, -1.11022e-15f, 8.0f, 9.6f,
                                9.6f, 8.8f }
                },
        };
    };

} // namespace libmediafy_equalizer
