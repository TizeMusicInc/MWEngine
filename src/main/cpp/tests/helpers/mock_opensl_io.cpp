#include "mock_opensl_io.h"
#include "../../audioengine.h"
#include "../../sequencer.h"
#include "../../utilities/debug.h"

inline OPENSL_STREAM* mock_android_OpenAudioDevice( int sr, int inchannels, int outchannels, int bufferframes )
{
    Debug::log( "mocked device opening" );

    OPENSL_STREAM *p = ( OPENSL_STREAM* ) calloc( sizeof( OPENSL_STREAM ), 1 );

    p->inchannels  = inchannels;
    p->outchannels = outchannels;
    p->sr = sr;

    AudioEngine::engine_started = true;

    return p;
}

inline void mock_android_CloseAudioDevice( OPENSL_STREAM *p )
{
    Debug::log( "mocked device closing" );

    if ( p != 0 )
        free( p );
}

inline int mock_android_AudioIn( OPENSL_STREAM *p, float *buffer, int size )
{
    AudioEngine::mock_opensl_time += ( float ) size / ( p->sr * p->inchannels );

    return size;
}

inline int mock_android_AudioOut( OPENSL_STREAM *p, float *buffer, int size )
{
    // AudioEngine thread will halt all unit test execution
    // android_AudioOut is called upon each iteration, here
    // we can check whether we can halt the thread

    int outputChannels   = AudioEngineProps::OUTPUT_CHANNELS;
    int singleBufferSize = size / outputChannels;

    Debug::log( "Audio Engine test %d running", AudioEngine::test_program );

    switch ( AudioEngine::test_program )
    {
        case 0: // engine start test
        case 1: // engine tempo update test

            Debug::log( "Audio Engine test %d done", AudioEngine::test_program );

            ++AudioEngine::test_program;    // advance to next test
            AudioEngine::stop();
            break;

        case 2: // output test

            if ( Sequencer::playing )
            {
                // test 1. ensure all buffer iterations are calculated accordingly
                // when this method runs the engine is writing its renderer output (and has thus
                // incremented buffer position pointers accordingly), we can thus assume that on
                // first run the current iteration is 1, not 0 (as it has completed its run)

                int currentIteration = ++AudioEngine::render_iterations;
                int maxIterations    = ( AudioEngine::max_buffer_position - AudioEngine::min_buffer_position ) / AudioEngineProps::BUFFER_SIZE;

                int expectedBufferPosition = currentIteration * AudioEngineProps::BUFFER_SIZE;

                if ( currentIteration == 1 )
                    AudioEngine::test_successful = true; // will be falsified by assertions below

                if ( AudioEngine::bufferPosition != expectedBufferPosition )
                    AudioEngine::test_successful = false;

                // test 2. evaluate buffer contents

                // expected samples as defined in audioengine_test.cpp
                SAMPLE_TYPE event1buffer[] = { -1,-1,-1,-1, 0,0,0,0, 1,1,1,1, 0,0,0,0 };
                SAMPLE_TYPE event2buffer[] = { .5,.5,.5,.5, 1,1,1,1, -.5,-.5,-.5,-.5, -1,-1,-1,-1 };
                SAMPLE_TYPE event3buffer[] = { .25,.25,.25,.25, 0,0,0,0, -.25,-.25,-.25,-.25, 0,0,0,0 };

                int event2start = 16;
                int event2end   = event2start + 16;
                int event3start = 24; // event 3 ends at singleBufferSize end

                for ( int i = 0, j = 0; i < singleBufferSize; ++i, j += AudioEngineProps::OUTPUT_CHANNELS )
                {
                    // minus 1 as we are testing the last iterations output buffer
                    int sequencerPos = (( currentIteration - 1 ) * AudioEngineProps::BUFFER_SIZE ) + i;

                    // 16 == size of the expected event buffers
                    // note that the contents of float* are interleaved
                    // (every other value is a sample for the other channel, hence the j increment)
                    int readOffset = (( currentIteration * AudioEngineProps::BUFFER_SIZE ) + j ) % 16;

                    SAMPLE_TYPE leftSample  = buffer[ j ];
                    SAMPLE_TYPE rightSample = buffer[ j + 1 ];

                    SAMPLE_TYPE expectedLeftSample  = 0.f;
                    SAMPLE_TYPE expectedRightSample = 0.f;

                    // test 2.1 test event1buffer (range 0 - 16)

                    if ( sequencerPos < event2start ) {
                        // mono event will be mixed into both channels
                        expectedLeftSample = expectedRightSample = event1buffer[ sequencerPos ];
                    }
                    else if ( sequencerPos >= event2start && sequencerPos < event3start ) {
                        // stereo event that only has right channel contents
                        expectedRightSample = event2buffer[ sequencerPos - event2start ];
                    }
                    else if ( sequencerPos >= event3start )
                    {
                        // left buffer is expected to contain event 3 samples only
                        expectedLeftSample  = event3buffer[ sequencerPos - event3start ];

                        // right buffer will have overlap with tail of event 2
                        expectedRightSample = event3buffer[ sequencerPos - event3start ];

                        if ( sequencerPos <= event2end )
                            expectedRightSample += event2buffer[ sequencerPos - event2start ];
                    }

                    if ( leftSample != expectedLeftSample )
                    {
                        Debug::log( "TEST 2 expected left sample: %f, got %f at buffer readoffset %d at sequencer position %d",
                                    expectedLeftSample, leftSample, readOffset, sequencerPos );
                    }
                    else if ( rightSample != expectedRightSample )
                    {
                        Debug::log( "TEST 2 expected right sample: %f, got %f at buffer readoffset %d at sequencer position %d",
                                    expectedRightSample, rightSample, readOffset, sequencerPos );
                    }
                    else {
                        continue;
                    }
                    AudioEngine::test_successful = false;
                    AudioEngine::stop();
                    break;
                }

                // stop the engine once it has rendered the full loop range

                if ( currentIteration == maxIterations )
                {
                    Debug::log( "Audio Engine test %d done", AudioEngine::test_program );

                    ++AudioEngine::test_program;    // advance to next test
                    AudioEngine::stop();
                }
            }
            break;

        case 3: // loop test

            if ( Sequencer::playing )
            {
                AudioEngine::test_successful = true; // will be falsified by assertions below

                // test 3. ensure the buffers of both the event at the end of the loop and
                // at the start of the Sequencers loop have been mixed into the output buffer

                for ( int i = 0, c = 0, bufferPosition = 88100; i < singleBufferSize; ++i, ++bufferPosition, c += outputChannels )
                {
                    // 77175 being audioEvent1 start, -0.25f being audioEvent1 contents, _0.5f being audioEvent2 contents
                    SAMPLE_TYPE expected = ( bufferPosition > 77175 ) ? -0.25f : +0.5f;
                    // divide by amount of channels (as volume is corrected for summing purposes)
                    expected /= 2;

                    SAMPLE_TYPE sample = buffer[ c ];

                    if ( sample != expected )
                    {
                        Debug::log( "TEST 3 expected %f, got %f at iteration %d (buffer pos %d)", expected, sample, i, bufferPosition );

                        AudioEngine::test_successful = false;
                        AudioEngine::stop();
                        break;
                    }

                    if ( bufferPosition >= AudioEngine::max_buffer_position )
                        bufferPosition = AudioEngine::min_buffer_position - 1; // will be incremented at end of iteration
                }

                Debug::log( "Audio Engine test %d done", AudioEngine::test_program );

                // stop the engine

                ++AudioEngine::test_program;    // advance to next test
                AudioEngine::stop();
            }
            break;

        case 4: // multi-measure loop test

            if ( Sequencer::playing )
            {
                // when this method runs the engine is writing its renderer output (and has thus
                // incremented buffer position pointers accordingly), we can thus assume that on
                // first run the current iteration is 1, not 0 (as it has completed its run)

                int currentIteration = ++AudioEngine::render_iterations;
                int maxIterations    = ( AudioEngine::max_buffer_position - AudioEngine::min_buffer_position ) / AudioEngineProps::BUFFER_SIZE;

                int expectedBufferPosition = currentIteration * AudioEngineProps::BUFFER_SIZE;

                AudioEngine::test_successful = true; // will be falsified by assertions below

                // test 3. ensure the buffers of both the event at the end of the loop and
                // at the start of the Sequencers loop have been mixed into the output buffer

                for ( int i = 0, c = 0, bufferPosition = 88100; i < singleBufferSize; ++i, ++bufferPosition, c += outputChannels )
                {
                    // 77175 being audioEvent1 start, -0.25f being audioEvent1 contents, _0.5f being audioEvent2 contents
                    SAMPLE_TYPE expected = ( bufferPosition > 77175 ) ? -0.25f : +0.5f;
                    // divide by amount of channels (as volume is corrected for summing purposes)
                    expected /= 2;

                    SAMPLE_TYPE sample = buffer[ c ];

                    if ( sample != expected )
                    {
                        Debug::log( "TEST 3 expected %f, got %f at iteration %d (buffer pos %d)", expected, sample, i, bufferPosition );

                        AudioEngine::test_successful = false;
                        AudioEngine::stop();
                        break;
                    }

                    if ( bufferPosition >= AudioEngine::max_buffer_position )
                        bufferPosition = AudioEngine::min_buffer_position - 1; // will be incremented at end of iteration
                }

                // stop the engine once it has rendered the full loop range

                if ( currentIteration == maxIterations )
                {
                    Debug::log( "Audio Engine test %d done", AudioEngine::test_program );

                    ++AudioEngine::test_program;    // advance to next test
                    AudioEngine::stop();
                }
            }

            break;
    }
    return size;
}

inline float mock_android_GetTimestamp( OPENSL_STREAM *p )
{
    return AudioEngine::mock_opensl_time;
}
