/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
 * FM stereo implementation by Alex Grinkov a.grinkov(at)gmail.com.
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include <cmath>
#include <iostream>
#include <sys/types.h>
#include <signal.h>
#include "receivers/nrsc5rx.h"

#define PREF_QUAD_RATE   1488375
#define TEMP_PIPE "/tmp/gqrx_nrsc5.sock"
#define SERVER "127.0.0.1"
#define PORT 8888   //The port on which to send data

nrsc5rx_sptr make_nrsc5rx(float quad_rate, int channel)
{
    return gnuradio::get_initial_sptr(new nrsc5rx(quad_rate, channel));
}

nrsc5rx::nrsc5rx(float quad_rate, int channel)
    : receiver_base_cf("NRSC5RX"),
      d_running(false),
      d_quad_rate(quad_rate),
      d_channel(channel),
      d_nrsc5_pid(0)
{
    pid_t mkfifo_pid = fork();
    if (mkfifo_pid == 0)
    {
        execl("/usr/bin/mkfifo", "/usr/bin/mkfifo", TEMP_PIPE, (char*)0);
        exit(1);
    }

    iq_resamp = make_resampler_cc(PREF_QUAD_RATE/d_quad_rate);

    meter = make_rx_meter_c(DETECTOR_TYPE_RMS);
    complex_to_float = gr::blocks::complex_to_float::make(1);
    multiply_const_1 = gr::blocks::multiply_const_ff::make(125.0f);
    multiply_const_2 = gr::blocks::multiply_const_ff::make(125.0f);
    add_const_1 = gr::blocks::add_const_ff::make(127.0f);
    add_const_2 = gr::blocks::add_const_ff::make(127.0f);
    float_to_uchar_1 = gr::blocks::float_to_uchar::make();
    float_to_uchar_2 = gr::blocks::float_to_uchar::make();
    interleave = gr::blocks::interleave::make(1);
    
    d_nrsc5_pid = fork();
    if(d_nrsc5_pid == 0) 
    {
        execl("/home/jason/Projects/nrsc5/build/src/nrsc5", "/home/jason/Projects/nrsc5/build/src/nrsc5", "-r", TEMP_PIPE, std::to_string(channel).c_str(), (char*)0);
        exit(1);
    }

    connect(self(), 0, iq_resamp, 0);
    connect(iq_resamp, 0, meter, 0);
    connect(iq_resamp, 0, complex_to_float, 0);
    connect(complex_to_float, 0, multiply_const_1, 0);
    connect(complex_to_float, 1, multiply_const_2, 0);
    connect(multiply_const_1, 0, add_const_1, 0);
    connect(multiply_const_2, 0, add_const_2, 0);
    connect(add_const_1, 0, float_to_uchar_1, 0);
    connect(add_const_2, 0, float_to_uchar_2, 0);
    connect(float_to_uchar_1, 0, interleave, 0);
    connect(float_to_uchar_2, 0, interleave, 1);

    file_sink = gr::blocks::file_sink::make(1, TEMP_PIPE);

    connect(interleave, 0, file_sink, 0);

    connect(iq_resamp, 0, self(), 0);
    connect(iq_resamp, 0, self(), 1);
}

nrsc5rx::~nrsc5rx()
{
}

bool nrsc5rx::start()
{
    d_running = true;
    return true;
}

bool nrsc5rx::stop()
{
    d_running = false;
    return true;
}

void nrsc5rx::set_quad_rate(float quad_rate)
{
    if (std::abs(d_quad_rate-quad_rate) > 0.5)
    {
#ifndef QT_NO_DEBUG_OUTPUT
        std::cerr << "Changing WFM RX quad rate: "  << d_quad_rate << " -> " << quad_rate << std::endl;
#endif
        d_quad_rate = quad_rate;
        lock();
        iq_resamp->set_rate(PREF_QUAD_RATE/d_quad_rate);
        unlock();
    }
}

void nrsc5rx::set_audio_rate(float audio_rate)
{
    (void) audio_rate;
}

void nrsc5rx::set_filter(double low, double high, double tw)
{

}

float nrsc5rx::get_signal_level(bool dbfs)
{
    if (dbfs)
        return meter->get_level_db();
    else
        return meter->get_level();

}

void nrsc5rx::set_demod(int channel)
{
    /* check if new channel selection is valid */
    if ((channel < 0) || (channel >= 3))
        return;

    if (channel == d_channel) {
        /* nothing to do */
        return;
    }

    d_channel = channel;

    // send udp packet to nrsc5 app to change program.
    struct sockaddr_in si_other;
    int s, slen=sizeof(si_other);
 
    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        return;
    }
 
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
     
    if (inet_aton(SERVER , &si_other.sin_addr) == 0) 
    {
        return;
    }

    unsigned char program = (unsigned char)d_channel;

    //send the message
    sendto(s, &program, 1 , 0 , (struct sockaddr *) &si_other, slen);
 
    close(s);
}
