#include "pot.h"

static uint8_t potCycleCounter = 0;
static uint8_t newPotCounter = 0;
static uint8_t smoothPotValues = 0;
static uint8_t newPotXCandidate = 128, newPotYCandidate = 128;
static uint8_t newPotXCandidate2S = 128, newPotYCandidate2S = 128;
static uint8_t skipSmoothing = 0;
uint8_t skipMeasurements = 0;
uint8_t outRegisters25 = 0;
uint8_t outRegisters26 = 0;
uint8_t paddleFilterMode = 0;

void pot_init(void)
{

}

void pot_process(void)
{
    if ( potCycleCounter == 0 )
    {
        if ( newPotCounter & 4 )			// in phase 2?
        {
            gpioDir |= bPOTX | bPOTY;       // enter phase 1
            newPotCounter = 0;

            if ( POT_OUTLIER_REJECTION > 1 )
            {
                #define GUARD 8
                if ( ( newPotXCandidate < ( 64 - GUARD ) || newPotXCandidate > ( 192 + GUARD ) ) ||
                        ( newPotYCandidate < ( 64 - GUARD ) || newPotYCandidate > ( 192 + GUARD ) ) )
                    skipMeasurements = 2;
            }

            if ( skipMeasurements )
            {
                skipMeasurements --;
                skipSmoothing = 1;
            } else
            {
                skipSmoothing = 0;

                #define max( a, b ) ( (a)>(b)?(a):(b) )
                #define min( a, b ) ( (a)<(b)?(a):(b) )
                #ifdef DIAGROM_HACK
                    if ( DIAGROM_THRESHOLD >= 80 )
                    {
                        if ( abs( newPotXCandidate - newPotYCandidate ) < 10 && newPotXCandidate >= 50 && newPotXCandidate < DIAGROM_THRESHOLD )
                        {
                            if ( presumablyFixedResistor < 40000 )
                                presumablyFixedResistor ++;
                        } else
                        {
                            if ( presumablyFixedResistor )
                                presumablyFixedResistor --;
                        }
                        if ( presumablyFixedResistor > 1000 )
                            diagROM_PaddleOffset = min( DIAGROM_THRESHOLD - newPotXCandidate, ( presumablyFixedResistor - 1000 ) / 1000 ); else
                            diagROM_PaddleOffset = 0;
                        newPotXCandidate = min( 255, (int)newPotXCandidate + (int)diagROM_PaddleOffset );
                        newPotYCandidate = min( 255, (int)newPotYCandidate + (int)diagROM_PaddleOffset );
                    } else
                    if ( DIAGROM_THRESHOLD > 1 )
                    {
                        newPotXCandidate = min( 255, (int)newPotXCandidate + (int)DIAGROM_THRESHOLD );
                        newPotYCandidate = min( 255, (int)newPotYCandidate + (int)DIAGROM_THRESHOLD );
                    }
                #endif

                if ( !paddleFilterMode )
                {
                    outRegisters25 = newPotXCandidate;
                    outRegisters26 = newPotYCandidate;
                } else
                if ( !smoothPotValues )
                {
                    newPotXCandidate2S = newPotXCandidate;
                    newPotYCandidate2S = newPotYCandidate;
                    smoothPotValues = 1;
                }
            }
        } else
        {
            gpioDir &= ~( bPOTX | bPOTY );  // enter phase 2
            newPotCounter = 0b111;
        }
    } else
    if ( newPotCounter & 4 )				// in phase 2, but cycle counter != 0
    {
        if ( ( newPotCounter & 1 ) && ( ( g & bPOTX ) || potCycleCounter == 255 ) )
        {
            newPotXCandidate = potCycleCounter;
            newPotCounter &= 0b110;
        }

        uint8_t potYState = 0;

        if ( config[ 57 ] )
            potYState = ( ( newPotCounter & 2 ) && ( ( (uint16_t)adc_hw->result > 1024 + config[ 57 ] * 64 ) || potCycleCounter == 255 ) ); else
            potYState = ( ( newPotCounter & 2 ) && ( ( g & bPOTY ) || potCycleCounter == 255 ) );

        if ( potYState )
        {
            newPotYCandidate = potCycleCounter;
            newPotCounter &= 0b101;
        } else

        // test validity of measurements
        if ( POT_OUTLIER_REJECTION )
        {
            uint8_t potYState = 0;

            if ( config[ 57 ] )
                potYState = ( !( newPotCounter & 2 ) && !( (uint16_t)adc_hw->result > 1024 + config[ 57 ] * 64 - 256 ) ); else
                potYState = ( !( newPotCounter & 2 ) && !( ( g & bPOTY ) ) );

            if ( ( !( newPotCounter & 1 ) && !( g & bPOTX ) && potCycleCounter == ( ( newPotXCandidate + 255 ) >> 1 ) ) ||
                    ( potYState && potCycleCounter == ( ( newPotYCandidate + 255 ) >> 1 ) ) )
                skipMeasurements = 2;
        }
    }

    potCycleCounter ++;
}