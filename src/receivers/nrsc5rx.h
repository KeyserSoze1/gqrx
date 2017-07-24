/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
 * NRSC5 implementation by Jason Pontious jpontious(at)gmail.com.
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
#ifndef NRSC5RX_H
#define NRSC5RX_H

#include "receivers/receiver_base.h"
#include "dsp/rx_meter.h"
#include "dsp/resampler_xx.h"

#include <gnuradio/blocks/complex_to_float.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/blocks/add_const_ff.h>
#include <gnuradio/blocks/float_to_uchar.h>
#include <gnuradio/blocks/interleave.h>
#include <gnuradio/blocks/file_sink.h>

#include <arpa/inet.h>
#include <sys/socket.h>

class nrsc5rx;

typedef boost::shared_ptr<nrsc5rx> nrsc5rx_sptr;

/*! \brief Public constructor of nrsc5rx. */
nrsc5rx_sptr make_nrsc5rx(float quad_rate, int channel);

/*! \brief iBiquity iBOC receiver.
 *  \ingroup RX
 *
 * This block provides receiver for HD radio transmissions.
 */
class nrsc5rx : public receiver_base_cf
{

public:
    nrsc5rx(float quad_rate, int channel);
    ~nrsc5rx();

    virtual bool start();
    virtual bool stop();

    virtual void set_quad_rate(float quad_rate);
    virtual void set_audio_rate(float audio_rate);

    virtual void set_filter(double low, double high, double tw);
    virtual void set_cw_offset(double offset) { (void)offset; }

    virtual float get_signal_level(bool dbfs);

    virtual void set_demod(int demod);

private:
    bool   d_running;          /*!< Whether receiver is running or not. */
    float  d_quad_rate;        /*!< Input sample rate. */

    int    d_channel;   /*!< Current channel. */
    pid_t d_nrsc5_pid;

    resampler_cc_sptr         iq_resamp; /*!< Baseband resampler. */

    /*!< Reverse samples back to native rtl-sdr format. */
    gr::blocks::complex_to_float::sptr complex_to_float;
    gr::blocks::multiply_const_ff::sptr multiply_const_1;
    gr::blocks::multiply_const_ff::sptr multiply_const_2;
    gr::blocks::add_const_ff::sptr add_const_1;
    gr::blocks::add_const_ff::sptr add_const_2;
    gr::blocks::float_to_uchar::sptr float_to_uchar_1;
    gr::blocks::float_to_uchar::sptr float_to_uchar_2;
    gr::blocks::interleave::sptr interleave;
    gr::blocks::file_sink::sptr file_sink;

    rx_meter_c_sptr           meter;     /*!< Signal strength. */

};

#endif // NRSC5RX_H
