// Copyright Hugh Perkins 2015 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <cstring>

#include "OpenCLHelper.h"

#include "StatefulTimer.h"

#include "DropoutPropagateCpu.h"

using namespace std;

#undef VIRTUAL
#define VIRTUAL 
#undef STATIC
#define STATIC

DropoutPropagateCpu::DropoutPropagateCpu( OpenCLHelper *cl, int numPlanes, int inputImageSize, float dropRatio ) :
        DropoutPropagate( cl, numPlanes, inputImageSize, dropRatio ) {
}
VIRTUAL void DropoutPropagateCpu::propagate( int batchSize, CLWrapper *inputWrapper, CLWrapper *outputWrapper ) {
//    cout << "DropoutPropagateCpu::propagate( CLWrapper * )" << endl;

    inputWrapper->copyToHost();

    float *input = reinterpret_cast<float *>( inputWrapper->getHostArray() );
    float *output = new float[ getResultsSize( batchSize ) ];

    propagate( batchSize, input, output );

    float *outputHostArray = reinterpret_cast<float *>( outputWrapper->getHostArray() );
    memcpy( outputHostArray, output, sizeof(float) * getResultsSize( batchSize ) );

    outputWrapper->copyToDevice();

    delete[] output;
}
VIRTUAL void DropoutPropagateCpu::propagate( int batchSize, float *input, float *output ) {
//    float *output = new float[ getResultsSize( batchSize ) ];
//    cout << "DropoutPropagateCpu::propagate( float * )" << endl;
    StatefulTimer::instance()->timeCheck("DropoutPropagateCpu::propagate start" );
//    for( int n = 0; n < batchSize; n++ ) {
//        for( int plane = 0; plane < numPlanes; plane++ ) {
//            for( int outputRow = 0; outputRow < outputImageSize; outputRow++ ) {
//                int inputRow = outputRow * dropoutSize;
//                for( int outputCol = 0; outputCol < outputImageSize; outputCol++ ) {
//                    int inputCol = outputCol * dropoutSize;
//                    int selector = 0;
//                    int resultIndex = getResultIndex( n, plane, outputRow, outputCol );
//                    output[ resultIndex ] = maxValue;
//                }
//            }
//        }
//    }
    StatefulTimer::instance()->timeCheck("DropoutPropagateCpu::propagate end" );
//    return output;
}

