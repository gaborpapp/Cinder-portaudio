/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

#include "portaudio.h"

#define SAMPLE_RATE 44100

using namespace ci;
using namespace ci::app;
using namespace std;

class PaTest1App : public AppBasic
{
	public:
		void setup();
		void shutdown();

		void keyDown( KeyEvent event );

		void draw();

	private:
		typedef struct
		{
			float sine[ 100 ];
			int phase;
			int sampsToGo;
		}
		patest1data;

		patest1data data;
		PaStream *stream;

		static int patest1Callback( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
				const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
				void *userData );

		int checkError( const PaError &err );
};

void PaTest1App::setup()
{
    /* initialise sinusoidal wavetable */
    for ( int i = 0; i < 100; i++ )
        data.sine[i] = math<float>::sin( ( (float)i / 100.f ) * M_PI * 2.f );
    data.phase = 0;
    data.sampsToGo = SAMPLE_RATE * 20;        /* 20 seconds. */

    PaError err;
    PaStreamParameters outputParameters;

    /* initialise portaudio subsytem */
    err = Pa_Initialize();

    outputParameters.device = Pa_GetDefaultOutputDevice();  /* default output device */
    if ( outputParameters.device == paNoDevice )
	{
		app::console() << "Error: No default output device." << endl;
		quit();
	}
    outputParameters.channelCount = 2;                      /* stereo output */
    outputParameters.sampleFormat = paFloat32;              /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
                        &stream,
                        NULL,
                        &outputParameters,
                        (double)SAMPLE_RATE, /* Samplerate in Hertz. */
                        512,                 /* Small buffers */
                        paClipOff,           /* We won't output out of range samples so don't bother clipping them. */
                        &PaTest1App::patest1Callback,
                        &data );
	if ( checkError( err ) )
		quit();

    err = Pa_StartStream( stream );
	if ( checkError( err ) )
		quit();
}

int PaTest1App::patest1Callback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    patest1data *data = (patest1data*)userData;
    float *out = (float*)outputBuffer;
    int framesToCalc = framesPerBuffer;
    unsigned long i = 0;
    int finished;

    if( data->sampsToGo < framesPerBuffer )
    {
        framesToCalc = data->sampsToGo;
        finished = paComplete;
    }
    else
    {
        finished = paContinue;
    }

    for( ; i<framesToCalc; i++ )
    {
        *out++ = data->sine[data->phase];  /* left */
        *out++ = data->sine[data->phase++];  /* right */
        if( data->phase >= 100 )
            data->phase = 0;
    }

    data->sampsToGo -= framesToCalc;

    /* zero remainder of final buffer if not already done */
    for( ; i<framesPerBuffer; i++ )
    {
        *out++ = 0; /* left */
        *out++ = 0; /* right */
    }

    return finished;
}

void PaTest1App::shutdown()
{
    PaError err;
    err = Pa_AbortStream( stream );
	checkError( err );

    /* sleep until playback has finished */
    while ( ( err = Pa_IsStreamActive( stream ) ) == 1 )
		Pa_Sleep( 1000 );
	checkError( err );
    //if ( err < 0 ) goto done;

    err = Pa_CloseStream( stream );
    //if ( err != paNoError ) goto done;
	checkError( err );
	Pa_Terminate();
}

int PaTest1App::checkError( const PaError &err )
{
	if ( err != paNoError )
	{
		app::console() << "An error occured while using portaudio" << endl;
		if ( err == paUnanticipatedHostError )
		{
			app::console() << " unanticipated host error." << endl;
			const PaHostErrorInfo* herr = Pa_GetLastHostErrorInfo();
			if ( herr )
			{
				app::console() << " Error number: " << herr->errorCode << endl;
				if ( herr->errorText )
					app::console() << " Error text: " << herr->errorText << endl;
			}
			else
				app::console() << " Pa_GetLastHostErrorInfo() failed!" << endl;
		}
		else
		{
			app::console() << " Error number: " << err << endl;
			app::console() << " Error text: " << Pa_GetErrorText( err ) << endl;
		}

		return 1;
	}

	return 0;
}

void PaTest1App::draw()
{
	gl::clear( Color::black() );
}

void PaTest1App::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( PaTest1App, RendererGl )

